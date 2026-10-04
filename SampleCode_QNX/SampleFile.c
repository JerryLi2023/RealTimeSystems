#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/dispatch.h>
#include <sys/neutrino.h>
#include <sys/iomsg.h>

#define ATTACH_POINT "Pedestrian"
#define BUF_SIZE 100
#define DATA_TYPE 0x22

#define SET_PEDESTRIAN_STATE 1
#define REQUEST_BUTTON_DATA 2

/* Teacher's padded header, with the unused pointer removed.
 * Use these same message definitions on the traffic-light node.
 */
typedef union {
    uint32_t sival_int;
    uint32_t dummy[4];
} _mysigval;

typedef struct {
    uint16_t type;
    uint16_t subtype;
    int8_t code;
    uint8_t zero[3];
    _mysigval value;
    uint8_t zero2[2];
    int32_t scoid;
} msg_header_t;

/* Traffic light -> pedestrian server. */
typedef struct {
    msg_header_t hdr;
    int32_t ClientID;
    int32_t trafficstate;
    int32_t time;
    int32_t period;
} my_data;

/* Pedestrian button data -> traffic light.
 * Also used as the reply to REQUEST_BUTTON_DATA.
 */
typedef struct {
    msg_header_t hdr;
    int32_t ClientID;
    int32_t LeftNorthSouth;
    int32_t RightNorthSouth;
    int32_t TopEastWest;
    int32_t BottomEastWest;
} client_data;

/* Text acknowledgement. */
typedef struct {
    msg_header_t hdr;
    char buf[BUF_SIZE];
} my_reply;


/*** Pedestrian server ***/
int server(PedstrianLight *light)
{
    name_attach_t *attach;
    my_data msg = {0};
    my_reply replymsg = {0};

    /* Receive either a native QNX pulse or an application message.
     * Copy application messages into msg to keep the teacher's
     * msg.hdr / msg.trafficstate access style.
     */
    union {
        struct _pulse pulse;
        my_data message;
    } received;

    int rcvid;
    int msgnum = 0;
    int Stay_alive = 0; /* Teacher's setting: 0 exits on disconnect. */
    int living = 1;
    int status = EXIT_SUCCESS;

    replymsg.hdr.type = 0x01;

    if ((attach = name_attach(NULL, ATTACH_POINT, 0)) == NULL) {
        perror("name_attach");
        return EXIT_FAILURE;
    }

    printf("Server listening on: %s\n", ATTACH_POINT);

    while (living) {
        memset(&received, 0, sizeof(received));
        rcvid = MsgReceive(attach->chid, &received,
                           sizeof(received), NULL);

        if (rcvid == -1) {
            perror("MsgReceive");
            status = EXIT_FAILURE;
            break;
        }

        /* Pulses use the native QNX structure. */
        if (rcvid == 0) {
            switch (received.pulse.code) {
                case _PULSE_CODE_DISCONNECT:
                    ConnectDetach(received.pulse.scoid);
                    if (Stay_alive == 0)
                        living = 0;
                    break;

                case _PULSE_CODE_UNBLOCK:
                case _PULSE_CODE_COIDDEATH:
                case _PULSE_CODE_THREADDEATH:
                default:
                    break;
            }
            continue;
        }

        /* Ordinary message: keep the teacher's msg.field format. */
        msg = received.message;

        if (msg.hdr.type == _IO_CONNECT) {
            MsgReply(rcvid, EOK, NULL, 0);
            continue;
        }

        if (msg.hdr.type >= _IO_BASE && msg.hdr.type <= _IO_MAX) {
            MsgError(rcvid, ENOSYS);
            continue;
        }

        if (msg.hdr.type != DATA_TYPE) {
            MsgError(rcvid, ENOSYS);
            continue;
        }

        switch (msg.hdr.subtype) {
            case SET_PEDESTRIAN_STATE:
                pthread_mutex_lock(&light_mutex);
                light->currentState =
                    (PedestrianCombination)msg.trafficstate;
                light->timer = msg.time;
                light->period = msg.period;
                pthread_mutex_unlock(&light_mutex);

                msgnum++;
                snprintf(replymsg.buf, BUF_SIZE,
                         "Message %d received", msgnum);
                replymsg.hdr.subtype = SET_PEDESTRIAN_STATE;

                MsgReply(rcvid, EOK, &replymsg, sizeof(replymsg));
                break;

            case REQUEST_BUTTON_DATA: {
                client_data buttons = {0};

                buttons.hdr.type = DATA_TYPE;
                buttons.hdr.subtype = REQUEST_BUTTON_DATA;
                buttons.ClientID = 800;

                pthread_mutex_lock(&light_mutex);
                buttons.LeftNorthSouth = light->LeftNorthSouth;
                buttons.RightNorthSouth = light->RightNorthSouth;
                buttons.TopEastWest = light->TopEastWest;
                buttons.BottomEastWest = light->BottomEastWest;
                pthread_mutex_unlock(&light_mutex);

                /* Return button data only when requested. */
                MsgReply(rcvid, EOK, &buttons, sizeof(buttons));
                break;
            }

            default:
                MsgError(rcvid, ENOSYS);
                break;
        }
    }

    name_detach(attach, 0);
    return status;
}


/*** Pedestrian client ***/
/* sname is the TRAFFIC LIGHT server's name:
 * Local:  "Pedestrian_To_L1"
 * Remote: "/net/<L1-hostname>/dev/name/local/Pedestrian_To_L1"
 *
 * Retains your existing repeated-send behaviour.
 * The remote server receives client_data and replies with my_reply.
 */
int client(const char *sname, PedstrianLight *light)
{
    client_data msg = {0};
    my_reply reply = {0};

    int server_coid;
    int status = EXIT_SUCCESS;

    msg.ClientID = 800;
    msg.hdr.type = DATA_TYPE;
    msg.hdr.subtype = 0; /* Button-data message to the traffic light. */

    printf("Trying to connect to: %s\n", sname);

    if ((server_coid = name_open(sname, 0)) == -1) {
        perror("name_open");
        return EXIT_FAILURE;
    }

    printf("Connection established to: %s\n", sname);

    while (1) {
        pthread_mutex_lock(&light_mutex);
        msg.LeftNorthSouth = light->LeftNorthSouth;
        msg.RightNorthSouth = light->RightNorthSouth;
        msg.TopEastWest = light->TopEastWest;
        msg.BottomEastWest = light->BottomEastWest;
        pthread_mutex_unlock(&light_mutex);

        /* Do not hold the mutex while waiting for the reply. */
        reply = (my_reply){0};

        if (MsgSend(server_coid, &msg, sizeof(msg),
                    &reply, sizeof(reply)) == -1) {
            perror("MsgSend");
            status = EXIT_FAILURE;
            break;
        }

        printf("Reply: %.*s\n", BUF_SIZE, reply.buf);
    }

    name_close(server_coid);
    return status;
}
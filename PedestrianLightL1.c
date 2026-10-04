#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include <sys/dispatch.h>
#include <sys/neutrino.h>
#include <sys/iomsg.h>
pthread_mutex_t light_mutex = PTHREAD_MUTEX_INITIALIZER;
/*-------------------------------------------------------------------------------
    Pedestrian Logic Structures 
--------------------------------------------------------------------------------*/
typedef struct {
    int LeftNorthSouthButton;
    int LeftSouthNorthButton;
    int RightNorthSouthButton;
    int RightSouthNorthButton;
    int TopEastWestButton;
    int TopWestEastButton;
    int BottomEastWestButton;
    int BottomWestEastButton;
} PedstrianButton;
typedef enum {
    PED_NONE = 0,
    PED_LNS = 1,
    PED_RNS = 2,
    PED_LNS_RNS = 3,
    PED_TEW = 4,
    PED_LNS_TEW = 5,
    PED_RNS_TEW = 6,
    PED_LNS_RNS_TEW = 7,
    PED_BEW = 8,
    PED_LNS_BEW = 9,
    PED_RNS_BEW = 10,
    PED_LNS_RNS_BEW = 11,
    PED_TEW_BEW = 12,
    PED_LNS_TEW_BEW = 13,
    PED_RNS_TEW_BEW = 14,
    PED_LNS_RNS_TEW_BEW = 15
} PedestrianCombination;
typedef enum {
    TRAFFIC_GREEN,
    TRAFFIC_RED_Flash,
    TRAFFIC_RED
} PedestrianState;
typedef struct {
    PedestrianCombination currentState;
    PedestrianState lightState;
    /* Button requests are separate from the light outputs. */
    int LeftNorthSouth;
    int RightNorthSouth;
    int TopEastWest;
    int BottomEastWest;
    int LeftNorthSouthLight;
    int RightNorthSouthLight;
    int TopEastWestLight;
    int BottomEastWestLight;
    int timer;
    int period;
    int stateChange;
    int phaseActive; /* Retained from your struct; unused by this bare logic. */
    PedstrianButton button;
} PedstrianLight;
void ReadExternalButtons(PedstrianButton *buttons);
void CheckPedestrianButtons(PedstrianLight *light, const PedstrianButton *buttons, PedstrianButton *previous);
void PedestrianStateOutput(PedestrianCombination state, PedstrianLight *light);
void *button_checker(void *state_ptr);
/*------------------------------------------------------------------------------
    Structure Used for both Client and Server Communication
*-----------------------------------------------------------------------------*/
#define BUF_SIZE 100
#define DATA_TYPE 0x22
typedef union {
    uint32_t sival_int;
    uint32_t dummy[4];
} _mysigval;
typedef struct _Mypulse {
   uint16_t type;
   uint16_t subtype;
   int8_t code;
   uint8_t zero[3];         // Same padding that is used in standard _pulse struct
   _mysigval value;
   uint8_t zero2[2];        // Extra padding to ensure alignment access.
   int32_t scoid;
} msg_header_t;
typedef char check_header[(sizeof(msg_header_t) == 32) ? 1 : -1];
/*------------------------------------------------------------------------------
    Client Structure: Pedestrian Light Control System
*-----------------------------------------------------------------------------*/
#define QNET_ATTACH_POINT_CLIENT  "/net/VM_x86_Target01/dev/name/local/myname" 
#define SET_PEDESTRIAN_STATE  1
#define REQUEST_BUTTON_DATA  2
typedef struct {
    msg_header_t hdr;
    int32_t ClientID;
    int32_t LeftNorthSouth;
    int32_t RightNorthSouth;
    int32_t TopEastWest;
    int32_t BottomEastWest;
} client_data;
typedef struct {
    msg_header_t hdr;
    char buf[BUF_SIZE];
} client_reply;
typedef char check_client_data[(sizeof(client_data) == 52 &&offsetof(client_data, LeftNorthSouth) == 36) ? 1 : -1];
/*------------------------------------------------------------------------------
    Server Structure: Pedestrian Light Control System
*-----------------------------------------------------------------------------*/
#define ATTACH_POINT "myname"
typedef struct {
    msg_header_t hdr;
    int32_t ClientID;
    int32_t trafficstate;
    int32_t time;
    int32_t period;
} server_data;
typedef struct {
    msg_header_t hdr;
    char buf[BUF_SIZE];
} my_reply;
typedef char check_server_data[(sizeof(server_data) == 48 && offsetof(server_data, trafficstate) == 36) ? 1 : -1];
/* Native pulses MUST be decoded with struct _pulse, not msg_header_t.
 * The kernel's pulse layout is independent of our application protocol. */
typedef union {
    struct _pulse pulse;
    server_data data;
} receive_buffer;
/*------------------------------------------------------------------------------
    Main Function: Pedestrian Light Control System
*-----------------------------------------------------------------------------*/
int main(void)
{
    PedstrianLight light = {
        .currentState = PED_LNS, /* Example crossing combination. */
        .lightState = TRAFFIC_RED,
        .timer = 10,
        .period = 1
    };
    PedestrianStateOutput(light.currentState, &light);
    /* Final 10% of the steps use the flashing-red state. */
    int flashStart = light.timer - (light.timer + 9) / 10;
    for (int i = 0; i < light.timer; i++) {
        if (i < flashStart) {
            light.lightState = TRAFFIC_GREEN;
        } else {
            light.lightState = TRAFFIC_RED_Flash;
        }
        sleep(light.period);
    }
    light.lightState = TRAFFIC_RED;
    return EXIT_SUCCESS;
}
/*------------------------------------------------------------------------------
    Pedestrian Light Server receiving data from the client
*-----------------------------------------------------------------------------*/

int server(PedstrianLight *light) {
    name_attach_t *attach;
    server_data msg = {0};
    my_reply replymsg = {0};

    /* Receive either a native QNX pulse or an application message.
     * Copy application messages into msg to keep the teacher's
     * msg.hdr / msg.trafficstate access style.
     */
    union {
        struct _pulse pulse;
        server_data message;
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

/*------------------------------------------------------------------------------
    Pedestrian Light Client sending data to the server
*-----------------------------------------------------------------------------*/

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

/*------------------------------------------------------------------------------
    Pedestrian Light Control System Logic
*-----------------------------------------------------------------------------*/
/* Placeholder required by your existing button_checker; replace with GPIO later. */
void ReadExternalButtons(PedstrianButton *buttons)
{
    *buttons = (PedstrianButton){0};
}
void CheckPedestrianButtons(PedstrianLight *light, const PedstrianButton *buttons,
                            PedstrianButton *previous) {
    // previous belongs to the single input worker; keep it between calls.
    pthread_mutex_lock(&light_mutex);
    light->button = *buttons;
    if ((light->button.LeftNorthSouthButton == 1 && previous->LeftNorthSouthButton == 0) ||
        (light->button.LeftSouthNorthButton == 1 && previous->LeftSouthNorthButton == 0)) {
        light->LeftNorthSouth = 1;
    }
    if ((light->button.RightNorthSouthButton == 1 && previous->RightNorthSouthButton == 0) ||
        (light->button.RightSouthNorthButton == 1 && previous->RightSouthNorthButton == 0)) {
        light->RightNorthSouth = 1;
    }
    if ((light->button.TopEastWestButton == 1 && previous->TopEastWestButton == 0) ||
        (light->button.TopWestEastButton == 1 && previous->TopWestEastButton == 0)) {
        light->TopEastWest = 1;
    }
    if ((light->button.BottomEastWestButton == 1 && previous->BottomEastWestButton == 0) ||
        (light->button.BottomWestEastButton == 1 && previous->BottomWestEastButton == 0)) {
        light->BottomEastWest = 1;
    }
    *previous = light->button;
    pthread_mutex_unlock(&light_mutex);
}
void *button_checker(void *state_ptr) {
    PedstrianLight *light = state_ptr;
    PedstrianButton previous = {0};
    while (1) {
        PedstrianButton buttons;
        ReadExternalButtons(&buttons); // Hardware work is OUTSIDE the mutex.
        CheckPedestrianButtons(light, &buttons, &previous);
        usleep(5000); // Same 5 ms polling interval as original.
    }
    return NULL;
}
/* Combination selects crossings; lightState separately specifies their colour.
 * Physical flashing/output is not implemented in this logic-only example.
 */
void PedestrianStateOutput(PedestrianCombination state, PedstrianLight *light)
{
    light->LeftNorthSouthLight = 0;
    light->RightNorthSouthLight = 0;
    light->TopEastWestLight = 0;
    light->BottomEastWestLight = 0;
    switch (state) {
        case PED_LNS:
            light->LeftNorthSouthLight = 1;
            break;
        case PED_RNS:
            light->RightNorthSouthLight = 1;
            break;
        case PED_LNS_RNS:
            light->LeftNorthSouthLight = 1;
            light->RightNorthSouthLight = 1;
            break;
        case PED_TEW:
            light->TopEastWestLight = 1;
            break;
        case PED_LNS_TEW:
            light->LeftNorthSouthLight = 1;
            light->TopEastWestLight = 1;
            break;
        case PED_RNS_TEW:
            light->RightNorthSouthLight = 1;
            light->TopEastWestLight = 1;
            break;
        case PED_LNS_RNS_TEW:
            light->LeftNorthSouthLight = 1;
            light->RightNorthSouthLight = 1;
            light->TopEastWestLight = 1;
            break;
        case PED_BEW:
            light->BottomEastWestLight = 1;
            break;
        case PED_LNS_BEW:
            light->LeftNorthSouthLight = 1;
            light->BottomEastWestLight = 1;
            break;
        case PED_RNS_BEW:
            light->RightNorthSouthLight = 1;
            light->BottomEastWestLight = 1;
            break;
        case PED_LNS_RNS_BEW:
            light->LeftNorthSouthLight = 1;
            light->RightNorthSouthLight = 1;
            light->BottomEastWestLight = 1;
            break;
        case PED_TEW_BEW:
            light->TopEastWestLight = 1;
            light->BottomEastWestLight = 1;
            break;
        case PED_LNS_TEW_BEW:
            light->LeftNorthSouthLight = 1;
            light->TopEastWestLight = 1;
            light->BottomEastWestLight = 1;
            break;
        case PED_RNS_TEW_BEW:
            light->RightNorthSouthLight = 1;
            light->TopEastWestLight = 1;
            light->BottomEastWestLight = 1;
            break;
        case PED_LNS_RNS_TEW_BEW:
            light->LeftNorthSouthLight = 1;
            light->RightNorthSouthLight = 1;
            light->TopEastWestLight = 1;
            light->BottomEastWestLight = 1;
            break;
        case PED_NONE:
        default:
            break;
    }
}


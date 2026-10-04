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

//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Global Mutex for Pedestrian Light Node
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------

pthread_mutex_t light_mutex = PTHREAD_MUTEX_INITIALIZER;

//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Pedestrian Logic Structures 
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Structure Used for both Client and Server Communication
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------

#define PEDESTRIAN_ATTACH_POINT "Pedestrian"
#define PEDESTRIAN_DATA_TYPE 0x22
#define PEDESTRIAN_BUF_SIZE 100

#define PEDESTRIAN_SET_STATE    1
#define PEDESTRIAN_REQUEST_DATA 2

typedef union {
    uint32_t sival_int;
    uint32_t dummy[4];
} Pedestrian_Sigval;

typedef struct {
    uint16_t type;
    uint16_t subtype;
    int8_t code;
    uint8_t zero[3];
    Pedestrian_Sigval value;
    uint8_t zero2[2];
    int32_t scoid;
} Pedestrian_MessageHeader;

//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Server Structure: Pedestrian Light Control System
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------

/* Reply to PEDESTRIAN_SET_STATE. */
typedef struct {
    Pedestrian_MessageHeader hdr;
    char buf[PEDESTRIAN_BUF_SIZE];
} Pedestrian_State_Reply;

/* Reply to PEDESTRIAN_REQUEST_DATA. */
typedef struct {
    Pedestrian_MessageHeader hdr;
    ButtonPresses buttons;
} Pedestrian_Button_Reply;

//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Main Function: Pedestrian Light Control System
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------

int main(void) {
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

//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Server Code — Pedestrian Node Only
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------

int Pedestrian_Server(PedstrianLight *light, pthread_mutex_t *mutex)
{
    name_attach_t *attach;
    Pedestrian_Client_data msg = {0};

    union {
        struct _pulse pulse;
        Pedestrian_Client_data message;
    } received;

    int rcvid;
    int status = EXIT_SUCCESS;

    if ((attach = name_attach(NULL,
                              PEDESTRIAN_ATTACH_POINT, 0)) == NULL) {
        perror("name_attach");
        return EXIT_FAILURE;
    }

    printf("Pedestrian server listening on: %s\n",
           PEDESTRIAN_ATTACH_POINT);

    while (1) {
        memset(&received, 0, sizeof(received));

        rcvid = MsgReceive(attach->chid, &received,
                           sizeof(received), NULL);

        if (rcvid == -1) {
            perror("MsgReceive");
            status = EXIT_FAILURE;
            break;
        }

        /* Native QNX pulse. */
        if (rcvid == 0) {
            switch (received.pulse.code) {
                case _PULSE_CODE_DISCONNECT:
                    ConnectDetach(received.pulse.scoid);
                    break;

                case _PULSE_CODE_UNBLOCK:
                case _PULSE_CODE_COIDDEATH:
                case _PULSE_CODE_THREADDEATH:
                default:
                    break;
            }

            /* Keep listening after a client disconnects. */
            continue;
        }

        msg = received.message;

        if (msg.hdr.type == _IO_CONNECT) {
            MsgReply(rcvid, EOK, NULL, 0);
            continue;
        }

        if (msg.hdr.type >= _IO_BASE &&
            msg.hdr.type <= _IO_MAX) {
            MsgError(rcvid, ENOSYS);
            continue;
        }

        if (msg.hdr.type != PEDESTRIAN_DATA_TYPE) {
            MsgError(rcvid, ENOSYS);
            continue;
        }

        switch (msg.hdr.subtype) {
            case PEDESTRIAN_SET_STATE: {
                Pedestrian_State_Reply reply = {0};

                pthread_mutex_lock(mutex);
                light->currentState = msg.pedestrianCombination;
                light->timer = msg.settings.time;
                light->period = msg.settings.peroid;
                pthread_mutex_unlock(mutex);

                reply.hdr.type = PEDESTRIAN_DATA_TYPE;
                reply.hdr.subtype = PEDESTRIAN_SET_STATE;

                snprintf(reply.buf, sizeof(reply.buf),
                         "Pedestrian state received");

                /* Acknowledges the data update, not physical GPIO output. */
                MsgReply(rcvid, EOK, &reply, sizeof(reply));
                break;
            }

            case PEDESTRIAN_REQUEST_DATA: {
                Pedestrian_Button_Reply reply = {0};

                reply.hdr.type = PEDESTRIAN_DATA_TYPE;
                reply.hdr.subtype = PEDESTRIAN_REQUEST_DATA;

                pthread_mutex_lock(mutex);
                reply.buttons.Left_NS = light->LeftNorthSouth;
                reply.buttons.Right_NS = light->RightNorthSouth;
                reply.buttons.Top_EW = light->TopEastWest;
                reply.buttons.Bottom_EW = light->BottomEastWest;
                pthread_mutex_unlock(mutex);

                /* Return button values without changing the light state. */
                MsgReply(rcvid, EOK, &reply, sizeof(reply));
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

//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Pedestrian Light Control System Logic
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------

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


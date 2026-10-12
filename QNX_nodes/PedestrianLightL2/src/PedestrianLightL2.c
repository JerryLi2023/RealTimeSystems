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
/* Main sleeps here until the server receives a state, or a tick expires. */
pthread_cond_t pedestrian_state_cond;
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
    int phaseActive; /* 1 while the commanded pedestrian sequence is running. */
    PedstrianButton button;
} PedstrianLight;
void ReadExternalButtons(PedstrianButton *buttons);
void CheckPedestrianButtons(PedstrianLight *light, const PedstrianButton *buttons, PedstrianButton *previous);
void PedestrianStateOutput(PedestrianCombination state, PedstrianLight *light);
void *button_checker(void *state_ptr);
int Pedestrian_Server(PedstrianLight *light, pthread_mutex_t *mutex);
static void *PedestrianServerThread(void *state_ptr);
//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Structure Used for both Client and Server Communication
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------
#define PEDESTRIAN_ATTACH_POINT "Pedestrian_L2"
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
/* Same definitions and field order as the traffic-light client. */
typedef struct {
    int time;
    int peroid; /* Keep the client's spelling on the wire. */
} Settings;
typedef struct {
    int Left_NS, Right_NS;
    int Top_EW, Bottom_EW;
} ButtonPresses;
typedef struct {
    Pedestrian_MessageHeader hdr;
    int32_t ClientID;
    PedestrianCombination pedestrianCombination;
    Settings settings;
} Pedestrian_Client_data;
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
static void *PedestrianServerThread(void *state_ptr)
{
    PedstrianLight *light = state_ptr;
    for (;;) {
        Pedestrian_Server(light, &light_mutex);
        sleep(2); /* Retry if attaching or receiving failed. */
    }
    return NULL;
}
int main(void)
{
    PedstrianLight light = {
        .currentState = PED_NONE,
        .lightState = TRAFFIC_RED,
        .period = 1
    };
    pthread_t server_thread, button_thread;
    pthread_condattr_t attr;
    int error;
    // Creates the thread a
    error = pthread_condattr_init(&attr);
    if (error != 0) {
        fprintf(stderr, "pthread_condattr_init: %s\n", strerror(error));
        return EXIT_FAILURE;
    }
    error = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    if (error == 0)
        error = pthread_cond_init(&pedestrian_state_cond, &attr);
    pthread_condattr_destroy(&attr);
    if (error != 0) {
        fprintf(stderr, "condition variable initialization: %s\n", strerror(error));
        return EXIT_FAILURE;
    }
    error = pthread_create(&server_thread, NULL, PedestrianServerThread, &light);
    if (error != 0) {
        fprintf(stderr, "pthread_create server: %s\n", strerror(error));
        return EXIT_FAILURE;
    }
    error = pthread_create(&button_thread, NULL, button_checker, &light);
    if (error != 0) {
        fprintf(stderr, "pthread_create buttons: %s\n", strerror(error));
        return EXIT_FAILURE;
    }
    pthread_mutex_lock(&light_mutex);
    for (;;) {
        /* Stay red until the traffic-light client sends a command. */
        while (!light.stateChange)
            pthread_cond_wait(&pedestrian_state_cond, &light_mutex);
        light.stateChange = 0;
        int steps = light.timer;
        int period = light.period;
        if (light.currentState == PED_NONE)
            continue; /* Server has already applied all-red. */
        int flashStart = steps - (steps / 10 + (steps % 10 != 0));
        struct timespec deadline;
        if (clock_gettime(CLOCK_MONOTONIC, &deadline) == -1) {
            perror("clock_gettime");
            pthread_mutex_unlock(&light_mutex);
            return EXIT_FAILURE;
        }
        for (int i = 0; i < steps && !light.stateChange; ++i) {
            light.lightState = i < flashStart ? TRAFFIC_GREEN : TRAFFIC_RED_Flash;
            deadline.tv_sec += period;
            /* Releases the mutex while waiting. New commands interrupt the tick.
             * Keep the same deadline on spurious wakes; do not restart the tick. */
            while (!light.stateChange) {
                error = pthread_cond_timedwait(&pedestrian_state_cond,
                                               &light_mutex, &deadline);
                if (error == ETIMEDOUT)
                    break;
                if (error != 0) {
                    fprintf(stderr, "pthread_cond_timedwait: %s\n", strerror(error));
                    pthread_mutex_unlock(&light_mutex);
                    return EXIT_FAILURE;
                }
            }
        }
        /* Never overwrite a newer state applied by the server. */
        if (!light.stateChange) {
            light.lightState = TRAFFIC_RED;
            light.phaseActive = 0;
            PedestrianStateOutput(PED_NONE, &light);
        }
        /* No automatic restart. Wait for the next traffic-light command. */
    }
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
                /* Only reject values that cannot describe a runnable cycle. */
                if (msg.pedestrianCombination < PED_NONE ||
                    msg.pedestrianCombination > PED_LNS_RNS_TEW_BEW ||
                    (msg.pedestrianCombination != PED_NONE &&
                     (msg.settings.time < 2 || msg.settings.peroid < 1))) {
                    MsgError(rcvid, EINVAL);
                    break;
                }
                pthread_mutex_lock(mutex);
                light->currentState = msg.pedestrianCombination;
                light->timer = msg.settings.time;
                light->period = msg.settings.peroid;
                /* Apply the command to the software outputs before acknowledging.
                 * PED_NONE stops the current cycle; other states replace it. */
                PedestrianStateOutput(light->currentState, light);
                light->phaseActive = light->currentState != PED_NONE;
                light->lightState = light->phaseActive ? TRAFFIC_GREEN : TRAFFIC_RED;
                light->stateChange = 1;
                /* Clear only requests for crossings now granted green.
                 * Requests for the other crossings remain pending. */
                if (light->LeftNorthSouthLight) light->LeftNorthSouth = 0;
                if (light->RightNorthSouthLight) light->RightNorthSouth = 0;
                if (light->TopEastWestLight) light->TopEastWest = 0;
                if (light->BottomEastWestLight) light->BottomEastWest = 0;
                pthread_cond_signal(&pedestrian_state_cond);
                pthread_mutex_unlock(mutex);
                reply.hdr.type = PEDESTRIAN_DATA_TYPE;
                reply.hdr.subtype = PEDESTRIAN_SET_STATE;
                snprintf(reply.buf, sizeof(reply.buf),
                         "Pedestrian state received");
                /* Software state is applied; the timed cycle need not finish to reply. */
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
void ReadExternalButtons(PedstrianButton *buttons) {
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

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

#define BUF_SIZE 100
#define DATA_TYPE 0x22

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
    int LeftNorthSouth;
    int RightNorthSouth;
    int TopEastWest;
    int BottomEastWest;
    PedestrianCombination currentState;
    PedestrianState lightState;
    int LeftNorthSouthLight;
    int RightNorthSouthLight;
    int TopEastWestLight;
    int BottomEastWestLight;
    int timer;
    int peroid;
    int stateChange;
    int phaseActive; /* Retained from your struct; unused by this bare logic. */
    PedstrianButton button;
} PedstrianLight;

void ReadExternalButtons(PedstrianButton *buttons);
void CheckPedestrianButtons(PedstrianLight *light, const PedstrianButton *buttons,
                            PedstrianButton *previous);
void PedestrianStateOutput(PedestrianCombination state, PedstrianLight *light);
void *button_checker(void *state_ptr);


/*------------------------------------------------------------------------------

    Client Structure: Pedestrian Light Control System

*-----------------------------------------------------------------------------*/
typedef union {
    uint32_t sival_int;
    uint32_t dummy[4];
} _mysigval;
typedef struct {
    uint16_t type, subtype;
    int8_t code;
    uint8_t zero[3];
    _mysigval value;
    uint8_t zero2[2];
    int32_t scoid;
} msg_header_t;
typedef struct {
    msg_header_t hdr;
    int32_t ClientID;
    int32_t LeftNorthSouth;
    int32_t RightNorthSouth;
    int32_t TopEastWest;
    int32_t BottomEastWest;
} my_data;
typedef struct {
    msg_header_t hdr;
    char buf[BUF_SIZE];
} my_reply;

/* A negative array bound stops compilation on an incompatible layout. */
typedef char check_header[(sizeof(msg_header_t) == 32) ? 1 : -1];
typedef char check_data[(sizeof(my_data) == 52 &&offsetof(my_data, LeftNorthSouth) == 36) ? 1 : -1];
typedef char check_reply[(sizeof(my_reply) == 132 && offsetof(my_reply, buf) == 32) ? 1 : -1];




int main(void)
{
    PedstrianLight light = {
        .currentState = PED_LNS, /* Example crossing combination. */
        .lightState = TRAFFIC_RED,
        .timer = 10,
        .peroid = 1
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
        sleep(light.peroid);
    }

    light.lightState = TRAFFIC_RED;
    return EXIT_SUCCESS;
}


int SendNamedData(int server_coid, int32_t ClientID,const PedstrianLight *light, my_reply *reply){
    my_data msg = {0};
    *reply = (my_reply){0};
    msg.hdr.type = DATA_TYPE;
    msg.ClientID = ClientID;
    // Copy the requests while the button thread cannot change them.
    pthread_mutex_lock(&light_mutex);
    msg.LeftNorthSouth = light->LeftNorthSouth;
    msg.RightNorthSouth = light->RightNorthSouth;
    msg.TopEastWest = light->TopEastWest;
    msg.BottomEastWest = light->BottomEastWest;
    pthread_mutex_unlock(&light_mutex);
    // The mutex is released before waiting for the server.
    return MsgSend(server_coid, &msg, sizeof(msg),reply, sizeof(*reply));
}

int SendNamedDataInititate(const char *sname, int argc, char **argv, PedstrianLight *light) {
    // Command-line name overrides the supplied default name.
    const char *server_name = argc > 1 ? argv[1] : sname;
    int server_coid = name_open(server_name, 0);
    int status = 0;
    my_reply reply;
    if (server_coid == -1) {
        perror("name_open");
        return 1;
    }
    while (1) {
        // Pass the pointer, not *light.
        if (SendNamedData(server_coid, 800, light, &reply) == -1) {
            perror("MsgSend");
            status = 1;
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

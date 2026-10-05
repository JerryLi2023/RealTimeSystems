#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/dispatch.h>
#include <sys/neutrino.h>
#include <sys/iomsg.h>


// Logic  Definitions for the traffic light controller node. This file contains the main logic for controlling the traffic lights at two intersections (L1 and L2) based on input from sensors and settings.
// Logic Structures and enumerations for the traffic light controller node. This file defines the data structures and enumerations used in the traffic light controller logic, including settings, movements, route indices, intersections, and states.

typedef enum {
    // North_South
    STATE_NS_SN_Left_NS_Right_NS = 0,
    STATE_NS_SN_NE_Left_NS = 1,
    STATE_NS_SN_SW_Right_NS = 2,
    STATE_NS_SN_NE_SW = 3,
    // West_South
    STATE_NE_SW_WS_WN = 4,
    STATE_WS_SW_WN_WE = 5,
    STATE_SW_WS_WN_Right_NS = 6,
    STATE_SW_WS_WE_Top_EW = 7,
    STATE_SW_WS_Top_EW_Right_NS = 8,
    // North_West
    STATE_NW_WN_NS_NE = 9,
    STATE_NW_WN_NE_ES = 10,
    STATE_NW_WN_NE_Bottom_EW = 11,
    STATE_NW_WN_NS_Right_NS = 12,
    STATE_NW_WN_Bottom_EW_Right_NS = 13,
    // East_West
    STATE_EW_WE_WN_ES = 14,
    STATE_EW_WE_WN_Bottom_EW = 15,
    STATE_EW_WE_ES_Top_EW = 16,
    STATE_EW_WE_Top_EW_Bottom_EW = 17,
    // East_North
    STATE_NE_ES_EN_SW = 18,
    STATE_NE_ES_EN_EW = 19,
    STATE_NE_ES_EN_Bottom_EW = 20,
    STATE_NE_EW_EN_Bottom_EW = 21,
    STATE_NE_Top_EW_EN_Bottom_EW = 22,
    // South_East
    STATE_ES_SE_SW_WN = 23,
    STATE_ES_SE_SW_SN = 24,
    STATE_ES_SE_SW_Bottom_EW = 25,
    STATE_ES_SE_SN_Bottom_EW = 26,
    STATE_ES_SE_Top_EW_Bottom_EW = 27,
    // No movements permitted
    STATE_ALL_RED = 28,
    /* Already returned by your selector; missing from the original enum. */
    STATE_NE_ES_EN_Left_NS = 29,
    STATE_NE_EN_Bottom_EW_Left_NS = 30,
    STATE_ES_SE_SW_Top_EW = 31,
    STATE_ES_SE_SN_Left_NS = 32,
    STATE_ES_SE_Top_EW_Left_NS = 33
} TrafficState;

typedef struct {
    int NE, NS, NW;
    int EN, ES, EW;
    int SN, SE, SW;
    int WN, WE, WS;
} CurrentCars;
typedef struct {
    int Left_NS, Right_NS;
    int Top_EW, Bottom_EW;
} ButtonPresses;

#define MAX_PRIORITY (INT_MAX / 8)
typedef struct {
    int time;
    int peroid;
    int train_detected; // Flag to indicate if a train is detected
    int hardware_error; // Flag to indicate if there is a hardware error
} Settings;
typedef struct {
    int NE, NS, NW;
    int EN, ES, EW;
    int SN, SE, SW;
    int WN, WE, WS;
    int Left_NS, Right_NS;
    int Top_EW, Bottom_EW;
} Movements;
typedef struct {
    int North_South;
    int West_South;
    int North_West;
    int East_West;
    int East_North;
    int South_East;
} RouteIndices;
typedef struct {
    Movements priority;
    Movements input;
    Movements output;
    Settings settings;
    int train_detected; // Flag to indicate if a train is detected
    int hardware_error; // Flag to indicate if there is a hardware error
    // Scores for every variant in each route group.
    int North_South[4];
    int West_South[5];
    int North_West[5];
    int East_West[4];
    int East_North[5];
    int South_East[5];
    TrafficState statesL1;
    TrafficState statesL2;
    RouteIndices best; // Highest-scoring variant index for each group.
} Intersection;
enum states {L1_NS_and_L2_NS, L1_EW_and_L2_EW, L1_NW_and_L2_SE, L1_WS_and_L2_EN, L1_EN_and_L2_NW, L1_SE_and_L2_EW, L1_SW_and_L2_WS, L1_WE_and_L2_NW, L1_WE_and_L2_WS, L1_EN_and_L2_EW, CONTROLLER_HOLD};
enum states CurState = CONTROLLER_HOLD;
enum states RequestedState = CONTROLLER_HOLD;

void Find_Maximum_Index(const int *array, int size, int *max_index);
void Calculate_Route_Scores(Intersection *light);
enum states TrafficLight_Logics(Intersection *L1, Intersection *L2);
void TrafficLight_State_Machine(void *state_ptr, void *L1_data, void *L2_data);
void Reset_Traffic_Light_Outputs(Intersection *light);
void North_South_route_Case_Statement(int route_index, Intersection *light);
void West_South_route_East_North_route_Case_Statement(int route_index, Intersection *light);
void North_West_route_and_West_north_route_Case_Statement(int route_index, Intersection *light);
void East_West_route_Case_Statement(int route_index, Intersection *light);
void East_to_North_route_Case_Statement(int route_index, Intersection *light);
void South_to_East_route_Case_Statement(int route_index, Intersection *light);
static void Register_Inputs(Intersection *light);
void Update_Waiting_Priorities(Intersection *light);
static void Update_One_Priority(int *priority, int pending);
void StateMachine(void *L1_data, void *L2_data, void *settings_data);

//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Universal Train Client/Server Structures
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------
typedef enum {
    TRAIN_NOT_PRESENT = 0,
    TRAIN_PRESENT = 1,
    TRAIN_ERROR = 2
} TrainState;

#define TRAIN_DATA_TYPE     0x23
#define TRAIN_STATUS_UPDATE 1
#define TRAIN_BUF_SIZE      100

#define TRAIN_TRAFFIC_ATTACH_POINT    "Train_To_Traffic"
#define TRAIN_CONTROLLER_ATTACH_POINT "Train_To_Controller"

typedef union {
    uint32_t sival_int;
    uint32_t dummy[4];
} Train_Sigval;
typedef struct {
    uint16_t type;
    uint16_t subtype;
    int8_t code;
    uint8_t zero[3];
    Train_Sigval value;
    uint8_t zero2[2];
    int32_t scoid;
} Train_MessageHeader;

//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Server Structure — Train Status Returned to Client
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------

typedef struct {
    Train_MessageHeader hdr;
    int32_t ClientID;
    int32_t trainState;
} Train_Client_data;
typedef struct {
    Train_MessageHeader hdr;
    char buf[TRAIN_BUF_SIZE];
} Train_Server_Reply;

//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Universal Client/Server Structures — Shared by Traffic Light and Controller
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------

#define CONTROLLER_L1_ATTACH_POINT "L1_To_Controller"
#define CONTROLLER_L1_DATA_TYPE    0x24
#define CONTROLLER_L1_BUF_SIZE     100

#define CONTROLLER_L1_UPDATE_DATA   1
#define CONTROLLER_L1_REQUEST_STATE 2
typedef union {
    uint32_t sival_int;
    uint32_t dummy[4];
} Controller_L1_Sigval;

typedef struct {
    uint16_t type;
    uint16_t subtype;
    int8_t code;
    uint8_t zero[3];
    Controller_L1_Sigval value;
    uint8_t zero2[2];
    int32_t scoid;
} Controller_L1_MessageHeader;

typedef struct {
    Controller_L1_MessageHeader hdr;
    int32_t ClientID;
    CurrentCars cars;
    ButtonPresses buttons;
} Controller_L1_Message;

typedef struct {
    Controller_L1_MessageHeader hdr;
    char buf[CONTROLLER_L1_BUF_SIZE];
} Controller_L1_Acknowledgement;

typedef struct {
    Controller_L1_MessageHeader hdr;
    int32_t state; /* Carries a TrafficState value. */
} Controller_L1_StateReply;

//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Main Function: Controller Logic Node
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------

int main(void) {
    printf("Controller logic running\n");

    Intersection L1 = {0};
    Intersection L2 = {0};
    Settings settings = {
        .time = 5,
        .peroid = 1,       // Original field spelling retained.
        .train_detected = 0, // Manual test: no train present.
        .hardware_error = 0
    };

    // MANUAL TEST INPUTS: uncomment/change these before running.
    // L1.input.NS = 1;
    // L1.input.SN = 1;
    // L2.input.NS = 1;
    // L2.input.SN = 1;
    // With all inputs zero, the controller selects HOLD and outputs stay zero.

    while (1) {
        // Put any single-threaded input updates here, before calculating.
        StateMachine(&L1, &L2, &settings);

        // Results are already saved in these existing variables:
        // CurState  : selected combined route (or CONTROLLER_HOLD).
        // L1.best / L2.best : best variant index within each route group.
        // L1.output / L2.output : selected movements, each set to 0 or 1.
        // Inspect them in the debugger or use them here.

        usleep(100000); // Recheck every 100 ms; priorities age separately.
    }

    return EXIT_SUCCESS;
}


//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Train-to-Controller Server — Runs on Controller Node
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------

int Train_Controller_Server(TrainState *trainState, pthread_mutex_t *mutex) {
    name_attach_t *attach;
    Train_Client_data msg = {0};

    union {
        struct _pulse pulse;
        Train_Client_data message;
    } received;

    int rcvid;
    int status = EXIT_SUCCESS;

    if ((attach = name_attach(NULL,
                              TRAIN_CONTROLLER_ATTACH_POINT, 0)) == NULL) {
        perror("name_attach");
        return EXIT_FAILURE;
    }

    printf("Server listening on: %s\n", TRAIN_CONTROLLER_ATTACH_POINT);

    while (1) {
        memset(&received, 0, sizeof(received));

        rcvid = MsgReceive(attach->chid, &received,
                           sizeof(received), NULL);

        if (rcvid == -1) {
            perror("MsgReceive");
            status = EXIT_FAILURE;
            break;
        }

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

        if (msg.hdr.type != TRAIN_DATA_TYPE ||
            msg.hdr.subtype != TRAIN_STATUS_UPDATE) {
            MsgError(rcvid, ENOSYS);
            continue;
        }

        /* Store the received status on the controller node. */
        pthread_mutex_lock(mutex);
        *trainState = (TrainState)msg.trainState;
        pthread_mutex_unlock(mutex);

        Train_Server_Reply reply = {0};

        reply.hdr.type = TRAIN_DATA_TYPE;
        reply.hdr.subtype = TRAIN_STATUS_UPDATE;

        snprintf(reply.buf, sizeof(reply.buf),
                 "Train status received");

        MsgReply(rcvid, EOK, &reply, sizeof(reply));
    }

    name_detach(attach, 0);
    return status;
}

//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Controller Server — Receives L1 Data and Returns statesL1
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------

int Controller_L1_Server(Intersection *intersection,
                         pthread_mutex_t *mutex)
{
    name_attach_t *attach;
    Controller_L1_Message msg = {0};

    union {
        struct _pulse pulse;
        Controller_L1_Message message;
    } received;

    int rcvid;
    int status = EXIT_SUCCESS;

    if ((attach = name_attach(NULL,
                              CONTROLLER_L1_ATTACH_POINT, 0)) == NULL) {
        perror("name_attach");
        return EXIT_FAILURE;
    }

    printf("Server listening on: %s\n", CONTROLLER_L1_ATTACH_POINT);

    while (1) {
        memset(&received, 0, sizeof(received));

        rcvid = MsgReceive(attach->chid, &received,
                           sizeof(received), NULL);

        if (rcvid == -1) {
            perror("MsgReceive");
            status = EXIT_FAILURE;
            break;
        }

        /* Handle native QNX pulses. */
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

        if (msg.hdr.type != CONTROLLER_L1_DATA_TYPE) {
            MsgError(rcvid, ENOSYS);
            continue;
        }

        switch (msg.hdr.subtype) {
            case CONTROLLER_L1_UPDATE_DATA: {
                Controller_L1_Acknowledgement reply = {0};

                pthread_mutex_lock(mutex);

                /* Copy waiting cars into the controller's input. */
                intersection->input.NE = msg.cars.NE;
                intersection->input.NS = msg.cars.NS;
                intersection->input.NW = msg.cars.NW;

                intersection->input.EN = msg.cars.EN;
                intersection->input.ES = msg.cars.ES;
                intersection->input.EW = msg.cars.EW;

                intersection->input.SN = msg.cars.SN;
                intersection->input.SE = msg.cars.SE;
                intersection->input.SW = msg.cars.SW;

                intersection->input.WN = msg.cars.WN;
                intersection->input.WE = msg.cars.WE;
                intersection->input.WS = msg.cars.WS;

                /* Copy pedestrian requests into the same input. */
                intersection->input.Left_NS = msg.buttons.Left_NS;
                intersection->input.Right_NS = msg.buttons.Right_NS;
                intersection->input.Top_EW = msg.buttons.Top_EW;
                intersection->input.Bottom_EW = msg.buttons.Bottom_EW;

                pthread_mutex_unlock(mutex);

                /* Existing controller logic calculates statesL1 separately. */
                reply.hdr.type = CONTROLLER_L1_DATA_TYPE;
                reply.hdr.subtype = CONTROLLER_L1_UPDATE_DATA;

                snprintf(reply.buf, sizeof(reply.buf),
                         "L1 cars and buttons received");

                MsgReply(rcvid, EOK, &reply, sizeof(reply));
                break;
            }

            case CONTROLLER_L1_REQUEST_STATE: {
                Controller_L1_StateReply reply = {0};

                reply.hdr.type = CONTROLLER_L1_DATA_TYPE;
                reply.hdr.subtype = CONTROLLER_L1_REQUEST_STATE;

                pthread_mutex_lock(mutex);
                reply.state = (int32_t)intersection->statesL1;
                pthread_mutex_unlock(mutex);

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

/*
#################################################################################
#################################################################################

    All logics here are only relavent to the traffic light logics

#################################################################################
#################################################################################
*/

// Called directly by main; calculates and stores outputs without sleeping.
void StateMachine(void *L1_data, void *L2_data, void *settings_data) {
    Intersection *L1 = L1_data;
    Intersection *L2 = L2_data;
    Settings *settings = settings_data;
    static uint64_t next_age_ms;
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
        CurState = RequestedState = CONTROLLER_HOLD;
        TrafficLight_State_Machine(&CurState, L1, L2);
        return;
    }
    uint64_t now_ms = (uint64_t)now.tv_sec * 1000 + (uint64_t)now.tv_nsec / 1000000;
    uint64_t interval_ms = (settings->time > 0 && settings->peroid > 0)
        ? (uint64_t)settings->time * settings->peroid * 1000 : 5000;
    if (!next_age_ms) next_age_ms = now_ms + interval_ms;
    if (now_ms >= next_age_ms) {
        Update_Waiting_Priorities(L1);
        Update_Waiting_Priorities(L2);
        next_age_ms = now_ms + interval_ms;
    }
    Register_Inputs(L1);
    Register_Inputs(L2);
    if (settings->train_detected || settings->hardware_error)
        RequestedState = CONTROLLER_HOLD;
    else
        RequestedState = TrafficLight_Logics(L1, L2);
    CurState = RequestedState;
    TrafficLight_State_Machine(&CurState, L1, L2);
}

void Reset_Traffic_Light_Outputs(Intersection *light) {
    if (light != NULL) {
        light->output = (Movements){0};
    }
}

void TrafficLight_State_Machine(void *state_ptr, void *L1_data, void *L2_data) {
    Intersection *L1 = L1_data;
    Intersection *L2 = L2_data;
    Reset_Traffic_Light_Outputs(L1);
    Reset_Traffic_Light_Outputs(L2);
    if (state_ptr == NULL) {
        return;
    }
    const enum states *CurrentState = (const enum states *)state_ptr;
    switch (*CurrentState) {
        case L1_NS_and_L2_NS:
            North_South_route_Case_Statement(L1->best.North_South, L1);
            North_South_route_Case_Statement(L2->best.North_South, L2);
            break;
        case L1_EW_and_L2_EW:
            East_West_route_Case_Statement(L1->best.East_West, L1);
            East_West_route_Case_Statement(L2->best.East_West, L2);
            break;
        case L1_NW_and_L2_SE:
            North_West_route_and_West_north_route_Case_Statement(L1->best.North_West, L1);
            South_to_East_route_Case_Statement(L2->best.South_East, L2);
            break;
        case L1_WS_and_L2_EN:
            West_South_route_East_North_route_Case_Statement(L1->best.West_South, L1);
            East_to_North_route_Case_Statement(L2->best.East_North, L2);
            break;
        case L1_EN_and_L2_NW:
            East_to_North_route_Case_Statement(L1->best.East_North, L1);
            North_West_route_and_West_north_route_Case_Statement(L2->best.North_West, L2);
            break;
        case L1_SE_and_L2_EW:
            South_to_East_route_Case_Statement(L1->best.South_East, L1);
            East_West_route_Case_Statement(L2->best.East_West, L2);
            break;
        case L1_SW_and_L2_WS:
            South_to_East_route_Case_Statement(L1->best.South_East, L1);
            West_South_route_East_North_route_Case_Statement(L2->best.West_South, L2);
            break;
        case L1_WE_and_L2_NW:
            East_West_route_Case_Statement(L1->best.East_West, L1);
            North_West_route_and_West_north_route_Case_Statement(L2->best.North_West, L2);
            break;
        case L1_WE_and_L2_WS:
            East_West_route_Case_Statement(L1->best.East_West, L1);
            West_South_route_East_North_route_Case_Statement(L2->best.West_South, L2);
            break;
        case L1_EN_and_L2_EW:
            East_to_North_route_Case_Statement(L1->best.East_North, L1);
            East_West_route_Case_Statement(L2->best.East_West, L2);
            break;
        default:
            // Unknown state: all permission outputs remain zero.
            break;
    }
}

void North_South_route_Case_Statement(int route_index, Intersection *light) {
    if (light == NULL) {
        return;
    }
    Reset_Traffic_Light_Outputs(light);
    switch (route_index) {
        case 0:
            // Enable: NS, SN, Left_NS, Right_NS.
            light->output.NS = 1;
            light->output.SN = 1;
            light->output.Left_NS = 1;
            light->output.Right_NS = 1;
            break;
        case 1:
            // Enable: NS, SN, NE, Left_NS.
            light->output.NS = 1;
            light->output.SN = 1;
            light->output.NE = 1;
            light->output.Left_NS = 1;
            break;
        case 2:
            // Enable: NS, SN, SW, Right_NS.
            light->output.NS = 1;
            light->output.SN = 1;
            light->output.SW = 1;
            light->output.Right_NS = 1;
            break;
        case 3:
            // Enable: NS, SN, NE, SW.
            light->output.NS = 1;
            light->output.SN = 1;
            light->output.NE = 1;
            light->output.SW = 1;
            break;
        default:
            // Unknown variant: this intersection remains stopped.
            break;
    }
}

void West_South_route_East_North_route_Case_Statement(int route_index, Intersection *light) {
    if (light == NULL) {
        return;
    }
    Reset_Traffic_Light_Outputs(light);
    switch (route_index) {
        case 0:
            // Enable: NE, SW, WS, WN.
            light->output.NE = 1;
            light->output.SW = 1;
            light->output.WS = 1;
            light->output.WN = 1;
            break;
        case 1:
            // Enable: WS, SW, WN, WE.
            light->output.WS = 1;
            light->output.SW = 1;
            light->output.WN = 1;
            light->output.WE = 1;
            break;
        case 2:
            // Enable: SW, WS, WN, Right_NS.
            light->output.SW = 1;
            light->output.WS = 1;
            light->output.WN = 1;
            light->output.Right_NS = 1;
            break;
        case 3:
            // Enable: SW, WS, WE, Top_EW.
            light->output.SW = 1;
            light->output.WS = 1;
            light->output.WE = 1;
            light->output.Top_EW = 1;
            break;
        case 4:
            // Enable: SW, WS, Top_EW, Right_NS.
            light->output.SW = 1;
            light->output.WS = 1;
            light->output.Top_EW = 1;
            light->output.Right_NS = 1;
            break;
        default:
            // Unknown variant: this intersection remains stopped.
            break;
    }
}

void North_West_route_and_West_north_route_Case_Statement(int route_index, Intersection *light) {
    if (light == NULL) {
        return;
    }
    Reset_Traffic_Light_Outputs(light);
    switch (route_index) {
        case 0:
            // Enable: NW, WN, NS, NE.
            light->output.NW = 1;
            light->output.WN = 1;
            light->output.NS = 1;
            light->output.NE = 1;
            break;
        case 1:
            // Enable: NW, WN, NE, ES.
            light->output.NW = 1;
            light->output.WN = 1;
            light->output.NE = 1;
            light->output.ES = 1;
            break;
        case 2:
            // Enable: NW, WN, NE, Bottom_EW.
            light->output.NW = 1;
            light->output.WN = 1;
            light->output.NE = 1;
            light->output.Bottom_EW = 1;
            break;
        case 3:
            // Enable: NW, WN, NS, Right_NS.
            light->output.NW = 1;
            light->output.WN = 1;
            light->output.NS = 1;
            light->output.Right_NS = 1;
            break;
        case 4:
            // Enable: NW, WN, Bottom_EW, Right_NS.
            light->output.NW = 1;
            light->output.WN = 1;
            light->output.Bottom_EW = 1;
            light->output.Right_NS = 1;
            break;
        default:
            // Unknown variant: this intersection remains stopped.
            break;
    }
}

void East_West_route_Case_Statement(int route_index, Intersection *light) {
    if (light == NULL) {
        return;
    }
    Reset_Traffic_Light_Outputs(light);
    switch (route_index) {
        case 0:
            // Enable: EW, WE, WN, ES.
            light->output.EW = 1;
            light->output.WE = 1;
            light->output.WN = 1;
            light->output.ES = 1;
            break;
        case 1:
            // Enable: EW, WE, WN, Bottom_EW.
            light->output.EW = 1;
            light->output.WE = 1;
            light->output.WN = 1;
            light->output.Bottom_EW = 1;
            break;
        case 2:
            // Enable: EW, WE, ES, Top_EW.
            light->output.EW = 1;
            light->output.WE = 1;
            light->output.ES = 1;
            light->output.Top_EW = 1;
            break;
        case 3:
            // Enable: EW, WE, Top_EW, Bottom_EW.
            light->output.EW = 1;
            light->output.WE = 1;
            light->output.Top_EW = 1;
            light->output.Bottom_EW = 1;
            break;
        default:
            // Unknown variant: this intersection remains stopped.
            break;
    }
}

void East_to_North_route_Case_Statement(int route_index, Intersection *light) {
    if (light == NULL) {
        return;
    }
    Reset_Traffic_Light_Outputs(light);
    switch (route_index) {
        case 0:
            // Enable: NE, ES, EN, SW.
            light->output.NE = 1;
            light->output.ES = 1;
            light->output.EN = 1;
            light->output.SW = 1;
            break;
        case 1:
            // Enable: NE, ES, EN, EW.
            light->output.NE = 1;
            light->output.ES = 1;
            light->output.EN = 1;
            light->output.EW = 1;
            break;
        case 2:
            // Enable: NE, ES, EN, Bottom_EW.
            light->output.NE = 1;
            light->output.ES = 1;
            light->output.EN = 1;
            light->output.Bottom_EW = 1;
            break;
        case 3:
            // Enable: NE, EW, EN, Bottom_EW.
            light->output.NE = 1;
            light->output.EW = 1;
            light->output.EN = 1;
            light->output.Bottom_EW = 1;
            break;
        case 4:
            // Enable: NE, Top_EW, EN, Bottom_EW.
            light->output.NE = 1;
            light->output.Top_EW = 1;
            light->output.EN = 1;
            light->output.Bottom_EW = 1;
            break;
        default:
            // Unknown variant: this intersection remains stopped.
            break;
    }
}

void South_to_East_route_Case_Statement(int route_index, Intersection *light) {
    if (light == NULL) {
        return;
    }
    Reset_Traffic_Light_Outputs(light);
    switch (route_index) {
        case 0:
            // Enable: ES, SE, SW, WN.
            light->output.ES = 1;
            light->output.SE = 1;
            light->output.SW = 1;
            light->output.WN = 1;
            break;
        case 1:
            // Enable: ES, SE, SW, SN.
            light->output.ES = 1;
            light->output.SE = 1;
            light->output.SW = 1;
            light->output.SN = 1;
            break;
        case 2:
            // Enable: ES, SE, SW, Bottom_EW.
            light->output.ES = 1;
            light->output.SE = 1;
            light->output.SW = 1;
            light->output.Bottom_EW = 1;
            break;
        case 3:
            // Enable: ES, SE, SN, Bottom_EW.
            light->output.ES = 1;
            light->output.SE = 1;
            light->output.SN = 1;
            light->output.Bottom_EW = 1;
            break;
        case 4:
            // Enable: ES, SE, Top_EW, Bottom_EW.
            light->output.ES = 1;
            light->output.SE = 1;
            light->output.Top_EW = 1;
            light->output.Bottom_EW = 1;
            break;
        default:
            // Unknown variant: this intersection remains stopped.
            break;
    }
}

void Calculate_Route_Scores(Intersection *light) {
    if (light == NULL) {
        return;
    }
    light->North_South[0] = light->priority.NS + light->priority.SN + light->priority.Left_NS + light->priority.Right_NS;
    light->North_South[1] = light->priority.NS + light->priority.SN + light->priority.NE + light->priority.Left_NS;
    light->North_South[2] = light->priority.NS + light->priority.SN + light->priority.SW + light->priority.Right_NS;
    light->North_South[3] = light->priority.NS + light->priority.SN + light->priority.NE + light->priority.SW;
    Find_Maximum_Index(light->North_South, 4, &light->best.North_South);
    light->West_South[0] = light->priority.NE + light->priority.SW + light->priority.WS + light->priority.WN;
    light->West_South[1] = light->priority.WS + light->priority.SW + light->priority.WN + light->priority.WE;
    light->West_South[2] = light->priority.SW + light->priority.WS + light->priority.WN + light->priority.Right_NS;
    light->West_South[3] = light->priority.SW + light->priority.WS + light->priority.WE + light->priority.Top_EW;
    light->West_South[4] = light->priority.SW + light->priority.WS + light->priority.Top_EW + light->priority.Right_NS;
    Find_Maximum_Index(light->West_South, 5, &light->best.West_South);
    light->North_West[0] = light->priority.NW + light->priority.WN + light->priority.NS + light->priority.NE;
    light->North_West[1] = light->priority.NW + light->priority.WN + light->priority.NE + light->priority.ES;
    light->North_West[2] = light->priority.NW + light->priority.WN + light->priority.NE + light->priority.Bottom_EW;
    light->North_West[3] = light->priority.NW + light->priority.WN + light->priority.NS + light->priority.Right_NS;
    light->North_West[4] = light->priority.NW + light->priority.WN + light->priority.Bottom_EW + light->priority.Right_NS;
    Find_Maximum_Index(light->North_West, 5, &light->best.North_West);
    light->East_West[0] = light->priority.EW + light->priority.WE + light->priority.WN + light->priority.ES;
    light->East_West[1] = light->priority.EW + light->priority.WE + light->priority.WN + light->priority.Bottom_EW;
    light->East_West[2] = light->priority.EW + light->priority.WE + light->priority.ES + light->priority.Top_EW;
    light->East_West[3] = light->priority.EW + light->priority.WE + light->priority.Top_EW + light->priority.Bottom_EW;
    Find_Maximum_Index(light->East_West, 4, &light->best.East_West);
    light->East_North[0] = light->priority.NE + light->priority.ES + light->priority.EN + light->priority.SW;
    light->East_North[1] = light->priority.NE + light->priority.ES + light->priority.EN + light->priority.EW;
    light->East_North[2] = light->priority.NE + light->priority.ES + light->priority.EN + light->priority.Bottom_EW;
    light->East_North[3] = light->priority.NE + light->priority.EW + light->priority.EN + light->priority.Bottom_EW;
    light->East_North[4] = light->priority.NE + light->priority.Top_EW + light->priority.EN + light->priority.Bottom_EW;
    Find_Maximum_Index(light->East_North, 5, &light->best.East_North);
    light->South_East[0] = light->priority.ES + light->priority.SE + light->priority.SW + light->priority.WN;
    light->South_East[1] = light->priority.ES + light->priority.SE + light->priority.SW + light->priority.SN;
    light->South_East[2] = light->priority.ES + light->priority.SE + light->priority.SW + light->priority.Bottom_EW;
    light->South_East[3] = light->priority.ES + light->priority.SE + light->priority.SN + light->priority.Bottom_EW;
    light->South_East[4] = light->priority.ES + light->priority.SE + light->priority.Top_EW + light->priority.Bottom_EW;
    Find_Maximum_Index(light->South_East, 5, &light->best.South_East);
}

enum states TrafficLight_Logics(Intersection *L1, Intersection *L2) {
    Calculate_Route_Scores(L1);
    Calculate_Route_Scores(L2);
    int TrafficRoutes[10];
    int Decided_route = 0;
    // L1 North South route + L2 North South route
    TrafficRoutes[0] = L1->North_South[L1->best.North_South] + L2->North_South[L2->best.North_South];
    // L1 East West route + L2 East West route
    TrafficRoutes[1] = L1->East_West[L1->best.East_West] + L2->East_West[L2->best.East_West];
    // L1 North West route + L2 South East route
    TrafficRoutes[2] = L1->North_West[L1->best.North_West] + L2->South_East[L2->best.South_East];
    // L1 West South route + L2 East North route
    TrafficRoutes[3] = L1->West_South[L1->best.West_South] + L2->East_North[L2->best.East_North];
    // L1 East North route + L2 North West route
    TrafficRoutes[4] = L1->East_North[L1->best.East_North] + L2->North_West[L2->best.North_West];
    // L1 South to East route + L2 East West route
    TrafficRoutes[5] = L1->South_East[L1->best.South_East] + L2->East_West[L2->best.East_West];
    // L1 South East route + L2 West South route (legacy state label uses SW)
    TrafficRoutes[6] = L1->South_East[L1->best.South_East] + L2->West_South[L2->best.West_South];
    // L1 West East route + L2 North West route
    TrafficRoutes[7] = L1->East_West[L1->best.East_West] + L2->North_West[L2->best.North_West];
    // L1 West East route + L2 West South route
    TrafficRoutes[8] = L1->East_West[L1->best.East_West] + L2->West_South[L2->best.West_South];
    // L1 East North route + L2 East West route
    TrafficRoutes[9] = L1->East_North[L1->best.East_North] + L2->East_West[L2->best.East_West];
    Find_Maximum_Index(TrafficRoutes, 10, &Decided_route);
    return TrafficRoutes[Decided_route] == 0 ? CONTROLLER_HOLD : (enum states)Decided_route;
}

void Find_Maximum_Index(const int *array, int size, int *max_index) {
    int max_value = array[0];
    *max_index = 0;
    for (int i = 1; i < size; i++) {
        if (array[i] > max_value) {
            max_value = array[i];
            *max_index = i;
        }
    }
}
// Incoming fields represent OUTSTANDING requests, held at 1 until served.
// Do not clear demand merely because a movement was selected for output.
static void Register_Inputs(Intersection *light) {
    if (!light->input.NE) light->priority.NE = 0;
    else if (light->priority.NE == 0) light->priority.NE = 1;
    if (!light->input.NS) light->priority.NS = 0;
    else if (light->priority.NS == 0) light->priority.NS = 1;
    if (!light->input.NW) light->priority.NW = 0;
    else if (light->priority.NW == 0) light->priority.NW = 1;
    if (!light->input.EN) light->priority.EN = 0;
    else if (light->priority.EN == 0) light->priority.EN = 1;
    if (!light->input.ES) light->priority.ES = 0;
    else if (light->priority.ES == 0) light->priority.ES = 1;
    if (!light->input.EW) light->priority.EW = 0;
    else if (light->priority.EW == 0) light->priority.EW = 1;
    if (!light->input.SN) light->priority.SN = 0;
    else if (light->priority.SN == 0) light->priority.SN = 1;
    if (!light->input.SE) light->priority.SE = 0;
    else if (light->priority.SE == 0) light->priority.SE = 1;
    if (!light->input.SW) light->priority.SW = 0;
    else if (light->priority.SW == 0) light->priority.SW = 1;
    if (!light->input.WN) light->priority.WN = 0;
    else if (light->priority.WN == 0) light->priority.WN = 1;
    if (!light->input.WE) light->priority.WE = 0;
    else if (light->priority.WE == 0) light->priority.WE = 1;
    if (!light->input.WS) light->priority.WS = 0;
    else if (light->priority.WS == 0) light->priority.WS = 1;
    if (!light->input.Left_NS) light->priority.Left_NS = 0;
    else if (light->priority.Left_NS == 0) light->priority.Left_NS = 1;
    if (!light->input.Right_NS) light->priority.Right_NS = 0;
    else if (light->priority.Right_NS == 0) light->priority.Right_NS = 1;
    if (!light->input.Top_EW) light->priority.Top_EW = 0;
    else if (light->priority.Top_EW == 0) light->priority.Top_EW = 1;
    if (!light->input.Bottom_EW) light->priority.Bottom_EW = 0;
    else if (light->priority.Bottom_EW == 0) light->priority.Bottom_EW = 1;
}

void Update_Waiting_Priorities(Intersection *light) {
    if (!light) return;
    Update_One_Priority(&light->priority.NE, light->input.NE);
    Update_One_Priority(&light->priority.NS, light->input.NS);
    Update_One_Priority(&light->priority.NW, light->input.NW);
    Update_One_Priority(&light->priority.EN, light->input.EN);
    Update_One_Priority(&light->priority.ES, light->input.ES);
    Update_One_Priority(&light->priority.EW, light->input.EW);
    Update_One_Priority(&light->priority.SN, light->input.SN);
    Update_One_Priority(&light->priority.SE, light->input.SE);
    Update_One_Priority(&light->priority.SW, light->input.SW);
    Update_One_Priority(&light->priority.WN, light->input.WN);
    Update_One_Priority(&light->priority.WE, light->input.WE);
    Update_One_Priority(&light->priority.WS, light->input.WS);
    Update_One_Priority(&light->priority.Left_NS, light->input.Left_NS);
    Update_One_Priority(&light->priority.Right_NS, light->input.Right_NS);
    Update_One_Priority(&light->priority.Top_EW, light->input.Top_EW);
    Update_One_Priority(&light->priority.Bottom_EW, light->input.Bottom_EW);
}

static void Update_One_Priority(int *priority, int pending) {
    if (!pending) *priority = 0;
    else if (*priority <= 0) *priority = 1;
    else if (*priority > MAX_PRIORITY / 2) *priority = MAX_PRIORITY;
    else *priority *= 2;
}

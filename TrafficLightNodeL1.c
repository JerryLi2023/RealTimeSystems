#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <sys/dispatch.h>
#include <sys/iomsg.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/neutrino.h>
pthread_cond_t pedestrian_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t controller_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t l1_l2_cond = PTHREAD_COND_INITIALIZER;
//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    State Machine Structure
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------
typedef struct {
    int time;
    int peroid;
} Settings;
typedef enum {
    TRAFFIC_GREEN,
    TRAFFIC_YELLOW,
    TRAFFIC_RED
} TrafficLightState;
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
typedef enum {
    NS = 0,
    EW = 1,
    NW = 2,
    WS = 3,
    EN = 4,
    SE = 5
} TrafficStates;
typedef struct {
    int NE, NS, NW;
    int EN, ES, EW;
    int SN, SE, SW;
    int WN, WE, WS;
    int Left_NS, Right_NS;
    int Top_EW, Bottom_EW;
} Movements;
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
typedef enum {
    TRAIN_NOT_PRESENT = 0,
    TRAIN_PRESENT = 1,
    TRAIN_ERROR = 2
} TrainState;
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
typedef struct {
    TrafficLightState trafficState;   // Shows the current light that traffic is on
    TrafficState trafficDirection;    // The Current State direction of the traffic light
    TrafficStates trafficDirectionNext; // Direction group for local cycling
    TrafficStates trainDirectionNext;          // The Next State direction of the train
    TrafficState controllerDirection; // Recieved traffic controller from the controller
    TrainState trainDirection;          // The state of the train direction
    CurrentCars storedDirection;         // Current waiting cars and pedestrians
    Movements outputL1;               // Ouput data to the traffic light L1
    Movements outputL2;               // Ouput data to the traffic light L2
    ButtonPresses buttons;
    PedestrianCombination pedestrianCombination; // The state of the pedestrian combination
    int train_detected; // Flag to indicate if a train is detected
    int stateChange;
    int pedestrianRequest; // Flag to indicate if there is a pedestrian request
    int controllerRequest;
    int L2SynconisedDataSend;
    int L2SyconisedStateChange;
    int L2StateValue;
    /* Last observed connection state, protected by light_mutex. */
    int pedestrianConnected;
    int controllerConnected;
    int L2Connected; /* Retained for compatibility; unused in this L2 program. */
    int L1Connected;
    int L1ServerReady;
    int L1DataRequest;
    int L1StartRequested;
    int L1StateValid;
    TrafficState L1Direction; /* Fallback received from L1, separate from controller. */
    int trainServerReady; /* Local name_attach succeeded. */
    int trainConnected;   /* A train status arrived; cleared on disconnect. */
    /* Reply bookkeeping: indices are the ClientLink enum below. */
    unsigned replyCount[3];
    int replyResult[3];
    int requestInFlight[3]; /* Do not reuse a link while a timed-out send is pending. */
    int requestExpired[3];
} TrafficLight;
// Global variables
Settings settings = {0};
pthread_mutex_t light_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t communication_cond = PTHREAD_COND_INITIALIZER;
// Function prototypes
TrafficState TrafficLogicNode(void *state_ptr);
TrafficState TrafficLogicNodeL2(void *state_ptr);
TrafficState TrainLogicNode(void *state_ptr);
TrafficState ControllerStateMachine(void *state_ptr, void *inputs);
TrafficState CrossCommunicationStateMachine(void *state_ptr, void *inputs);
TrafficState NoControllerStateMachine(void *state_ptr, void *inputs);
void StateOutput(TrafficState state, Movements *output);
TrafficState TrafficLogicNode(void *state_ptr);
TrafficState TrafficLogicNodeL2(void *state_ptr);
TrafficState TrainLogicNode(void *state_ptr);
void StateOutput(TrafficState state, Movements *output);
PedestrianCombination GetPedestrianCombination(const Movements *output);
//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//   Pedestrian Client/Server Structures
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
typedef struct {
    Pedestrian_MessageHeader hdr;
    int32_t ClientID;
    PedestrianCombination pedestrianCombination;
    Settings settings;
} Pedestrian_Client_data;
typedef struct {
    Pedestrian_MessageHeader hdr;
    char buf[PEDESTRIAN_BUF_SIZE];
} Pedestrian_State_Reply;
typedef struct {
    Pedestrian_MessageHeader hdr;
    ButtonPresses buttons;
} Pedestrian_Button_Reply;
//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Train Client/Server Structures
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------
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
//   Controller Client/Server Structures
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------
#define CONTROLLER_L2_ATTACH_POINT "L2_To_Controller"
#define CONTROLLER_L2_DATA_TYPE    0x24
#define CONTROLLER_L2_BUF_SIZE     100
#define CONTROLLER_L2_UPDATE_DATA   1
#define CONTROLLER_L2_REQUEST_STATE 2
typedef union {
    uint32_t sival_int;
    uint32_t dummy[4];
} Controller_L2_Sigval;
typedef struct {
    uint16_t type;
    uint16_t subtype;
    int8_t code;
    uint8_t zero[3];
    Controller_L2_Sigval value;
    uint8_t zero2[2];
    int32_t scoid;
} Controller_L2_MessageHeader;
typedef struct {
    Controller_L2_MessageHeader hdr;
    int32_t ClientID;
    CurrentCars cars;
    ButtonPresses buttons;
} Controller_L2_Message;
typedef struct {
    Controller_L2_MessageHeader hdr;
    char buf[CONTROLLER_L2_BUF_SIZE];
} Controller_L2_Acknowledgement;
typedef struct {
    Controller_L2_MessageHeader hdr;
    int32_t state; /* Carries a TrafficState value. */
} Controller_L2_StateReply;
//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    L1 and L2 Client/Server Structures
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------
#define L1_L2_ATTACH_POINT "L1_To_L2"
#define L1_L2_DATA_TYPE    0x25
#define L1_L2_BUF_SIZE    100
#define L1_L2_SEND_DATA    1
#define L1_L2_CHANGE_STATE 2
#define L1_L2_SET_STATE    3
typedef union {
    uint32_t sival_int;
    uint32_t dummy[4];
} L1_L2_Sigval;
typedef struct {
    uint16_t type;
    uint16_t subtype;
    int8_t code;
    uint8_t zero[3];
    L1_L2_Sigval value;
    uint8_t zero2[2];
    int32_t scoid;
} L1_L2_MessageHeader;
typedef struct {
    L1_L2_MessageHeader hdr;
    int32_t ClientID;
    int32_t state;
} L1_L2_Message;
typedef struct {
    L1_L2_MessageHeader hdr;
    ButtonPresses buttons;
    char buf[L1_L2_BUF_SIZE];
} L1_L2_Reply;
//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Main Function for Client/Server
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------
typedef enum {
    PEDESTRIAN_LINK = 0,
    CONTROLLER_LINK = 1
} ClientLink;
typedef struct {
    ClientLink link;
    const char *sname;
    TrafficLight *light;
} ClientThreadArguments;
int Train_Traffic_Server(TrafficLight *light, pthread_mutex_t *mutex);
int Pedestrian_Client(const char *sname, TrafficLight *light, const Settings *timing, pthread_mutex_t *mutex);
int Controller_L2_Client(const char *sname, TrafficLight *light, pthread_mutex_t *mutex);
int L1_L2_Server(TrafficLight *light, pthread_mutex_t *mutex);
static int *ClientConnectionFlag(TrafficLight *light, ClientLink link);
static void SetClientConnection(TrafficLight *, ClientLink, int, pthread_mutex_t *);
static void RecordClientReply(TrafficLight *, ClientLink, int, pthread_mutex_t *);
static int RequestAndWait(TrafficLight *, ClientLink, int *, pthread_cond_t *);
static void *RetryClientConnection(void *argument);
static void *TrainServerThread(void *argument);
static void *L1ServerThread(void *argument);
void TrafficLightTransition(TrafficLight *light, int tick, int greenTicks, int yellowTicks);
//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Main — Data Requests, Next-State Selection, Outputs, Timing and Local Cycling
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------
// Starts communication threads, requests data, selects next states, and runs the traffic-light timing and output sequence.
int main(int argc, char **argv) {
    // L2 owns this structure. There is no local L1 output structure to calculate.
    TrafficLight L2 = {
        .trafficState = TRAFFIC_RED,
        .trafficDirection = STATE_ALL_RED,
        .trafficDirectionNext = NS,
        .trainDirectionNext = NS,
        .controllerDirection = STATE_ALL_RED,
        .L1Direction = STATE_ALL_RED,
        .trainDirection = TRAIN_NOT_PRESENT
    };
    pthread_t threads[4]; // Two servers and two clients; main runs the lights.
    ClientThreadArguments clients[2] = {
        { PEDESTRIAN_LINK, argc > 1 ? argv[1] : PEDESTRIAN_ATTACH_POINT, &L2 },
        { CONTROLLER_LINK, argc > 2 ? argv[2] : CONTROLLER_L2_ATTACH_POINT, &L2 }
    };
    // Client paths can name remote nodes. Servers register their local short names.
    int error;
    settings.time = 5;   // Same L1 split: time - 3 green, 2 yellow, 1 red.
    settings.peroid = 1; // Seconds per tick.

    // Receive the train status independently of the L1 and controller connections.
    error = pthread_create(&threads[0], NULL, TrainServerThread, &L2);
    if (error != 0) {
        fprintf(stderr, "pthread_create train server: %s\n", strerror(error));
        return EXIT_FAILURE;
    }
    // L1 is the client. This server receives its data/state/start commands.
    error = pthread_create(&threads[1], NULL, L1ServerThread, &L2);
    if (error != 0) {
        fprintf(stderr, "pthread_create L1 server: %s\n", strerror(error));
        return EXIT_FAILURE;
    }
    // Pedestrian and controller each get their own persistent client thread.
    for (int i = 0; i < 2; ++i) {
        error = pthread_create(&threads[i + 2], NULL, RetryClientConnection, &clients[i]);
        if (error != 0) {
            fprintf(stderr, "pthread_create client: %s\n", strerror(error));
            return EXIT_FAILURE;
        }
    }
    sleep(5); // Same startup allowance as L1; connections open concurrently.

    for (;;) {
        TrafficLight snapshotL2;
        Settings timing;
        TrafficState nextL2 = STATE_ALL_RED;
        int controllerReady = 0;
        int localCycle = 0;

        // Prepare data while red, then wait for permission to start if L1 is connected.
        for (;;) {
            pthread_mutex_lock(&light_mutex);
            L2.L1DataRequest = 0; // This preparation handles the pending data request.
            pthread_mutex_unlock(&light_mutex);

            // Read L2's own pedestrian buttons; do not copy L1's pedestrian values.
            if (!RequestAndWait(&L2, PEDESTRIAN_LINK,
                                &L2.pedestrianRequest, &pedestrian_cond)) {
                pthread_mutex_lock(&light_mutex);
                L2.buttons = (ButtonPresses){0};
                pthread_mutex_unlock(&light_mutex);
            }
            // Upload L2 cars/buttons; the client waits one second and asks for statesL2.
            controllerReady = RequestAndWait(&L2, CONTROLLER_LINK,
                                             &L2.controllerRequest, &controller_cond);

            pthread_mutex_lock(&light_mutex);
            // A controller reply supplies the state, but does NOT grant a start.
            // Release the mutex while waiting so servers can update these flags.
            while (L2.L1Connected && !L2.L1StartRequested && !L2.L1DataRequest)
                pthread_cond_wait(&l1_l2_cond, &light_mutex);

            // L1 asked for another data upload while we were waiting/preparing.
            // Refresh it before selecting the next state; keep any start signal latched.
            if (L2.L1DataRequest) {
                pthread_mutex_unlock(&light_mutex);
                continue;
            }

            snapshotL2 = L2;
            timing = settings;
            controllerReady = controllerReady && L2.controllerConnected;
            localCycle = !controllerReady && !L2.L1Connected;

            // Same train priority as L1. The state functions only choose a value.
            if (controllerReady)
                nextL2 = ControllerStateMachine(&snapshotL2, NULL);
            else if (L2.L1Connected)
                nextL2 = CrossCommunicationStateMachine(&snapshotL2, NULL);
            else
                nextL2 = NoControllerStateMachine(&snapshotL2, NULL);

            // Consume one start permission. A signal received during the last
            // cycle was retained, so it is not lost when main was sleeping.
            L2.L1StartRequested = 0;
            L2.trafficDirection = nextL2;
            StateOutput(nextL2, &L2.outputL2);
            L2.pedestrianCombination = GetPedestrianCombination(&L2.outputL2);
            pthread_mutex_unlock(&light_mutex);
            break;
        }

        // Tell L2's pedestrian node which crossings are part of this new state.
        RequestAndWait(&L2, PEDESTRIAN_LINK, &L2.stateChange, &pedestrian_cond);

        // Same colour timing and new-train response as your L1 main loop.
        for (int tick = 0; tick < timing.time; ++tick) {
            pthread_mutex_lock(&light_mutex);
            if (!snapshotL2.train_detected && L2.train_detected && tick < timing.time - 3)
                tick = timing.time - 3; // End green early; finish yellow and red.
            TrafficLightTransition(&L2, tick, timing.time - 3, 2);
            if (nextL2 == STATE_ALL_RED)
                L2.trafficState = TRAFFIC_RED;
            pthread_mutex_unlock(&light_mutex);
            sleep(timing.peroid);
        }

        // Finish at red. L1 permission never skips yellow/red clearance.
        pthread_mutex_lock(&light_mutex);
        L2.trafficState = TRAFFIC_RED;
        StateOutput(STATE_ALL_RED, &L2.outputL2);
        L2.pedestrianCombination = PED_NONE;
        // Increment our own route only when neither controller nor L1 chose it.
        if (localCycle && !snapshotL2.train_detected)
            L2.trafficDirectionNext = (TrafficStates)((L2.trafficDirectionNext + 1) % 6);
        pthread_mutex_unlock(&light_mutex);
        RequestAndWait(&L2, PEDESTRIAN_LINK, &L2.stateChange, &pedestrian_cond);
    }
    return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Connection and Reply Notifications — Small Hooks Used by Existing Clients
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------
// Returns the address of the connection flag for the selected client. -> Checks if pedestrian, controller or L2 is connected
static int *ClientConnectionFlag(TrafficLight *light, ClientLink link) {
    if (link == PEDESTRIAN_LINK) return &light->pedestrianConnected;
    return &light->controllerConnected;
}
static void SetClientConnection(TrafficLight *light, ClientLink link, int connected, pthread_mutex_t *mutex) {
    // Lock up the mutex for data safety
    pthread_mutex_lock(mutex);
    // Set the Connection with the connected value and also uses the link to determine which of the client link it is
    *ClientConnectionFlag(light, link) = connected;
    // If there is no connection to the server
    if (!connected) {
        // Store the Results as EXIT_FAILURE and increment the failure
        light->replyResult[link] = EXIT_FAILURE;
        light->replyCount[link]++;
        // Clears the request as there is no connection and no request
        if (link == PEDESTRIAN_LINK) {
            light->pedestrianRequest = 0;
            light->stateChange = 0;
        } else if (link == CONTROLLER_LINK) {
            light->controllerRequest = 0;
        }
        light->requestInFlight[link] = 0;
        light->requestExpired[link] = 0;
    }
    pthread_cond_broadcast(&communication_cond); // Wake up all the other thread to tell them communication has failed
    // This is done so that the threads relying on this infomation of thread connection can know even when sleeping
    pthread_mutex_unlock(mutex);
}
static void RecordClientReply(TrafficLight *light, ClientLink link, int result, pthread_mutex_t *mutex) {
    pthread_mutex_lock(mutex);
    light->replyResult[link] = light->requestExpired[link] ? EXIT_FAILURE : result;
    light->requestInFlight[link] = 0;
    light->replyCount[link]++;
    pthread_cond_broadcast(&communication_cond);
    pthread_mutex_unlock(mutex);
}
static int RequestAndWait(TrafficLight *light, ClientLink link, int *request, pthread_cond_t *condition) {
    unsigned before;
    int success;
    struct timespec deadline;
    // Setup an timer
    if (clock_gettime(CLOCK_REALTIME, &deadline) == -1)
        return 0;
    deadline.tv_sec += 3;  // Example deadline: three seconds from now.
    // Lock the Mutexes
    pthread_mutex_lock(&light_mutex);
    if (!*ClientConnectionFlag(light, link) || light->requestInFlight[link]) {
        // If the client is not connected to the current given link return 0 (Not successful)
        pthread_mutex_unlock(&light_mutex);
        return 0;
    }
    // If there is an conection
    before = light->replyCount[link];
    light->requestInFlight[link] = 1;
    light->requestExpired[link] = 0;
    *request = 1;
    pthread_cond_signal(condition);
    // Check if an clent reply is given -> used to check is message is received
    while (light->replyCount[link] == before) {
        // Timed thread wait
        int error = pthread_cond_timedwait(&communication_cond, &light_mutex, &deadline);
        /* A result may have arrived at the same time as the timeout. */
        if (light->replyCount[link] != before)
            break;
        // Timeout error -> if the reply takes too long to arrive
        if (error == ETIMEDOUT) {
            light->requestExpired[link] = 1;
            pthread_mutex_unlock(&light_mutex);
            fprintf(stderr, "Timed out waiting for client\n");
            return 0;
        }
        // Error handeling
        if (error != 0) {
            light->requestExpired[link] = 1;
            pthread_mutex_unlock(&light_mutex);
            return 0;
        }
    }
    success = (light->replyResult[link] == EXIT_SUCCESS);
    pthread_mutex_unlock(&light_mutex);
    return success;
}
//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Worker Threads — Reconnect Clients Two Seconds After Failure
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------
// Infinite Loop to try connection and sleep for 2 seconds
static void *RetryClientConnection(void *argument) {
    ClientThreadArguments *args = argument;
    for (;;) {
        if (args->link == PEDESTRIAN_LINK)
            Pedestrian_Client(args->sname, args->light, &settings, &light_mutex);
        else
            Controller_L2_Client(args->sname, args->light, &light_mutex);
        SetClientConnection(args->light, args->link, 0, &light_mutex);
        fprintf(stderr, "Connection to %s ended; retrying in 2 seconds\n", args->sname);
        sleep(2);
    }
    return NULL;
}
static void *L1ServerThread(void *argument) {
    TrafficLight *light = argument;
    for (;;) {
        L1_L2_Server(light, &light_mutex);
        pthread_mutex_lock(&light_mutex);
        light->L1ServerReady = 0;
        light->L1Connected = 0;
        light->L1DataRequest = 0;
        light->L1StartRequested = 0;
        light->L1StateValid = 0;
        pthread_cond_broadcast(&l1_l2_cond);
        pthread_mutex_unlock(&light_mutex);
        sleep(2);
    }
    return NULL;
}
// Start Up the Train server
static void *TrainServerThread(void *argument) {
    TrafficLight *light = argument;
    for (;;) {
        // Tries to set up the server every 2 seconds
        Train_Traffic_Server(light, &light_mutex);
        pthread_mutex_lock(&light_mutex);
        light->trainServerReady = 0;
        light->trainConnected = 0;
        pthread_mutex_unlock(&light_mutex);
        sleep(2);
    }
    return NULL;
}
//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Train-to-Traffic Server
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------
int Train_Traffic_Server(TrafficLight *light, pthread_mutex_t *mutex) {
    name_attach_t *attach;
    Train_Client_data msg = {0};
    union {
        struct _pulse pulse;
        Train_Client_data message;
    } received;
    int rcvid;
    int status = EXIT_SUCCESS;
    if ((attach = name_attach(NULL,
                              TRAIN_TRAFFIC_ATTACH_POINT, 0)) == NULL) {
        perror("name_attach");
        return EXIT_FAILURE;
    }
    pthread_mutex_lock(mutex);
    light->trainServerReady = 1;
    pthread_cond_broadcast(&communication_cond);
    pthread_mutex_unlock(mutex);
    printf("Server listening on: %s\n", TRAIN_TRAFFIC_ATTACH_POINT);
    while (1) {
        memset(&received, 0, sizeof(received));
        // If no data is received -> stays here indefinitely
        rcvid = MsgReceive(attach->chid, &received, sizeof(received), NULL);
        if (rcvid == -1) {
            perror("MsgReceive");
            status = EXIT_FAILURE;
            break;
        }
        if (rcvid == 0) {
            switch (received.pulse.code) {
                case _PULSE_CODE_DISCONNECT:
                    ConnectDetach(received.pulse.scoid);
                    pthread_mutex_lock(mutex);
                    light->trainConnected = 0;
                    pthread_mutex_unlock(mutex);
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
        /* Store the received status on the traffic-light node. */
        pthread_mutex_lock(mutex);
        light->trainConnected = 1;
        light->trainDirection = (TrainState)msg.trainState;
        light->train_detected = (light->trainDirection == TRAIN_PRESENT);
        pthread_mutex_unlock(mutex);
        Train_Server_Reply reply = {0};
        reply.hdr.type = TRAIN_DATA_TYPE;
        reply.hdr.subtype = TRAIN_STATUS_UPDATE;
        snprintf(reply.buf, sizeof(reply.buf), "Train status received");
        MsgReply(rcvid, EOK, &reply, sizeof(reply));
    }
    name_detach(attach, 0);
    return status;
}
//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    L2 Server — Receives Data Requests, Fallback States and Start Signals from L1
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------
int L1_L2_Server(TrafficLight *light, pthread_mutex_t *mutex) {
    name_attach_t *attach;
    union {
        struct _pulse pulse;
        L1_L2_Message message;
    } received;
    int rcvid;
    int status = EXIT_SUCCESS;

    // L1 opens /net/<L2-host>/dev/name/local/L1_To_L2 to reach this server.
    if ((attach = name_attach(NULL, L1_L2_ATTACH_POINT, 0)) == NULL) {
        perror("name_attach L1_To_L2");
        return EXIT_FAILURE;
    }
    pthread_mutex_lock(mutex);
    light->L1ServerReady = 1; // Listening does not yet mean L1 is connected.
    pthread_mutex_unlock(mutex);
    printf("L2 listening for L1 on: %s\n", L1_L2_ATTACH_POINT);

    while (1) {
        memset(&received, 0, sizeof(received));
        rcvid = MsgReceive(attach->chid, &received, sizeof(received), NULL);
        if (rcvid == -1) {
            perror("MsgReceive L1_To_L2");
            status = EXIT_FAILURE;
            break;
        }
        if (rcvid == 0) {
            switch (received.pulse.code) {
                case _PULSE_CODE_DISCONNECT:
                    // One L1 client is expected on this channel.
                    ConnectDetach(received.pulse.scoid);
                    pthread_mutex_lock(mutex);
                    light->L1Connected = 0;
                    light->L1DataRequest = 0;
                    light->L1StartRequested = 0;
                    light->L1StateValid = 0;
                    pthread_cond_broadcast(&l1_l2_cond); // Let main use fallback mode.
                    pthread_mutex_unlock(mutex);
                    break;
                case _PULSE_CODE_UNBLOCK:
                case _PULSE_CODE_COIDDEATH:
                case _PULSE_CODE_THREADDEATH:
                default:
                    break;
            }
            continue;
        }

        L1_L2_Message msg = received.message;
        if (msg.hdr.type == _IO_CONNECT) {
            pthread_mutex_lock(mutex);
            light->L1Connected = 1;
            pthread_mutex_unlock(mutex);
            MsgReply(rcvid, EOK, NULL, 0);
            continue;
        }
        if ((msg.hdr.type >= _IO_BASE && msg.hdr.type <= _IO_MAX) ||
            msg.hdr.type != L1_L2_DATA_TYPE || msg.ClientID != 1) {
            MsgError(rcvid, ENOSYS);
            continue;
        }
        if (msg.hdr.subtype != L1_L2_SEND_DATA &&
            msg.hdr.subtype != L1_L2_SET_STATE &&
            msg.hdr.subtype != L1_L2_CHANGE_STATE) {
            MsgError(rcvid, ENOSYS);
            continue;
        }
        if (msg.hdr.subtype == L1_L2_SET_STATE &&
            (msg.state < STATE_NS_SN_Left_NS_Right_NS ||
             msg.state > STATE_ES_SE_Top_EW_Left_NS)) {
            MsgError(rcvid, EINVAL);
            continue;
        }

        L1_L2_Reply reply = {0};
        reply.hdr.type = L1_L2_DATA_TYPE;
        reply.hdr.subtype = msg.hdr.subtype;
        pthread_mutex_lock(mutex);
        light->L1Connected = 1;
        switch (msg.hdr.subtype) {
            case L1_L2_SEND_DATA:
                // Return the latest stored L2 buttons. Main will perform a fresh
                // pedestrian/controller exchange at its next preparation point.
                // Reply immediately so this receive loop can accept L1's start.
                reply.buttons = light->buttons;
                light->L1DataRequest = 1;
                light->L1StateValid = 0; // Do not reuse a previous cycle's fallback.
                light->L1StartRequested = 0; // A new preparation supersedes an older pending start.
                snprintf(reply.buf, sizeof(reply.buf), "L2 data preparation requested");
                break;
            case L1_L2_SET_STATE:
                // Store L1's choice separately. A working controller has priority.
                light->L1Direction = (TrafficState)msg.state;
                light->L1StateValid = 1;
                snprintf(reply.buf, sizeof(reply.buf), "L2 fallback state received");
                break;
            case L1_L2_CHANGE_STATE:
                // Permission to begin the next cycle; main consumes this once.
                // Keep it even if main is still completing the current cycle.
                light->L1StartRequested = 1;
                snprintf(reply.buf, sizeof(reply.buf), "L2 start signal received");
                break;
        }
        pthread_cond_broadcast(&l1_l2_cond);
        pthread_mutex_unlock(mutex);
        // Acknowledges receipt. It does not claim that GPIO changed simultaneously.
        MsgReply(rcvid, EOK, &reply, sizeof(reply));
    }
    name_detach(attach, 0);
    return status;
}

//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Controller L2 Client — Persistent Connection, One Exchange per Request
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------
int Controller_L2_Client(const char *sname, TrafficLight *light, pthread_mutex_t *mutex) {
    int server_coid;
    int status = EXIT_SUCCESS;
    printf("Trying to connect to: %s\n", sname);
    /* Open the connection once. */
    if ((server_coid = name_open(sname, 0)) == -1) {
        perror("name_open");
        return EXIT_FAILURE;
    }
    SetClientConnection(light, CONTROLLER_LINK, 1, mutex);
    printf("Connection established to: %s\n", sname);
    while (1) {
        Controller_L2_Message msg = {0};
        Controller_L2_Acknowledgement acknowledgement = {0};
        Controller_L2_StateReply reply = {0};
        pthread_mutex_lock(mutex);
        /* Sleep until another thread requests an exchange. */
        while (!light->controllerRequest) {
            pthread_cond_wait(&controller_cond, mutex);
        }
        if (light->requestExpired[CONTROLLER_LINK]) {
            light->controllerRequest = 0;
            pthread_mutex_unlock(mutex);
            RecordClientReply(light, CONTROLLER_LINK, EXIT_FAILURE, mutex);
            continue;
        }
        msg.cars = light->storedDirection;
        msg.buttons = light->buttons;
        pthread_mutex_unlock(mutex);
        msg.ClientID = 2;
        msg.hdr.type = CONTROLLER_L2_DATA_TYPE;
        msg.hdr.subtype = CONTROLLER_L2_UPDATE_DATA;
        /* First message: send cars and buttons. */
        if (MsgSend(server_coid, &msg, sizeof(msg),
                    &acknowledgement, sizeof(acknowledgement)) == -1) {
            perror("MsgSend");
            status = EXIT_FAILURE;
            break;
        }
        printf("Reply: %.*s\n",
               CONTROLLER_L2_BUF_SIZE, acknowledgement.buf);
        /* Your requested calculation interval. */
        sleep(1);
        /* Second message: ask for the calculated L2 state. */
        msg.hdr.subtype = CONTROLLER_L2_REQUEST_STATE;
        if (MsgSend(server_coid, &msg, sizeof(msg), &reply, sizeof(reply)) == -1) {
            perror("MsgSend");
            status = EXIT_FAILURE;
            break;
        }
        pthread_mutex_lock(mutex);
        if (!light->requestExpired[CONTROLLER_LINK])
            light->controllerDirection = (TrafficState)reply.state;
        light->controllerRequest = 0; /* This exchange is complete. */
        pthread_mutex_unlock(mutex);
        RecordClientReply(light, CONTROLLER_LINK, EXIT_SUCCESS, mutex);
        /* Return to waiting, keeping the connection open. */
    }
    name_close(server_coid);
    return status;
}
//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Client Code for Pedestrian
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------
int Pedestrian_Client(const char *sname, TrafficLight *light, const Settings *settings, pthread_mutex_t *mutex) {
    int server_coid;
    int status = EXIT_SUCCESS;
    printf("Trying to connect to: %s\n", sname);
    /* Open once and keep the connection while the client runs. */
    if ((server_coid = name_open(sname, 0)) == -1) {
        perror("name_open");
        return EXIT_FAILURE;
    }
    SetClientConnection(light, PEDESTRIAN_LINK, 1, mutex);
    printf("Connection established to: %s\n", sname);
    while (1) {
        Pedestrian_Client_data msg = {0};
        int command;
        msg.ClientID = 800;
        msg.hdr.type = PEDESTRIAN_DATA_TYPE;
        pthread_mutex_lock(mutex);
        /* Block until work is requested. Releases mutex while waiting. */
        while (!light->pedestrianRequest && !light->stateChange) {
            pthread_cond_wait(&pedestrian_cond, mutex);
        }
        if (light->requestExpired[PEDESTRIAN_LINK]) {
            light->pedestrianRequest = 0;
            light->stateChange = 0;
            pthread_mutex_unlock(mutex);
            RecordClientReply(light, PEDESTRIAN_LINK, EXIT_FAILURE, mutex);
            continue;
        }
        /* Consume one request. Button requests take priority. */
        if (light->pedestrianRequest) {
            command = PEDESTRIAN_REQUEST_DATA;
        } else {
            command = PEDESTRIAN_SET_STATE;
            msg.pedestrianCombination = light->pedestrianCombination;
            msg.settings = *settings;
            light->stateChange = 0;
        }
        pthread_mutex_unlock(mutex);
        msg.hdr.subtype = command;
        /* Never hold the mutex while waiting for a network reply. */
        switch (command) {
            case PEDESTRIAN_SET_STATE: {
                Pedestrian_State_Reply reply = {0};
                if (MsgSend(server_coid, &msg, sizeof(msg),
                            &reply, sizeof(reply)) == -1) {
                    perror("MsgSend");
                    status = EXIT_FAILURE;
                } else {
                    printf("Reply: %.*s\n",
                           PEDESTRIAN_BUF_SIZE, reply.buf);
                }
                break;
            }
            case PEDESTRIAN_REQUEST_DATA: {
                Pedestrian_Button_Reply reply = {0};
                if (MsgSend(server_coid, &msg, sizeof(msg),
                            &reply, sizeof(reply)) == -1) {
                    perror("MsgSend");
                    status = EXIT_FAILURE;
                } else {
                    pthread_mutex_lock(mutex);
                    if (!light->requestExpired[PEDESTRIAN_LINK])
                        light->buttons = reply.buttons;
                    light->pedestrianRequest = 0;
                    pthread_mutex_unlock(mutex);
                }
                break;
            }
        }
        if (status == EXIT_SUCCESS)
            RecordClientReply(light, PEDESTRIAN_LINK, EXIT_SUCCESS, mutex);
        if (status == EXIT_FAILURE) {
            /* Leave the failed operation pending before exiting. */
            pthread_mutex_lock(mutex);
            if (command == PEDESTRIAN_REQUEST_DATA)
                light->pedestrianRequest = 1;
            else
                light->stateChange = 1;
            pthread_mutex_unlock(mutex);
            break;
        }
    }
    name_close(server_coid);
    return status;
}
//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Code below is for the traffic light logic node, which is separate from the controller and train nodes.
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------
void TrafficLightTransition(TrafficLight *light, int tick, int greenTicks, int yellowTicks)
{
    if (tick < greenTicks) {
        light->trafficState = TRAFFIC_GREEN;
    } else if (tick < greenTicks + yellowTicks) {
        light->trafficState = TRAFFIC_YELLOW;
    } else {
        light->trafficState = TRAFFIC_RED;
    }
}
/* These three functions select states only. main owns timing and outputs.
 * Pass snapshots made under light_mutex, not live concurrently modified data.
 */
TrafficState ControllerStateMachine(void *state_ptr, void *inputs) {
    TrafficLight *light = state_ptr;
    (void)inputs;
    return light->train_detected ? TrainLogicNode(light) : light->controllerDirection;
}
TrafficState CrossCommunicationStateMachine(void *state_ptr, void *inputs)
{
    TrafficLight *light = state_ptr;
    (void)inputs;
    if (light->train_detected)
        return TrainLogicNode(light);
    return light->L1StateValid ? light->L1Direction : STATE_ALL_RED;
}
TrafficState NoControllerStateMachine(void *state_ptr, void *inputs) {
    TrafficLight *light = state_ptr;
    (void)inputs;
    return light->train_detected ? TrainLogicNode(light) : TrafficLogicNode(light);
}
static TrafficState SelectTrafficState(int direction, int Left_NS, int Right_NS,int Top_EW, int Bottom_EW) {
    switch (direction) {
        case NS:
            if (Left_NS && Right_NS)
                return STATE_NS_SN_Left_NS_Right_NS;
            if (Left_NS)
                return STATE_NS_SN_NE_Left_NS;
            if (Right_NS)
                return STATE_NS_SN_SW_Right_NS;
            return STATE_NS_SN_NE_SW;
        case EW:
            if (Top_EW && Bottom_EW)
                return STATE_EW_WE_Top_EW_Bottom_EW;
            if (Top_EW)
                return STATE_EW_WE_ES_Top_EW;
            if (Bottom_EW)
                return STATE_EW_WE_WN_Bottom_EW;
            return STATE_EW_WE_WN_ES;
        case NW:
            if (Right_NS && Bottom_EW)
                return STATE_NW_WN_Bottom_EW_Right_NS;
            if (Right_NS)
                return STATE_NW_WN_NS_Right_NS;
            if (Bottom_EW)
                return STATE_NW_WN_NE_Bottom_EW;
            return STATE_NW_WN_NS_NE;
        case WS:
            if (Top_EW && Right_NS)
                return STATE_SW_WS_Top_EW_Right_NS;
            if (Top_EW)
                return STATE_SW_WS_WE_Top_EW;
            if (Right_NS)
                return STATE_SW_WS_WN_Right_NS;
            return STATE_WS_SW_WN_WE;
        case EN:
            if (Bottom_EW && Left_NS)
                return STATE_NE_EN_Bottom_EW_Left_NS;
            if (Bottom_EW)
                return STATE_NE_EW_EN_Bottom_EW;
            if (Left_NS)
                return STATE_NE_ES_EN_Left_NS;
            return STATE_NE_ES_EN_EW;
        case SE:
            if (Top_EW && Left_NS)
                return STATE_ES_SE_Top_EW_Left_NS;
            if (Top_EW)
                return STATE_ES_SE_SW_Top_EW;
            if (Left_NS)
                return STATE_ES_SE_SN_Left_NS;
            return STATE_ES_SE_SW_SN;
        default:
            return STATE_ALL_RED;
    }
}
// Calculates once and returns. Does not change light or any output fields.
TrafficState TrafficLogicNode(void *state_ptr) {
    const TrafficLight *light = state_ptr;
    if (!light) return STATE_ALL_RED;
    return SelectTrafficState(light->trafficDirectionNext, light->buttons.Left_NS, light->buttons.Right_NS, light->buttons.Top_EW, light->buttons.Bottom_EW);
}
/* Missing L2 wrapper: use the same selector with L2's own data. */
TrafficState TrafficLogicNodeL2(void *state_ptr) {
    return TrafficLogicNode(state_ptr);
}
// This is the uploaded traffic-route selector using trainDirection,
// not the separate TrainData/boom-gate TrainLogicNode from the train node.
TrafficState TrainLogicNode(void *state_ptr) {
    const TrafficLight *light = state_ptr;
    if (!light) return STATE_ALL_RED;
    switch (light->trainDirectionNext) {
        case NS:
        case NW:
        case WS:
            return SelectTrafficState(light->trainDirectionNext, light->buttons.Left_NS, light->buttons.Right_NS, light->buttons.Top_EW, light->buttons.Bottom_EW);
        default:
            return STATE_ALL_RED;
    }
}
void StateOutput(TrafficState state, Movements *output) {
    if (output == NULL) {
        return;
    }
    *output = (Movements){0}; /* Clear ALL_RED/default as well. */
    switch (state) {
        // North-South routes
        case STATE_NS_SN_Left_NS_Right_NS:
            *output = (Movements){
                .NS = 1, .SN = 1, .Left_NS = 1, .Right_NS = 1
            };
            break;
        case STATE_NS_SN_NE_Left_NS:
            *output = (Movements){
                .NS = 1, .SN = 1, .NE = 1, .Left_NS = 1
            };
            break;
        case STATE_NS_SN_SW_Right_NS:
            *output = (Movements){
                .NS = 1, .SN = 1, .SW = 1, .Right_NS = 1
            };
            break;
        case STATE_NS_SN_NE_SW:
            *output = (Movements){
                .NS = 1, .SN = 1, .NE = 1, .SW = 1
            };
            break;
        // West-South routes
        case STATE_NE_SW_WS_WN:
            *output = (Movements){
                .NE = 1, .SW = 1, .WS = 1, .WN = 1
            };
            break;
        case STATE_WS_SW_WN_WE:
            *output = (Movements){
                .WS = 1, .SW = 1, .WN = 1, .WE = 1
            };
            break;
        case STATE_SW_WS_WN_Right_NS:
            *output = (Movements){
                .SW = 1, .WS = 1, .WN = 1, .Right_NS = 1
            };
            break;
        case STATE_SW_WS_WE_Top_EW:
            *output = (Movements){
                .SW = 1, .WS = 1, .WE = 1, .Top_EW = 1
            };
            break;
        case STATE_SW_WS_Top_EW_Right_NS:
            *output = (Movements){
                .SW = 1, .WS = 1, .Top_EW = 1, .Right_NS = 1
            };
            break;
        // North-West routes
        case STATE_NW_WN_NS_NE:
            *output = (Movements){
                .NW = 1, .WN = 1, .NS = 1, .NE = 1
            };
            break;
        case STATE_NW_WN_NE_ES:
            *output = (Movements){
                .NW = 1, .WN = 1, .NE = 1, .ES = 1
            };
            break;
        case STATE_NW_WN_NE_Bottom_EW:
            *output = (Movements){
                .NW = 1, .WN = 1, .NE = 1, .Bottom_EW = 1
            };
            break;
        case STATE_NW_WN_NS_Right_NS:
            *output = (Movements){
                .NW = 1, .WN = 1, .NS = 1, .Right_NS = 1
            };
            break;
        case STATE_NW_WN_Bottom_EW_Right_NS:
            *output = (Movements){
                .NW = 1, .WN = 1, .Bottom_EW = 1, .Right_NS = 1
            };
            break;
        // East-West routes
        case STATE_EW_WE_WN_ES:
            *output = (Movements){
                .EW = 1, .WE = 1, .WN = 1, .ES = 1
            };
            break;
        case STATE_EW_WE_WN_Bottom_EW:
            *output = (Movements){
                .EW = 1, .WE = 1, .WN = 1, .Bottom_EW = 1
            };
            break;
        case STATE_EW_WE_ES_Top_EW:
            *output = (Movements){
                .EW = 1, .WE = 1, .ES = 1, .Top_EW = 1
            };
            break;
        case STATE_EW_WE_Top_EW_Bottom_EW:
            *output = (Movements){
                .EW = 1, .WE = 1, .Top_EW = 1, .Bottom_EW = 1
            };
            break;
        // East-North routes
        case STATE_NE_ES_EN_SW:
            *output = (Movements){
                .NE = 1, .ES = 1, .EN = 1, .SW = 1
            };
            break;
        case STATE_NE_ES_EN_EW:
            *output = (Movements){
                .NE = 1, .ES = 1, .EN = 1, .EW = 1
            };
            break;
        case STATE_NE_ES_EN_Bottom_EW:
            *output = (Movements){
                .NE = 1, .ES = 1, .EN = 1, .Bottom_EW = 1
            };
            break;
        case STATE_NE_EW_EN_Bottom_EW:
            *output = (Movements){
                .NE = 1, .EW = 1, .EN = 1, .Bottom_EW = 1
            };
            break;
        case STATE_NE_Top_EW_EN_Bottom_EW:
            *output = (Movements){
                .NE = 1, .Top_EW = 1, .EN = 1, .Bottom_EW = 1
            };
            break;
        // South-East routes
        case STATE_ES_SE_SW_WN:
            *output = (Movements){
                .ES = 1, .SE = 1, .SW = 1, .WN = 1
            };
            break;
        case STATE_ES_SE_SW_SN:
            *output = (Movements){
                .ES = 1, .SE = 1, .SW = 1, .SN = 1
            };
            break;
        case STATE_ES_SE_SW_Bottom_EW:
            *output = (Movements){
                .ES = 1, .SE = 1, .SW = 1, .Bottom_EW = 1
            };
            break;
        case STATE_ES_SE_SN_Bottom_EW:
            *output = (Movements){
                .ES = 1, .SE = 1, .SN = 1, .Bottom_EW = 1
            };
            break;
        case STATE_ES_SE_Top_EW_Bottom_EW:
            *output = (Movements){
                .ES = 1, .SE = 1, .Top_EW = 1, .Bottom_EW = 1
            };
            break;
        /* Missing mappings for states already used in your selector. */
        case STATE_NE_ES_EN_Left_NS:
            *output = (Movements){.NE = 1, .ES = 1, .EN = 1, .Left_NS = 1};
            break;
        case STATE_NE_EN_Bottom_EW_Left_NS:
            *output = (Movements){.NE = 1, .EN = 1, .Bottom_EW = 1, .Left_NS = 1};
            break;
        case STATE_ES_SE_SW_Top_EW:
            *output = (Movements){.ES = 1, .SE = 1, .SW = 1, .Top_EW = 1};
            break;
        case STATE_ES_SE_SN_Left_NS:
            *output = (Movements){.ES = 1, .SE = 1, .SN = 1, .Left_NS = 1};
            break;
        case STATE_ES_SE_Top_EW_Left_NS:
            *output = (Movements){.ES = 1, .SE = 1, .Top_EW = 1, .Left_NS = 1};
            break;
        case STATE_ALL_RED:
        default:
            // All outputs remain zero.
            break;
    }
}
PedestrianCombination GetPedestrianCombination(const Movements *output) {
    int combination = PED_NONE;
    if (output->Left_NS)
        combination += PED_LNS;
    if (output->Right_NS)
        combination += PED_RNS;
    if (output->Top_EW)
        combination += PED_TEW;
    if (output->Bottom_EW)
        combination += PED_BEW;
    return (PedestrianCombination)combination;
}
TrafficState MovementsToTrafficState(const Movements *movement) {
    Movements expected;
    for (int state = STATE_NS_SN_Left_NS_Right_NS;
         state <= STATE_ES_SE_Top_EW_Left_NS;
         state++) {
        StateOutput((TrafficState)state, &expected);
        if (movement->NE == expected.NE &&
            movement->NS == expected.NS &&
            movement->NW == expected.NW &&
            movement->EN == expected.EN &&
            movement->ES == expected.ES &&
            movement->EW == expected.EW &&
            movement->SN == expected.SN &&
            movement->SE == expected.SE &&
            movement->SW == expected.SW &&
            movement->WN == expected.WN &&
            movement->WE == expected.WE &&
            movement->WS == expected.WS &&
            movement->Left_NS == expected.Left_NS &&
            movement->Right_NS == expected.Right_NS &&
            movement->Top_EW == expected.Top_EW &&
            movement->Bottom_EW == expected.Bottom_EW) {
            return (TrafficState)state;
        }
    }
    /* No matching combination. */
    return STATE_ALL_RED;
}

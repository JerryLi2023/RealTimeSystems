#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h> /* sleep() */
#include <sys/iofunc.h>
#include <sys/netmgr.h>
#include <sys/neutrino.h> /* struct _pulse */
#ifndef BUF_SIZE
#define BUF_SIZE 100
#endif
// State Machine Structure
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
/* Reuse the direction constants; C cannot declare NS/NW/WS twice. */
typedef TrafficStates TrainState;
typedef struct {
    int NE, NS, NW;
    int EN, ES, EW;
    int SN, SE, SW;
    int WN, WE, WS;
    int Left_NS, Right_NS;
    int Top_EW, Bottom_EW;
} Movements;
typedef struct {
    int Left_NS, Right_NS;
    int Top_EW, Bottom_EW;
} ButtonPresses;
typedef struct {
    TrafficLightState trafficState;   // Shows the current light that traffic is on
    TrafficState trafficDirection;    // The Current State direction of the traffic light
    TrafficStates trafficDirectionNext; // Direction group for local cycling
    TrafficStates trainDirectionNext;          // The Next State direction of the train
    TrafficState controllerDirection; // Recieved traffic controller from the controller
    TrainState trainDirection;          // The state of the train direction
    Movements outputL1;               // Ouput data to the traffic light L1
    Movements outputL2;               // Ouput data to the traffic light L2
    ButtonPresses buttons;
    int train_detected; // Flag to indicate if a train is detected
    int stateChange;
} TrafficLight;


// Global variables
Settings settings = {0};
int train_detected = 0; // Flag to indicate if a train is detected
int train_state = 0; // Flag to indicate if state machine should remain at train state
// Function prototypes
TrafficState TrafficLogicNode(void *state_ptr);
TrafficState TrafficLogicNodeL2(void *state_ptr);
TrafficState TrainLogicNode(void *state_ptr);
void ControllerStateMachine(void *state_ptr, void *inputs);
void CrossCommunicationStateMachine(void *state_ptr, void *inputs);
void NoControllerStateMachine(void *state_ptr, void *inputs);
void StateOutput(TrafficState state, Movements *output);
#ifndef SAMPLECODE_NO_MAIN
int main(void) {
    TrafficLight light = {
        .trafficState = TRAFFIC_GREEN,
        .trafficDirection = STATE_NS_SN_NE_SW,
        .trafficDirectionNext = NS,
        .trainDirectionNext = NS,
        .controllerDirection = STATE_NS_SN_Left_NS_Right_NS,
        .trainDirection = NS
    };
    /* Example timing: original zero values made the loops run without waiting. */
    settings.time = 5;
    settings.peroid = 1;

    while (1) {
        if (controllerConnected) {
            ControllerStateMachine(&light, NULL);
        } else if (L2Connected) {
            CrossCommunicationStateMachine(&light, &L2);
        } else {
            NoControllerStateMachine(&light, NULL);
        }
    }

    /* server_PedestrianL1 was declared but not supplied, so it cannot be linked.
     * Run your existing local loop directly until you add the server body.
     * To run controller mode instead, replace this call with:
     * ControllerStateMachine(&light, NULL);
     */
    NoControllerStateMachine(&light, NULL);
    return 0;
}
#endif

void ControllerStateMachine(void *state_ptr, void *inputs) {
    // Implement the controller state machine logic here
    // This function will manage the traffic light states based on inputs and timing
    TrafficLight *light = state_ptr;
    (void)inputs;
    if (light->train_detected) {
        light->trafficDirection = TrainLogicNode(light);
        StateOutput(light->trafficDirection, &light->outputL1);
        for (int i = 0; i < settings.time; i++) {
            sleep(settings.peroid);
        }
    } else {
        light->trafficDirection = light->controllerDirection;
        StateOutput(light->trafficDirection, &light->outputL1);
        for (int i = 0; i < settings.time; i++) {
            sleep(settings.peroid);
            if (light->train_detected) {
                break; // Exit the loop if a train is detected
            }
        }
    }
}

void CrossCommunicationStateMachine(void *state_ptr, void *state_ptr2) {
    // Implement the cross-communication state machine logic here
    // This function will handle communication between different traffic light nodes
    TrafficLight *L2 = state_ptr2;
    if (L2->train_detected) {
        L2->trafficDirection = TrainLogicNode(L2);
        StateOutput(L2->trafficDirection, &L2->outputL2);
        for (int i = 0; i < settings.time; i++) {
            sleep(settings.peroid);
        }
    } else {
        L2->trafficDirection = TrafficLogicNodeL2(L2);
        StateOutput(L2->trafficDirection, &L2->outputL2);
        for (int i = 0; i < settings.time; i++) {
            sleep(settings.peroid);
            if (L2->train_detected) {
                break; // Exit the loop if a train is detected
            }
        }
        /* Advance after using this group; wrap SE back to NS. */
        L2->trafficDirectionNext = (TrafficStates)((L2->trafficDirectionNext + 1) % 6);
    }
}
void NoControllerStateMachine(void *state_ptr, void *inputs) {
    // Implement the no-controller state machine logic here
    // This function will handle the traffic light behavior when there is no controller
    TrafficLight *light = state_ptr;
    (void)inputs;
    if (light->train_detected) {
        light->trafficDirection = TrainLogicNode(light);
        StateOutput(light->trafficDirection, &light->outputL1);
        for (int i = 0; i < settings.time; i++) {
            sleep(settings.peroid);
        }
    } else {
        light->trafficDirection = TrafficLogicNode(light);
        StateOutput(light->trafficDirection, &light->outputL1);
        for (int i = 0; i < settings.time; i++) {
            sleep(settings.peroid);
            if (light->train_detected) {
                break; // Exit the loop if a train is detected
            }
        }
        /* Advance after using this group; wrap SE back to NS. */
        light->trafficDirectionNext = (TrafficStates)((light->trafficDirectionNext + 1) % 6);
        }

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
    switch (light->trainDirection) {
        case NS:
        case NW:
        case WS:
            return SelectTrafficState(light->trainDirection, light->buttons.Left_NS, light->buttons.Right_NS, light->buttons.Top_EW, light->buttons.Bottom_EW);
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
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/iofunc.h>
#include <sys/netmgr.h>


typedef enum {
    TRAFFIC_GREEN,
    TRAFFIC_YELLOW,
    TRAFFIC_RED
} TrafficLightState;

typedef enum {
    NS, // North-South
    EW, // East-West
    NW, // North-West
    WS, // West-South
    EN, // East-North
    SE, // South-East
} TrafficState;

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
    TrafficLightState trafficState;
    TrafficState trafficDirection;
    Movements output;
    ButtonPresses buttons;
    int train_detected; // Flag to indicate if a train is detected
} TrafficLight;

// Function prototypes
void TrafficLogicNode(void *state_ptr, void *inputs);
void TrainLogicNode(void *state_ptr, void *inputs);


void TrafficLogicNode(void *state_ptr, void *inputs) {

    enum TrafficLight light = *(TrafficLight *)state_ptr;

    while (1) {
        // Simulate traffic light state changes
        switch (light.trafficState) {
            case NS:
                if (light.button.Right_NS) {
                    light.output.NE = 0;
                    light.output.Right_NS = 1;
                } else {
                    light.output.NE = 1;
                    light.output.Right_NS = 0;
                }
                light.output.NS = 1;
                light.output.NW = 0;
                light.output.EN = 0;
                light.output.ES = 0;
                light.output.EW = 0;
                light.output.SN = 1;
                light.output.SE = 0;
                if (light.button.Left_NS) {
                    light.output.SW = 0;
                    light.output.Left_NS = 1;
                } else {
                    light.output.SW = 1;
                    light.output.Left_NS = 0;
                }
                light.output.WN = 0;
                light.output.WE = 0;
                light.output.WS = 0;
                light.output.Top_EW = 0;
                light.output.Bottom_EW = 0;
                break;
            case EW:
                light.output.NE = 0;
                light.output.NS = 0;
                light.output.NW = 0;
                light.output.EN = 0;
                if (light.button.Bottom_EW) {
                    light.output.ES = 0;
                    light.output.Bottom_EW = 1;
                } else {
                    light.output.ES = 1;
                    light.output.Bottom_EW = 0;
                }
                light.output.EW = 1;
                light.output.SN = 0;
                light.output.SE = 0;
                light.output.SW = 0;
                if (light.button.Top_EW) {
                    light.output.WN = 0;
                    light.output.Top_EW = 1;
                } else {
                    light.output.WN = 1;
                    light.output.Top_EW = 0;
                }
                light.output.WE = 1;
                light.output.WS = 0;
                light.output.Left_NS = 0;
                light.output.Right_NS = 0;
                break;
            case NW:
                if (light.button.Right_NS) {
                    light.output.NE = 0;
                    light.output.Right_NS = 1;
                } else {
                    light.output.NE = 1;
                    light.output.Right_NS = 0;
                }
                if (light.button.Bottom_NS) {
                    light.output.NS = 0;
                    light.output.Bottom_NS = 1;
                } else {
                    light.output.NS = 1;
                    light.output.Bottom_NS = 0;
                }
                light.output.NW = 1;
                light.output.EN = 0;
                light.output.ES = 0;
                light.output.EW = 0;
                light.output.SN = 0;
                light.output.SE = 0;
                light.output.SW = 0;
                light.output.WN = 1;
                light.output.WE = 0;
                light.output.WS = 0;
                light.output.Left_NS = 0;
                light.output.Top_EW = 0;
                break;
            case WS:
                light.output.NE = 0;
                light.output.NS = 0;
                light.output.NW = 0;
                light.output.EN = 0;
                light.output.ES = 0;
                light.output.EW = 0;
                light.output.SN = 0;
                light.output.SE = 0;
                light.output.SW = 1;
                if (light.button.Top_EW) {
                    light.output.WN = 0;
                    light.output.Top_EW = 1;
                } else {
                    light.output.WN = 1;
                    light.output.Top_EW = 0;
                }
                if (light.button.Right_NS) {
                    light.output.WE = 0;
                    light.output.Right_NS = 1;
                } else {
                    light.output.WE = 1;
                    light.output.Right_NS = 0;
                }
                light.output.WS = 1;
                light.output.Left_NS = 0;
                light.output.Bottom_EW = 0;
                break;
            case EN:
                light.output.NE = 1;
                light.output.NS = 0;
                light.output.NW = 0;
                light.output.EN = 1;
                if (light.button.Bottom_EW) {
                    light.output.ES = 0;
                    light.output.Bottom_EW = 1;
                } else {
                    light.output.ES = 1;
                    light.output.Bottom_EW = 0;
                }
                if (light.button.Left_NS) {
                    light.output.EW = 0;
                    light.output.Left_NS = 1;
                } else {
                    light.output.EW = 1;
                    light.output.Left_NS = 0;
                }
                light.output.SN = 0;
                light.output.SE = 0;
                light.output.SW = 0;
                light.output.WN = 0;
                light.output.WE = 0;
                light.output.WS = 0;
                light.output.Top_EW = 0;
                light.output.Right_NS = 0;
                break;
            case SE:
                light.output.NE = 0;
                light.output.NS = 0;
                light.output.NW = 0;
                light.output.EN = 0;
                light.output.ES = 1;
                light.output.EW = 0;
                if (light.button.Top_EW) {
                    light.output.SN = 0;
                    light.output.Top_EW = 1;
                } else {
                    light.output.SN = 1;
                    light.output.Top_EW = 0;
                }
                light.output.SE = 1;
                if (light.button.Left_NS) {
                    light.output.SW = 0;
                    light.output.Left_NS = 1;
                } else {
                    light.output.SW = 1;
                    light.output.Left_NS = 0;
                }
                light.output.WN = 0;
                light.output.WE = 0;
                light.output.WS = 0;
                light.output.Bottom_EW = 0;
                light.output.Right_NS = 0;
                break;
            default:
                light.output.NE = 0;
                light.output.NS = 0;
                light.output.NW = 0;
                light.output.EN = 0;
                light.output.ES = 0;
                light.output.EW = 0;
                light.output.SN = 0;
                light.output.SE = 0;
                light.output.SW = 0;
                light.output.WN = 0;
                light.output.WE = 0;
                light.output.WS = 0;
                light.output.Left_NS = 0;
                light.output.Right_NS = 0;
                light.output.Top_EW = 0;
                light.output.Bottom_EW = 0;
                break;
        }
    }
}

void TrainLogicNode(void *state_ptr, void *inputs) {

    enum TrafficLight light = *(TrafficLight *)state_ptr

    switch (light.trafficState) {
        case NS:
            if (light.button.Right_NS) {
                light.output.NE = 0;
                light.output.Right_NS = 1;
            } else {
                light.output.NE = 1;
                light.output.Right_NS = 0;
            }
            light.output.NS = 1;
            light.output.NW = 0;
            light.output.EN = 0;
            light.output.ES = 0;
            light.output.EW = 0;
            light.output.SN = 1;
            light.output.SE = 0;
            if (light.button.Left_NS) {
                light.output.SW = 0;
                light.output.Left_NS = 1;
            } else {
                light.output.SW = 1;
                light.output.Left_NS = 0;
            }
            light.output.WN = 0;
            light.output.WE = 0;
            light.output.WS = 0;
            light.output.Top_EW = 0;
            light.output.Bottom_EW = 0;
            break;
        case NW:
            if (light.button.Right_NS) {
                light.output.NE = 0;
                light.output.Right_NS = 1;
            } else {
                light.output.NE = 1;
                light.output.Right_NS = 0;
            }
            if (light.button.Bottom_NS) {
                light.output.NS = 0;
                light.output.Bottom_NS = 1;
            } else {
                light.output.NS = 1;
                light.output.Bottom_NS = 0;
            }
            light.output.NW = 1;
            light.output.EN = 0;
            light.output.ES = 0;
            light.output.EW = 0;
            light.output.SN = 0;
            light.output.SE = 0;
            light.output.SW = 0;
            light.output.WN = 1;
            light.output.WE = 0;
            light.output.WS = 0;
            light.output.Left_NS = 0;
            light.output.Top_EW = 0;
        case WS:
            light.output.NE = 0;
            light.output.NS = 0;
            light.output.NW = 0;
            light.output.EN = 0;
            light.output.ES = 0;
            light.output.EW = 0;
            light.output.SN = 0;
            light.output.SE = 0;
            light.output.SW = 1;
            if (light.button.Top_EW) {
                light.output.WN = 0;
                light.output.Top_EW = 1;
            } else {
            light.output.WN = 1;
                light.output.Top_EW = 0;
            }
            if (light.button.Right_NS) {
                light.output.WE = 0;
                light.output.Right_NS = 1;
            } else {
                light.output.WE = 1;
                light.output.Right_NS = 0;
            }
            light.output.WS = 1;
            light.output.Left_NS = 0;
            light.output.Bottom_EW = 0;
            break;
        default:
            light.output.NE = 0;
            light.output.NS = 0;
            light.output.NW = 0;
            light.output.EN = 0;
            light.output.ES = 0;
            light.output.EW = 0;
            light.output.SN = 0;
            light.output.SE = 0;
            light.output.SW = 0;
            light.output.WN = 0;
            light.output.WE = 0;
            light.output.WS = 0;
            light.output.Left_NS = 0;
            light.output.Right_NS = 0;
            light.output.Top_EW = 0;
            light.output.Bottom_EW = 0;
            break;
    }
}
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
} TrafficLight;

void TrafficLightNode(void *state_ptr, void *inputs) {

    TrafficLight light = {TRAFFIC_RED, NS, {0}, {0}};

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
                break;
            case EW:
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
                break;
            case NW:
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
                light.output.SW = 0;
                light.output.WN = 0;
                light.output.WE = 0;
                light.output.WS = 0;
                break;
            case EN:
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
                break;
            case SE:
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
                break;
        }

        // Simulate a delay for the traffic light cycle
        sleep(5);
    }
}

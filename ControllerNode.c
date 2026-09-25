#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/iofunc.h>
#include <sys/netmgr.h>

typedef struct {
    int time;
    int peroid;
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
    Movements output;

    // Scores for every variant in each route group.
    int North_South[4];
    int West_South[5];
    int North_West[5];
    int East_West[4];
    int East_North[5];
    int South_East[5];

    RouteIndices best; // Highest-scoring variant index for each group.
} Intersection;

Intersection L1 = {0};
Intersection L2 = {0};
Settings settings = {0};

enum states {L1_NS_and_L2_NS, L1_EW_and_L2_EW, L1_NW_and_L2_SE, L1_WS_and_L2_EN, L1_EN_and_L2_NW, L1_SE_and_L2_EW, L1_SW_and_L2_WS, L1_WE_and_L2_NW, L1_WE_and_L2_WS, L1_EN_and_L2_EW};
enum states CurState = L1_NS_and_L2_NS;
enum states RequestedState = L1_NS_and_L2_NS;
int Decided_route = 0;

int train_detected = 0; // Flag to indicate if a train is detected

/* Example access:
 * L1.priority.NE = 8;
 * L2.output.WE = 1;
 * North_South_route_Case_Statement(L1.best.North_South, &L1);
 */

void Find_Maximum_Index(const int *array, int size, int *max_index);
void Calculate_Route_Scores(Intersection *light);
void TrafficLight_Logics(void *inputs);
void TrafficLight_State_Machine(void *state_ptr, void *inputs);
void Reset_Traffic_Light_Outputs(Intersection *light);
void North_South_route_Case_Statement(int route_index, Intersection *light);
void West_South_route_East_North_route_Case_Statement(int route_index, Intersection *light);
void North_West_route_and_West_north_route_Case_Statement(int route_index, Intersection *light);
void East_West_route_Case_Statement(int route_index, Intersection *light);
void East_to_North_route_Case_Statement(int route_index, Intersection *light);
void South_to_East_route_Case_Statement(int route_index, Intersection *light);
static void Increase_Waiting_Priority(int *priority, int output);
void Update_Waiting_Priorities(Intersection *light);
static void Update_One_Priority(int *priority, int output);


int main(int argc, char *argv[]) {
	printf("Control node running\n");

	pthread_t  th1;
	void *retval;

	// Create and start the thread
	pthread_create (&th1, NULL, StateMachine, NULL);


	pthread_join (th1, &retval);

	printf("Main Controller Terminated....\n");
	return ret;
}

void StateMachine(void *state, void *inputs) {
    enum states currentState = *(enum states *)state;
    while (1) {
        for (int i = 0; i < settings.time; i++) {
            sleep(settings.peroid);
            if (train_detected) {
                // Handle train detection logic here
                break; // Exit the loop if a train is detected
            }
        }
        Update_Waiting_Priorities(&L1);
        Update_Waiting_Priorities(&L2);
        Update_One_Priority(&L1.priority.NE, L1.output.NE);
        Update_One_Priority(&L1.priority.NS, L1.output.NS);
        TrafficLight_Logics(inputs);

        if (currentState != RequestedState) {
            currentState = RequestedState;
            *(enum states *)state = currentState;
        }
        TrafficLight_State_Machine(state, inputs);
    }
}

void Reset_Traffic_Light_Outputs(Intersection *light)
{
    if (light != NULL) {
        light->output = (Movements){0};
    }
}

void TrafficLight_State_Machine(void *state_ptr, void *inputs) {
    (void)inputs;
    Reset_Traffic_Light_Outputs(&L1);
    Reset_Traffic_Light_Outputs(&L2);

    if (state_ptr == NULL) {
        return;
    }

    const enum states *CurrentState = (const enum states *)state_ptr;
    switch (*CurrentState) {
        case L1_NS_and_L2_NS:
            North_South_route_Case_Statement(L1.best.North_South, &L1);
            North_South_route_Case_Statement(L2.best.North_South, &L2);
            break;

        case L1_EW_and_L2_EW:
            East_West_route_Case_Statement(L1.best.East_West, &L1);
            East_West_route_Case_Statement(L2.best.East_West, &L2);
            break;

        case L1_NW_and_L2_SE:
            North_West_route_and_West_north_route_Case_Statement(L1.best.North_West, &L1);
            South_to_East_route_Case_Statement(L2.best.South_East, &L2);
            break;

        case L1_WS_and_L2_EN:
            West_South_route_East_North_route_Case_Statement(L1.best.West_South, &L1);
            East_to_North_route_Case_Statement(L2.best.East_North, &L2);
            break;

        case L1_EN_and_L2_NW:
            East_to_North_route_Case_Statement(L1.best.East_North, &L1);
            North_West_route_and_West_north_route_Case_Statement(L2.best.North_West, &L2);
            break;

        case L1_SE_and_L2_EW:
            South_to_East_route_Case_Statement(L1.best.South_East, &L1);
            East_West_route_Case_Statement(L2.best.East_West, &L2);
            break;

        case L1_SW_and_L2_WS:
            South_to_East_route_Case_Statement(L1.best.South_East, &L1);
            West_South_route_East_North_route_Case_Statement(L2.best.West_South, &L2);
            break;

        case L1_WE_and_L2_NW:
            East_West_route_Case_Statement(L1.best.East_West, &L1);
            North_West_route_and_West_north_route_Case_Statement(L2.best.North_West, &L2);
            break;

        case L1_WE_and_L2_WS:
            East_West_route_Case_Statement(L1.best.East_West, &L1);
            West_South_route_East_North_route_Case_Statement(L2.best.West_South, &L2);
            break;

        case L1_EN_and_L2_EW:
            East_to_North_route_Case_Statement(L1.best.East_North, &L1);
            East_West_route_Case_Statement(L2.best.East_West, &L2);
            break;

        default:
            // Unknown state: all permission outputs remain zero.
            break;
    }
}

void North_South_route_Case_Statement(int route_index, Intersection *light)
{
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

void West_South_route_East_North_route_Case_Statement(int route_index, Intersection *light)
{
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

void Calculate_Route_Scores(Intersection *light)
{
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

void TrafficLight_Logics(void *inputs)
{
    (void)inputs; // Populate L1.priority and L2.priority before calling.
    Calculate_Route_Scores(&L1);
    Calculate_Route_Scores(&L2);

    int TrafficRoutes[10];
    // L1 North South route + L2 North South route
    TrafficRoutes[0] = L1.North_South[L1.best.North_South] + L2.North_South[L2.best.North_South];
    // L1 East West route + L2 East West route
    TrafficRoutes[1] = L1.East_West[L1.best.East_West] + L2.East_West[L2.best.East_West];
    // L1 North West route + L2 South East route
    TrafficRoutes[2] = L1.North_West[L1.best.North_West] + L2.South_East[L2.best.South_East];
    // L1 West South route + L2 East North route
    TrafficRoutes[3] = L1.West_South[L1.best.West_South] + L2.East_North[L2.best.East_North];
    // L1 East North route + L2 North West route
    TrafficRoutes[4] = L1.East_North[L1.best.East_North] + L2.North_West[L2.best.North_West];
    // L1 South to East route + L2 East West route
    TrafficRoutes[5] = L1.South_East[L1.best.South_East] + L2.East_West[L2.best.East_West];
    // L1 South East route + L2 West South route (legacy state label uses SW)
    TrafficRoutes[6] = L1.South_East[L1.best.South_East] + L2.West_South[L2.best.West_South];
    // L1 West East route + L2 North West route
    TrafficRoutes[7] = L1.East_West[L1.best.East_West] + L2.North_West[L2.best.North_West];
    // L1 West East route + L2 West South route
    TrafficRoutes[8] = L1.East_West[L1.best.East_West] + L2.West_South[L2.best.West_South];
    // L1 East North route + L2 East West route
    TrafficRoutes[9] = L1.East_North[L1.best.East_North] + L2.East_West[L2.best.East_West];
    Find_Maximum_Index(TrafficRoutes, 10, &Decided_route);
    RequestedState = (enum states)Decided_route;

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

# define INT_MAX 2147483647

#define MAX_PRIORITY (INT_MAX / 8)

static void Increase_Waiting_Priority(int *priority, int output) {
    // No waiting request, or movement was allowed this cycle.
    if (*priority <= 0 || output != 0) {
        return;
    }

    // Double without overflowing.
    if (*priority > MAX_PRIORITY / 2) {
        *priority = MAX_PRIORITY;
    } else {
        *priority *= 2;
    }
}

void Update_Waiting_Priorities(Intersection *light)
{
    if (light == NULL) {
        return;
    }

    Increase_Waiting_Priority(&light->priority.NE, light->output.NE);
    Increase_Waiting_Priority(&light->priority.NS, light->output.NS);
    Increase_Waiting_Priority(&light->priority.NW, light->output.NW);

    Increase_Waiting_Priority(&light->priority.EN, light->output.EN);
    Increase_Waiting_Priority(&light->priority.ES, light->output.ES);
    Increase_Waiting_Priority(&light->priority.EW, light->output.EW);

    Increase_Waiting_Priority(&light->priority.SN, light->output.SN);
    Increase_Waiting_Priority(&light->priority.SE, light->output.SE);
    Increase_Waiting_Priority(&light->priority.SW, light->output.SW);

    Increase_Waiting_Priority(&light->priority.WN, light->output.WN);
    Increase_Waiting_Priority(&light->priority.WE, light->output.WE);
    Increase_Waiting_Priority(&light->priority.WS, light->output.WS);

    Increase_Waiting_Priority(&light->priority.Left_NS, light->output.Left_NS);
    Increase_Waiting_Priority(&light->priority.Right_NS, light->output.Right_NS);
    Increase_Waiting_Priority(&light->priority.Top_EW, light->output.Top_EW);
    Increase_Waiting_Priority(&light->priority.Bottom_EW, light->output.Bottom_EW);
}

static void Update_One_Priority(int *priority, int output) {
    if (*priority <= 0) {
        return;                  // Nobody waiting
    }

    if (output == 1) {
        *priority = 0;           // Served: clear priority
    } else if (*priority > MAX_PRIORITY / 2) {
        *priority = MAX_PRIORITY;
    } else {
        *priority *= 2;          // Still waiting: double priority
    }
}
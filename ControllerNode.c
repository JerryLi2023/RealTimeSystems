#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/iofunc.h>
#include <sys/netmgr.h>

#define INT_MAX 2147483647
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

    RouteIndices best; // Highest-scoring variant index for each group.
} Intersection;

enum states {L1_NS_and_L2_NS, L1_EW_and_L2_EW, L1_NW_and_L2_SE, L1_WS_and_L2_EN, L1_EN_and_L2_NW, L1_SE_and_L2_EW, L1_SW_and_L2_WS, L1_WE_and_L2_NW, L1_WE_and_L2_WS, L1_EN_and_L2_EW};
enum states CurState = L1_NS_and_L2_NS;
enum states RequestedState = L1_NS_and_L2_NS;

/* Example access:
 * L1.priority.NE = 8;
 * L2.output.WE = 1;
 * North_South_route_Case_Statement(L1.best.North_South, &L1);
 */

 // Send and receive Structs
 // Server side receives data from the client and sends back a reply.
 // Client side sends data to the server and receives a reply
typedef struct {
    struct _pulse hdr;  // Our real data comes after this header
    int ClientID;       // our data (unique id from client)vv
    int NE, NS, NW;
    int EN, ES, EW;
    int SN, SE, SW;
    int WN, WE, WS;
    int Left_NS, Right_NS;
    int Top_EW, Bottom_EW;
} L1_to_Controller_data;
typedef struct {
    struct _pulse hdr;  // Our real data comes after this header
    char buf[BUF_SIZE]; // Message we send back to clients to tell them the messages was processed correctly.
} L1_to_Controller_reply;
typedef struct {
    struct _pulse hdr;  // Our real data comes after this header
    int ClientID;       // our data (unique id from client)vv
    int NE, NS, NW;
    int EN, ES, EW;
    int SN, SE, SW;
    int WN, WE, WS;
    int Left_NS, Right_NS;
    int Top_EW, Bottom_EW;
} Controller_to_L1_data;
typedef struct {
    struct _pulse hdr;  // Our real data comes after this header
    char buf[BUF_SIZE]; // Message we send back to clients to tell them the messages was processed correctly.
} Controller_to_L1_reply;
typedef struct {
    struct _pulse hdr;  // Our real data comes after this header
    int ClientID;       // our data (unique id from client)vv
    int NE, NS, NW;
    int EN, ES, EW;
    int SN, SE, SW;
    int WN, WE, WS;
    int Left_NS, Right_NS;
    int Top_EW, Bottom_EW;
} L2_to_Controller_data;
typedef struct {
    struct _pulse hdr;  // Our real data comes after this header
    char buf[BUF_SIZE]; // Message we send back to clients to tell them the messages was processed correctly.
} L_to_Controller_reply;
typedef struct {
    struct _pulse hdr;  // Our real data comes after this header
    int ClientID;       // our data (unique id from client)vv
    int NE, NS, NW;
    int EN, ES, EW;
    int SN, SE, SW;
    int WN, WE, WS;
    int Left_NS, Right_NS;
    int Top_EW, Bottom_EW;
} Controller_to_L2_data;
typedef struct {
    struct _pulse hdr;  // Our real data comes after this header
    char buf[BUF_SIZE]; // Message we send back to clients to tell them the messages was processed correctly.
} Controller_to_L_reply;
typedef struct {
    struct _pulse hdr;  // Our real data comes after this header
    int ClientID;       // our data (unique id from client)
    int train_detected; // Flag to indicate if a train is detected
    int hardware_error; // Flag to indicate if there is a hardware error
} Train_server_data;

typedef struct {
    struct _pulse hdr;  // Our real data comes after this header
    char buf[BUF_SIZE]; // Message we send back to clients to tell them the messages was processed correctly.
} Train_server_reply;


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
void StateMachine(void *state, void *inputs);
void client_Controller_L1(void *state_ptr);
void client_Controller_L2(void *state_ptr);
void server_Controller_L1(void *state_ptr);
void server_Controller_L2(void *state_ptr);
void server_Controller_Train(void *state_ptr);
void *server_Start_Controller_L1(void *state_ptr);
void *server_Start_Controller_L2(void *state_ptr);
void *client_Start_Controller_L1(void *state_ptr);
void *client_Start_Controller_L2(void *state_ptr);

int main(int argc, char *argv[]) {
	printf("Control node running\n");

	pthread_t  th1, th2, th3, th4, th5;
	void *retval;
    Intersection L1 = {0};
    Intersection L2 = {0};
    Settings settings = {0};

	// Create and start the thread
	pthread_create (&th1, NULL, client_Start_Controller_L1, &L1);
    pthread_create (&th2, NULL, client_Start_Controller_L2, &L2);
    pthread_create (&th3, NULL, server_Start_Controller_L1, &L1);
    pthread_create (&th4, NULL, server_Start_Controller_L2, &L2);
    pthread_create (&th5, NULL, server_Start_Controller_Train, &settings);

    while (1) {
        StateMachine(&L1, &L2, &settings);
    }

	pthread_join (th1, &retval);
    pthread_join (th2, &retval);
    pthread_join (th3, &retval);
    pthread_join (th4, &retval);
    pthread_join (th5, &retval);

	printf("Main Controller Terminated....\n");
	return ret;
}

void StateMachine(void *L1_data, void *L2_data, void *settings_data) {
    struct Intersection L1 = *(struct Intersection *)L1_data;
    struct Intersection L2 = *(struct Intersection *)L2_data;
    struct Settings settings = *(struct Settings *)settings_data;
    enum states currentState;
    for (int i = 0; i < settings.time; i++) {
        sleep(settings.period);
    }
    Update_Waiting_Priorities(&L1);
    Update_Waiting_Priorities(&L2);
    Update_One_Priority(&L1.priority.NE, L1.output.NE);
    Update_One_Priority(&L1.priority.NS, L1.output.NS);
    currentState = (enum states *)TrafficLight_Logics(&L1, &L2);

    TrafficLight_State_Machine(&currentState, &L1, &L2);
}

void Reset_Traffic_Light_Outputs(Intersection *light) {
    if (light != NULL) {
        light->output = (Movements){0};
    }
}

void TrafficLight_State_Machine(void *state_ptr, void *L1_data, void *L2_data) {
    struct Intersection L1 = *(struct Intersection *)L1_data;
    struct Intersection L2 = *(struct Intersection *)L2_data;
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

void *TrafficLight_Logics(void *L1_data, void *L2_data) {
    struct Intersection L1 = *(struct Intersection *)L1_data;
    struct Intersection L2 = *(struct Intersection *)L2_data;

    Calculate_Route_Scores(&L1);
    Calculate_Route_Scores(&L2);

    int TrafficRoutes[10];
    int Decided_route = 0;
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

void Update_Waiting_Priorities(Intersection *light) {
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

void *client_Start_Controller_L1(void *state_ptr) {
    while (1) {
        sleep(2);
        (void)client_Controller_l1(state_ptr);
    }

    return NULL;
}

/*** Client code ***/
int client_Controller_L1(void *state_ptr) {

    struct Intersection L1 = *(struct Intersection *)state_ptr;

    int serverPID;
    int serverCHID;

    FILE *serverFile;

    serverFile = fopen("/tmp/Controller_To_L1.info", "r");

    if (serverFile == NULL) {
        perror("Failed to open /tmp/Controller_To_L1.info");
        return EXIT_FAILURE;
    }

    if (fscanf(serverFile, "%d", &serverPID) != 1) {
        printf("Failed to read server PID\n");
        fclose(serverFile);
        return EXIT_FAILURE;
    }

    if (fscanf(serverFile, "%d", &serverCHID) != 1) {
        printf("Failed to read server channel ID\n");
        fclose(serverFile);
        return EXIT_FAILURE;
    }

    fclose(serverFile);

        printf("Server information loaded from file:\n");
    Controller_to_L1_data msg = {0};
    Controller_to_L1_reply reply = {0};

    msg.ClientID = 500;

    int server_coid;

    printf("   --> Trying to connect (server) process which has a PID: %d\n",   serverPID);
    printf("   --> on channel: %d\n\n", serverCHID);

    // set up message passing channel
    server_coid = ConnectAttach(ND_LOCAL_NODE, serverPID, serverCHID, _NTO_SIDE_CHANNEL, 0);
    if (server_coid == -1)
    {
        printf("\n    ERROR, could not connect to server!\n\n");
        return EXIT_FAILURE;
    }


    printf("Connection established to process with PID:%d, Ch:%d\n", serverPID, serverCHID);

    // We would have pre-defined data to stuff here
    msg.hdr.type = 0x00;
    msg.hdr.subtype = 0x00;

    // Do whatever work you wanted with server connection
    while (1) {
        sleep(1);

        // Write your code
        msg.NE = L1->output->NE;
        msg.NS = L1->output->NS;
        msg.NW = L1->output->NW;
        msg.EN = L1->output->EN;
        msg.ES = L1->output->ES;
        msg.EW = L1->output->EW;
        msg.SN = L1->output->SN;
        msg.SE = L1->output->SE;
        msg.SW = L1->output->SW;
        msg.WN = L1->output->WN;
        msg.WE = L1->output->WE;
        msg.WS = L1->output->WS;
        msg.Left_NS = L1->output->Left_NS;
        msg.Right_NS = L1->output->Right_NS;
        msg.Top_EW = L1->output->Top_EW;
        msg.Bottom_EW = L1->output->Bottom_EW;

        if (MsgSend(server_coid, &msg, sizeof(msg), &reply, sizeof(reply)) == -1) {
            perror("MsgSend");
            break;
        } else { // now process the reply
            printf("   -->Reply is: '%.*s'\n", (int)sizeof(reply.buf), reply.buf);
        }
    }


    // Close the connection
    printf("\n Sending message to server to tell it to close the connection\n");
    ConnectDetach(server_coid);

    return EXIT_SUCCESS;
}

void *server_Start_Controller_L1(void *state_ptr) {
    printf("Server running\n");

    (void)server_Controller_L1(state_ptr);

    printf("Main (Server) Terminated....\n");
    return NULL;
}

/*** Server code ***/
int server_Controller_L1(void *state_ptr) {
    int serverPID=0, chid=0;

    struct Intersection L1 = *(struct Intersection *)state_ptr;

    serverPID = getpid();

    ControllerLight *light = state_ptr;

    // Create Channel
    chid = ChannelCreate(_NTO_CHF_DISCONNECT);
    if (chid == -1)  // _NTO_CHF_DISCONNECT flag used to allow detach
    {
        printf("\nFailed to create communication channel on server\n");
        return EXIT_FAILURE;
    }

    FILE *serverFile;

    serverFile = fopen("/tmp/Controller_To_L1.info", "w");

    if (serverFile == NULL)
    {
        perror("Failed to open /tmp/Controller_To_L1.info");
        ChannelDestroy(chid);
        return EXIT_FAILURE;
    }

    fprintf(serverFile, "%d\n%d\n", serverPID, chid);

    fclose(serverFile);

    printf("Server information written to /tmp/TrafficToPedestrian_L1.info\n");

    printf("Server Listening for Clients on:\n");
    printf("  --> Process ID   : %d \n", serverPID);
    printf("  --> Channel ID   : %d \n\n", chid);

    Pedstrian_server_data msg = {0};
    struct _msg_info info;
    int rcvid=0, msgnum=0;      // no message received yet
    int Stay_alive=0, living=0; // server stays running (ignores _PULSE_CODE_DISCONNECT request)

    Pedstrian_server_reply replymsg = {0};           // replymsg structure for sending back to client
    replymsg.hdr.type = 0x01;
    replymsg.hdr.subtype = 0x00;
    snprintf(replymsg.buf, sizeof(replymsg.buf), "OK");

    living =1;
    while (living)
    {
       // Do your MsgReceive's here now with the chid
       rcvid = MsgReceive(chid, &msg, sizeof(msg), &info);

       if (rcvid == -1)  // Error condition, exit
       {
           if (errno == EINTR) continue;
           perror("MsgReceive");
           break;
       }

       // did we receive a Pulse or message?
       // for Pulses:
       if (rcvid == 0)  //  Pulse received, work out what type
       {
           switch (msg.hdr.code)
           {
               case _PULSE_CODE_DISCONNECT:
                    // A client disconnected all its connections by running
                    // name_close() for each name_open()  or terminated
                   if( Stay_alive == 0)
                   {
                       ConnectDetach(msg.hdr.scoid);
                       printf("\nServer was told to Detach from connection:%d ...\n", msg.hdr.scoid);
                       continue;
                   }
                   else
                   {
                       printf("\nServer received Detach pulse from connection:%d but rejected it ...\n", msg.hdr.scoid);
                   }
                   break;

               case _PULSE_CODE_UNBLOCK:
                    // REPLY blocked client wants to unblock (was hit by a signal
                    // or timed out).  It's up to you if you reply now or later.
                   printf("\nServer got _PULSE_CODE_UNBLOCK after %d, msgnum\n", msgnum);
                   break;

               case _PULSE_CODE_COIDDEATH:  // from the kernel
                   printf("\nServer got _PULSE_CODE_COIDDEATH after %d, msgnum\n", msgnum);
                   break;

               case _PULSE_CODE_THREADDEATH: // from the kernel
                   printf("\nServer got _PULSE_CODE_THREADDEATH after %d, msgnum\n", msgnum);
                   break;

               default:
                   // Some other pulse sent by one of your processes or the kernel
                   printf("\nServer got some other pulse after %d, msgnum\n", msgnum);
                   break;

           }
           continue;// go back to top of while loop
       }

       // for messages:
       if(rcvid > 0) {
           msgnum++;

            // If the Global Name Service (gns) is running, name_open() sends a connect message. The server must EOK it.
            if (msg.hdr.type == _IO_CONNECT )
            {
                MsgReply( rcvid, EOK, NULL, 0 );
                printf("\n gns service is running....");
                continue;   // go back to top of while loop
            }

            // Some other I/O message was received; reject it
            if (msg.hdr.type > _IO_BASE && msg.hdr.type <= _IO_MAX )
            {
                MsgError( rcvid, ENOSYS );
                printf("\n Server received and IO message and rejected it....");
                continue;   // go back to top of while loop
            }

            if ((size_t)info.msglen != sizeof(msg) ||
                (size_t)info.srcmsglen != sizeof(msg)) {
                MsgError(rcvid, EMSGSIZE);
                continue;
            }
            if (msg.hdr.type != 0x00 ||
                (msg.stateChange != 0 && msg.stateChange != 1)) {
                MsgError(rcvid, EINVAL);
                continue;
            }


                L2.input.NE = msg.NE;
                L2.input.NS = msg.NS;
                L2.input.NW = msg.NW;
                L2.input.EN = msg.EN;
                L2.input.ES = msg.ES;
                L2.input.EW = msg.EW;
                L2.input.SN = msg.SN;
                L2.input.SE = msg.SE;
                L2.input.SW = msg.SW;
                L2.input.WN = msg.WN;
                L2.input.WE = msg.WE;
                L2.input.WS = msg.WS;
                L2.input.Left_NS = msg.Left_NS;
                L2.input.Right_NS = msg.Right_NS;
                L2.input.Top_EW = msg.Top_EW;
                L2.input.Bottom_EW = msg.Bottom_EW;

           MsgReply(rcvid, EOK, &replymsg, sizeof(replymsg));
       }
       else
       {
           printf("\nERROR: Server received something, but could not handle it correctly\n");
       }

    }

    printf("\nServer received Destroy command\n");
    // destroyed channel before exiting
    ChannelDestroy(chid);
    unlink("/tmp/TrafficToPedestrian_L1.info");

    return EXIT_FAILURE;
}

void *client_Start_Controller_L2(void *state_ptr) {
    while (1) {
        sleep(2);
        (void)client_Controller_L2(state_ptr);
    }

    return NULL;
}

/*** Client code ***/
int client_Controller_L2(void *state_ptr) {

    int serverPID;
    int serverCHID;

    struct Intersection L2 = *(struct Intersection *)state_ptr;

    FILE *serverFile;

    serverFile = fopen("/tmp/Controller_To_L2.info", "r");

    if (serverFile == NULL) {
        perror("Failed to open /tmp/Controller_To_L2.info");
        return EXIT_FAILURE;
    }

    if (fscanf(serverFile, "%d", &serverPID) != 1) {
        printf("Failed to read server PID\n");
        fclose(serverFile);
        return EXIT_FAILURE;
    }

    if (fscanf(serverFile, "%d", &serverCHID) != 1) {
        printf("Failed to read server channel ID\n");
        fclose(serverFile);
        return EXIT_FAILURE;
    }

    fclose(serverFile);

        printf("Server information loaded from file:\n");
    Controller_to_L2_data msg = {0};
    Controller_to_L2_reply reply = {0};

    msg.ClientID = 500;

    int server_coid;

    printf("   --> Trying to connect (server) process which has a PID: %d\n",   serverPID);
    printf("   --> on channel: %d\n\n", serverCHID);

    // set up message passing channel
    server_coid = ConnectAttach(ND_LOCAL_NODE, serverPID, serverCHID, _NTO_SIDE_CHANNEL, 0);
    if (server_coid == -1)
    {
        printf("\n    ERROR, could not connect to server!\n\n");
        return EXIT_FAILURE;
    }


    printf("Connection established to process with PID:%d, Ch:%d\n", serverPID, serverCHID);

    // We would have pre-defined data to stuff here
    msg.hdr.type = 0x00;
    msg.hdr.subtype = 0x00;

    // Do whatever work you wanted with server connection
    while (1) {
        sleep(1);

        // Write your code
        msg.NE = L2->output->NE;
        msg.NS = L2->output->NS;
        msg.NW = L2->output->NW;
        msg.EN = L2->output->EN;
        msg.ES = L2->output->ES;
        msg.EW = L2->output->EW;
        msg.SN = L2->output->SN;
        msg.SE = L2->output->SE;
        msg.SW = L2->output->SW;
        msg.WN = L2->output->WN;
        msg.WE = L2->output->WE;
        msg.WS = L2->output->WS;
        msg.Left_NS = L2->output->Left_NS;
        msg.Right_NS = L2->output->Right_NS;
        msg.Top_EW = L2->output->Top_EW;
        msg.Bottom_EW = L2->output->Bottom_EW;

        if (MsgSend(server_coid, &msg, sizeof(msg), &reply, sizeof(reply)) == -1) {
            perror("MsgSend");
            break;
        } else { // now process the reply
            printf("   -->Reply is: '%.*s'\n", (int)sizeof(reply.buf), reply.buf);
        }
    }


    // Close the connection
    printf("\n Sending message to server to tell it to close the connection\n");
    ConnectDetach(server_coid);

    return EXIT_SUCCESS;
}

void *server_Start_Controller_L2(void *state_ptr) {
    printf("Server running\n");

    (void)server_Controller_L2(state_ptr);

    printf("Main (Server) Terminated....\n");
    return NULL;
}

/*** Server code ***/
int server_Controller_L2(void *state_ptr) {
    int serverPID=0, chid=0;

    struct Intersection L2 = *(struct Intersection *)state_ptr;

    serverPID = getpid();

    ControllerLight *light = state_ptr;

    // Create Channel
    chid = ChannelCreate(_NTO_CHF_DISCONNECT);
    if (chid == -1)  // _NTO_CHF_DISCONNECT flag used to allow detach
    {
        printf("\nFailed to create communication channel on server\n");
        return EXIT_FAILURE;
    }

    FILE *serverFile;

    serverFile = fopen("/tmp/L2_To_Controller.info", "w");

    if (serverFile == NULL)
    {
        perror("Failed to open /tmp/L2_To_Controller.info");
        ChannelDestroy(chid);
        return EXIT_FAILURE;
    }

    fprintf(serverFile, "%d\n%d\n", serverPID, chid);

    fclose(serverFile);

    printf("Server information written to /tmp/L2_To_Controller.info\n");

    printf("Server Listening for Clients on:\n");
    printf("  --> Process ID   : %d \n", serverPID);
    printf("  --> Channel ID   : %d \n\n", chid);

    Pedstrian_server_data msg = {0};
    struct _msg_info info;
    int rcvid=0, msgnum=0;      // no message received yet
    int Stay_alive=0, living=0; // server stays running (ignores _PULSE_CODE_DISCONNECT request)

    Pedstrian_server_reply replymsg = {0};           // replymsg structure for sending back to client
    replymsg.hdr.type = 0x01;
    replymsg.hdr.subtype = 0x00;
    snprintf(replymsg.buf, sizeof(replymsg.buf), "OK");

    living =1;
    while (living)
    {
       // Do your MsgReceive's here now with the chid
       rcvid = MsgReceive(chid, &msg, sizeof(msg), &info);

       if (rcvid == -1)  // Error condition, exit
       {
           if (errno == EINTR) continue;
           perror("MsgReceive");
           break;
       }

       // did we receive a Pulse or message?
       // for Pulses:
       if (rcvid == 0)  //  Pulse received, work out what type
       {
           switch (msg.hdr.code)
           {
               case _PULSE_CODE_DISCONNECT:
                    // A client disconnected all its connections by running
                    // name_close() for each name_open()  or terminated
                   if( Stay_alive == 0)
                   {
                       ConnectDetach(msg.hdr.scoid);
                       printf("\nServer was told to Detach from connection:%d ...\n", msg.hdr.scoid);
                       continue;
                   }
                   else
                   {
                       printf("\nServer received Detach pulse from connection:%d but rejected it ...\n", msg.hdr.scoid);
                   }
                   break;

               case _PULSE_CODE_UNBLOCK:
                    // REPLY blocked client wants to unblock (was hit by a signal
                    // or timed out).  It's up to you if you reply now or later.
                   printf("\nServer got _PULSE_CODE_UNBLOCK after %d, msgnum\n", msgnum);
                   break;

               case _PULSE_CODE_COIDDEATH:  // from the kernel
                   printf("\nServer got _PULSE_CODE_COIDDEATH after %d, msgnum\n", msgnum);
                   break;

               case _PULSE_CODE_THREADDEATH: // from the kernel
                   printf("\nServer got _PULSE_CODE_THREADDEATH after %d, msgnum\n", msgnum);
                   break;

               default:
                   // Some other pulse sent by one of your processes or the kernel
                   printf("\nServer got some other pulse after %d, msgnum\n", msgnum);
                   break;

           }
           continue;// go back to top of while loop
       }

       // for messages:
       if(rcvid > 0) {
           msgnum++;

            // If the Global Name Service (gns) is running, name_open() sends a connect message. The server must EOK it.
            if (msg.hdr.type == _IO_CONNECT )
            {
                MsgReply( rcvid, EOK, NULL, 0 );
                printf("\n gns service is running....");
                continue;   // go back to top of while loop
            }

            // Some other I/O message was received; reject it
            if (msg.hdr.type > _IO_BASE && msg.hdr.type <= _IO_MAX )
            {
                MsgError( rcvid, ENOSYS );
                printf("\n Server received and IO message and rejected it....");
                continue;   // go back to top of while loop
            }

            if ((size_t)info.msglen != sizeof(msg) ||
                (size_t)info.srcmsglen != sizeof(msg)) {
                MsgError(rcvid, EMSGSIZE);
                continue;
            }
            if (msg.hdr.type != 0x00 ||
                (msg.stateChange != 0 && msg.stateChange != 1)) {
                MsgError(rcvid, EINVAL);
                continue;
            }

            // An unchanged status must not overwrite a pending command.
                L2.input.NE = msg.NE;
                L2.input.NS = msg.NS;
                L2.input.NW = msg.NW;
                L2.input.EN = msg.EN;
                L2.input.ES = msg.ES;
                L2.input.EW = msg.EW;
                L2.input.SN = msg.SN;
                L2.input.SE = msg.SE;
                L2.input.SW = msg.SW;
                L2.input.WN = msg.WN;
                L2.input.WE = msg.WE;
                L2.input.WS = msg.WS;
                L2.input.Left_NS = msg.Left_NS;
                L2.input.Right_NS = msg.Right_NS;
                L2.input.Top_EW = msg.Top_EW;
                L2.input.Bottom_EW = msg.Bottom_EW;


           MsgReply(rcvid, EOK, &replymsg, sizeof(replymsg));
       }
       else
       {
           printf("\nERROR: Server received something, but could not handle it correctly\n");
       }

    }

    printf("\nServer received Destroy command\n");
    // destroyed channel before exiting
    ChannelDestroy(chid);
    unlink("/tmp/TrafficToPedestrian_L1.info");

    return EXIT_FAILURE;
}

void *server_Start_Controller_Train(void *state_ptr) {
    printf("Server running\n");

    (void)server_Controller_Train(state_ptr);

    printf("Main (Server) Terminated....\n");
    return NULL;
}

/*** Server code ***/
int server_Controller_Train(void *state_ptr) {
    int serverPID=0, chid=0;

    serverPID = getpid();

    struct Setting setting = *(struct Setting *)state_ptr;

    // Create Channel
    chid = ChannelCreate(_NTO_CHF_DISCONNECT);
    if (chid == -1)  // _NTO_CHF_DISCONNECT flag used to allow detach
    {
        printf("\nFailed to create communication channel on server\n");
        return EXIT_FAILURE;
    }

    FILE *serverFile;

    serverFile = fopen("/tmp/TrainToController.info", "w");

    if (serverFile == NULL)
    {
        perror("Failed to open /tmp/TrainToController.info");
        ChannelDestroy(chid);
        return EXIT_FAILURE;
    }

    fprintf(serverFile, "%d\n%d\n", serverPID, chid);

    fclose(serverFile);

    printf("Server information written to /tmp/TrainToController.info\n");

    printf("Server Listening for Clients on:\n");
    printf("  --> Process ID   : %d \n", serverPID);
    printf("  --> Channel ID   : %d \n\n", chid);

    Pedstrian_server_data msg = {0};
    struct _msg_info info;
    int rcvid=0, msgnum=0;      // no message received yet
    int Stay_alive=0, living=0; // server stays running (ignores _PULSE_CODE_DISCONNECT request)

    Pedstrian_server_reply replymsg = {0};           // replymsg structure for sending back to client
    replymsg.hdr.type = 0x01;
    replymsg.hdr.subtype = 0x00;
    snprintf(replymsg.buf, sizeof(replymsg.buf), "OK");

    living =1;
    while (living)
    {
       // Do your MsgReceive's here now with the chid
       rcvid = MsgReceive(chid, &msg, sizeof(msg), &info);

       if (rcvid == -1)  // Error condition, exit
       {
           if (errno == EINTR) continue;
           perror("MsgReceive");
           break;
       }

       // did we receive a Pulse or message?
       // for Pulses:
       if (rcvid == 0)  //  Pulse received, work out what type
       {
           switch (msg.hdr.code)
           {
               case _PULSE_CODE_DISCONNECT:
                    // A client disconnected all its connections by running
                    // name_close() for each name_open()  or terminated
                   if( Stay_alive == 0)
                   {
                       ConnectDetach(msg.hdr.scoid);
                       printf("\nServer was told to Detach from connection:%d ...\n", msg.hdr.scoid);
                       continue;
                   }
                   else
                   {
                       printf("\nServer received Detach pulse from connection:%d but rejected it ...\n", msg.hdr.scoid);
                   }
                   break;

               case _PULSE_CODE_UNBLOCK:
                    // REPLY blocked client wants to unblock (was hit by a signal
                    // or timed out).  It's up to you if you reply now or later.
                   printf("\nServer got _PULSE_CODE_UNBLOCK after %d, msgnum\n", msgnum);
                   break;

               case _PULSE_CODE_COIDDEATH:  // from the kernel
                   printf("\nServer got _PULSE_CODE_COIDDEATH after %d, msgnum\n", msgnum);
                   break;

               case _PULSE_CODE_THREADDEATH: // from the kernel
                   printf("\nServer got _PULSE_CODE_THREADDEATH after %d, msgnum\n", msgnum);
                   break;

               default:
                   // Some other pulse sent by one of your processes or the kernel
                   printf("\nServer got some other pulse after %d, msgnum\n", msgnum);
                   break;

           }
           continue;// go back to top of while loop
       }

       // for messages:
       if(rcvid > 0) {
           msgnum++;

            // If the Global Name Service (gns) is running, name_open() sends a connect message. The server must EOK it.
            if (msg.hdr.type == _IO_CONNECT )
            {
                MsgReply( rcvid, EOK, NULL, 0 );
                printf("\n gns service is running....");
                continue;   // go back to top of while loop
            }

            // Some other I/O message was received; reject it
            if (msg.hdr.type > _IO_BASE && msg.hdr.type <= _IO_MAX )
            {
                MsgError( rcvid, ENOSYS );
                printf("\n Server received and IO message and rejected it....");
                continue;   // go back to top of while loop
            }

            if ((size_t)info.msglen != sizeof(msg) ||
                (size_t)info.srcmsglen != sizeof(msg)) {
                MsgError(rcvid, EMSGSIZE);
                continue;
            }
            if (msg.hdr.type != 0x00 ||
                (msg.stateChange != 0 && msg.stateChange != 1)) {
                MsgError(rcvid, EINVAL);
                continue;
            }

            // An unchanged status must not overwrite a pending command.
            setting.train_detected = msg.train_detected;
            setting.hardware_error = msg.hardware_error;


           MsgReply(rcvid, EOK, &replymsg, sizeof(replymsg));
       }
       else
       {
           printf("\nERROR: Server received something, but could not handle it correctly\n");
       }

    }

    printf("\nServer received Destroy command\n");
    // destroyed channel before exiting
    ChannelDestroy(chid);
    unlink("/tmp/TrafficToPedestrian_L1.info");

    return EXIT_FAILURE;
}
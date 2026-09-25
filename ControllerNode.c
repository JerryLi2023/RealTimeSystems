#include <stdbool.h>
    


enum states {State0, State1, State2, State3, State4, State5, State6, State7};
enum states CurState;
// State Logics Global Variables
// L1 Traffic Light Logics
int North_South_route_Without_turn_L1;
int North_South_route_With_NE_turn_L1;
int North_South_route_With_SW_turn_L1;
int North_South_route_With_NE_and_SW_turn_L1;
int West_South_route_East_North_route_With_WN_and_NE_turn_L1;
int West_South_route_East_North_route_With_WN_and_WE_turn_L1;
int West_South_route_East_North_route_With_WN_turn_L1;
int West_South_route_East_North_route_With_WE_turn_L1;
int West_South_route_East_North_route_Without_turn_L1; 
int North_West_route_and_West_north_route_With_NS_and_NE_turn_L1;
int North_West_route_and_West_north_route_With_NS_and_ES_turn_L1;
int North_West_route_and_West_north_route_With_NS_turn_L1;
int North_West_route_and_West_north_route_With_NE_turn_L1;
int North_West_route_and_West_north_route_Without_turn_L1;
int East_West_route_With_WN_and_ES_turn_L1;
int East_West_route_With_WN_turn_L1;
int East_West_route_With_ES_turn_L1;
int East_West_route_Without_turn_L1;
int East_to_North_route_With_SW_and_ES_turn_L1;
int East_to_North_route_With_EW_and_ES_turn_L1;
int East_to_North_route_With_ES_turn_L1;
int East_to_North_route_With_EW_turn_L1;
int East_to_North_route_Without_turn_L1;
int South_to_East_route_With_WN_and_SW_turn_L1;
int South_to_East_route_With_SN_and_SW_turn_L1;
int South_to_East_route_With_SW_turn_L1;
int South_to_East_route_With_SN_turn_L1;
int South_to_East_route_Without_turn_L1;
// L1 Traffic Lights
bool NE_L1 = 0;
bool NS_L1 = 0;
bool NW_L1 = 0;
bool EN_L1 = 0;
bool ES_L1 = 0;
bool EW_L1 = 0;
bool SN_L1 = 0;
bool SE_L1 = 0;
bool SW_L1 = 0;
bool WN_L1 = 0;
bool WE_L1 = 0;
bool WS_L1 = 0;
// L1 Pedestrian Lights
bool Left_NS_L1 = 0;
bool Right_NS_L1 = 0;
bool Top_EW_L1 = 0;
bool Bottom_EW_L1 = 0;
// L2 Traffic Light Logics
int North_South_route_Without_turn_L2;
int North_South_route_With_NE_turn_L2;
int North_South_route_With_SW_turn_L2;
int North_South_route_With_NE_and_SW_turn_L2;
int West_South_route_East_North_route_With_WN_and_NE_turn_L2;
int West_South_route_East_North_route_With_WN_and_WE_turn_L2;
int West_South_route_East_North_route_With_WN_turn_L2;
int West_South_route_East_North_route_With_WE_turn_L2;
int West_South_route_East_North_route_Without_turn_L2; 
int North_West_route_and_West_north_route_With_NS_and_NE_turn_L2;
int North_West_route_and_West_north_route_With_NS_and_ES_turn_L2;
int North_West_route_and_West_north_route_With_NS_turn_L2;
int North_West_route_and_West_north_route_With_NE_turn_L2;
int North_West_route_and_West_north_route_Without_turn_L2;
int East_West_route_With_WN_and_ES_turn_L2;
int East_West_route_With_WN_turn_L2;
int East_West_route_With_ES_turn_L2;
int East_West_route_Without_turn_L2;
int East_to_North_route_With_SW_and_ES_turn_L2;
int East_to_North_route_With_EW_and_ES_turn_L2;
int East_to_North_route_With_ES_turn_L2;
int East_to_North_route_With_EW_turn_L2;
int East_to_North_route_Without_turn_L2;
int South_to_East_route_With_WN_and_SW_turn_L2;
int South_to_East_route_With_SN_and_SW_turn_L2;
int South_to_East_route_With_SW_turn_L2;
int South_to_East_route_With_SN_turn_L2;
int South_to_East_route_Without_turn_L2;
// L2 Traffic Lights
bool NE_L2 = 0;
bool NS_L2 = 0;
bool NW_L2 = 0;
bool EN_L2 = 0;
bool ES_L2 = 0;
bool EW_L2 = 0;
bool SN_L2 = 0;
bool SE_L2 = 0;
bool SW_L2 = 0;
bool WN_L2 = 0;
bool WE_L2 = 0;
bool WS_L2 = 0;
// L2 Pedestrian Lights
bool Left_NS_L2 = 0;
bool Right_NS_L2 = 0;
bool Top_EW_L2 = 0;
bool Bottom_EW_L2 = 0;


    
void SingleStep_TrafficLight_SM(void *state_ptr, void *inputs) {

	enum states *CurrentState = (enum states*) state_ptr;

	char *message = (char *)inputs;

	switch (*CurrentState) {
		case 0:
			// do some work;
			*CurrentState = State1;
			printf ("East and West traffic light: Red | North and South traffic light: Red\n");
			sleep(1);
			break;
		case 1:
			// do some work;
			*CurrentState = State2;
			printf ("East and West traffic light: Red | North and South traffic light: Red\n");
			sleep(1);
			break;
		case 2:
			// do some work;
			printf ("East and West traffic light: Green | North and South traffic light: Red\n");
			if (*message == 'n') {
				*CurrentState = State3;
			} else {
				*CurrentState = State2;
			}
			sleep(2);
			break;
		case 3:
			// do some work;
			*CurrentState = State4;
			printf ("East and West traffic light: Yellow | North and South traffic light: Red\n");
			sleep(1);
			break;
		case 4:
			// do some work;
			*CurrentState = State5;
			printf ("East and West traffic light: Red | North and South traffic light: Red\n");
			sleep(1);
			break;
		case 5:
			// do some work;
			printf ("East and West traffic light: Red | North and South traffic light: Green\n");
			if (*message == 'e') {
				*CurrentState = State6;
			} else {
				*CurrentState = State5;
			}
			sleep(2);
			break;
		case 6:
			// do some work;
			*CurrentState = State7;
			printf ("East and West traffic light: Red | North and South traffic light: Yellow\n");
			sleep(1);
			break;
		case 7:
			// do some work;
			*CurrentState = State0;
			printf ("East and West traffic light: Red | North and South traffic light: Red\n");
			sleep(1);
			break;
	}
}

void TrafficLightLogics (void *state_ptr, void *inputs) {
    enum states *CurrentState = (enum states*) state_ptr;
    char *message = (char *)inputs;

    while (1) {
        SingleStep_TrafficLight_SM(CurrentState, message);
    }
}
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
int North_West_route_and_West_north_route_With_NE_and_ES_turn_L1;
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
int NE_L1 = 0;
int NS_L1 = 0;
int NW_L1 = 0;
int EN_L1 = 0;
int ES_L1 = 0;
int EW_L1 = 0;
int SN_L1 = 0;
int SE_L1 = 0;
int SW_L1 = 0;
int WN_L1 = 0;
int WE_L1 = 0;
int WS_L1 = 0;
// L1 Pedestrian Lights
int Left_NS_L1 = 0;
int Right_NS_L1 = 0;
int Top_EW_L1 = 0;
int Bottom_EW_L1 = 0;
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
int North_West_route_and_West_north_route_With_NE_and_ES_turn_L2;
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
int NE_L2 = 0;
int NS_L2 = 0;
int NW_L2 = 0;
int EN_L2 = 0;
int ES_L2 = 0;
int EW_L2 = 0;
int SN_L2 = 0;
int SE_L2 = 0;
int SW_L2 = 0;
int WN_L2 = 0;
int WE_L2 = 0;
int WS_L2 = 0;
// L2 Pedestrian Lights
int Left_NS_L2 = 0;
int Right_NS_L2 = 0;
int Top_EW_L2 = 0;
int Bottom_EW_L2 = 0;


    
void TrafficLight_Logics(void *state_ptr, void *inputs) {

    // L1 Traffic Light Logics
	North_South_route_Without_turn_L1 = NS_L1 + SN_L1 + Left_NS_L1 + Right_NS_L1; // North South route (Without turn) = NS + SN + PD (Left North South + Right North South) 
    North_South_route_With_NE_turn_L1 = NS_L1 + SN_L1 + NE_L1 + Left_NS_L1; // North South route (With NE turn) = NS + SN + NE + PD(Left North South) 
    North_South_route_With_SW_turn_L1 = NS_L1 + SN_L1 + SW_L1 + Right_NS_L1; // North South route (With SW turn) = NS + SN + SW + PD(Right North South) 
    North_South_route_With_NE_and_SW_turn_L1 = NS_L1 + SN_L1 + NE_L1 + SW_L1; // North South route (With NE and SW turn) NS + SN + NE + SW 
    West_South_route_East_North_route_With_WN_and_NE_turn_L1 = NE_L1 + SW_L1 + WS_L1 + WN_L1; // West South route and South West route (With WN and NE turn) = NE + SW + WS + WN 
    West_South_route_East_North_route_With_WN_and_WE_turn_L1 = WS_L1 + SW_L1 + WN_L1 + WE_L1; // West South route and South West route (With WN and WE turn) = WS + SW + WN + WE 
    West_South_route_East_North_route_With_WN_turn_L1 = SW_L1 + WS_L1 + WN_L1 + Right_NS_L1; // West South route and South West route (With WN turn) = SW + WS + WN + PD(Right North South)
    West_South_route_East_North_route_With_WE_turn_L1 = SW_L1 + WS_L1 + WE_L1 + Top_EW_L1; // West South route and South West route (With WE turn) = SW + WS + WE + PD(Top East West) 
    West_South_route_East_North_route_Without_turn_L1 = SW_L1 + WS_L1 + Top_EW_L1 + Right_NS_L1; // West South route and South West route (Without turn) = SW + WS + PD(Top East West + Right North South) 
    North_West_route_and_West_north_route_With_NS_and_NE_turn_L1 = NW_L1 + WN_L1 + NS_L1 + NE_L1; // North_West route and West north route (With NS, NE) = NW + WN + NS + NE 
    North_West_route_and_West_north_route_With_NE_and_ES_turn_L1 = NW_L1 + WN_L1 + NS_L1 + ES_L1; // North_West route and West north route (With NE, ES) = NW + WN + NE + ES 
    North_West_route_and_West_north_route_With_NE_turn_L1 = NW_L1 + WN_L1 + NE_L1 + Bottom_EW_L1; // North_West route and West north route (With NE) = NW + WN + NE + PD (Bottom East West) 
    North_West_route_and_West_north_route_With_NS_turn_L1 = NW_L1 + WN_L1 + NS_L1 + Right_NS_L1; // North_West route and West north route (With NS) = NW + WN + NS + PD (Right North South) 
    North_West_route_and_West_north_route_Without_turn_L1 = NW_L1 + WN_L1 + Bottom_EW_L1 + Right_NS_L1; // North_West route and West north route (Without) = NW + WN + PD (Bottom East West + Right North South) 
    East_West_route_With_WN_and_ES_turn_L1 = EW_L1 + WE_L1 + WN_L1 + ES_L1; // East West route and West to South (With WN and ES turn) = WN + WE + EW + ES 
    East_West_route_With_WN_turn_L1 = EW_L1 + WE_L1 + WN_L1 + Bottom_EW_L1; // East West route and West to South (With WN) = WN + WE + EW + PD(Bottom East West) 
    East_West_route_With_ES_turn_L1 = EW_L1 + WE_L1 + ES_L1 + Top_EW_L1; // East West route and West to South (With ES) = WE + EW + ES + PD(Top East West) 
    East_West_route_Without_turn_L1 = EW_L1 + WE_L1 + Top_EW_L1 + Bottom_EW_L1; // East West route and West to South (Without turns) = WE + EW + PD(Upper East West + Bottom East West) 
    East_to_North_route_With_SW_and_ES_turn_L1 = NE_L1 + ES_L1 + EN_L1 + SW_L1; // East to North route (With SW and ES turn) = NE + ES + EN + SW 
    East_to_North_route_With_EW_and_ES_turn_L1 = NE_L1 + ES_L1 + EN_L1 + EW_L1; // East to North route (With EW and ES turn) = NE + ES + EN + EW 
    East_to_North_route_With_ES_turn_L1 = NE_L1 + ES_L1 + EN_L1 + Bottom_EW_L1; // East to North route (With ES turn) = NE + ES + EN + PD(Bottom East West)
    East_to_North_route_With_EW_turn_L1 = NE_L1 + EW_L1 + EN_L1 + Bottom_EW_L1; // East to North route (With EW turn) = NE + EW + EN + PD(Bottom East West)
    East_to_North_route_Without_turn_L1 = NE_L1 + Top_EW_L1 + EN_L1 + Bottom_EW_L1; // East to North route (Without turn) = NE + ES + PD(Left North South + Bottom East West) 
    South_to_East_route_With_WN_and_SW_turn_L1 = ES_L1 + SE_L1 + SW_L1 + WN_L1; // South to East route (With WN and SW) = ES + SE + SW + WN 
    South_to_East_route_With_SN_and_SW_turn_L1 = ES_L1 + SE_L1 + SW_L1 + SN_L1; // South to East route (With SN and SW) = ES + SE + SW + SN 
    South_to_East_route_With_SW_turn_L1 = ES_L1 + SE_L1 + SW_L1 + Bottom_EW_L1; // South to East route (With SW turn) = ES + SE + SW + PD(Upper East West) 
    South_to_East_route_With_SN_turn_L1 = ES_L1 + SE_L1 + SN_L1 + Bottom_EW_L1; // South to East route (With SN turn) = ES + SE + SN + PD(Left North South) 
    South_to_East_route_Without_turn_L1 = ES_L1 + SE_L1 + Top_EW_L1 + Bottom_EW_L1; // South to East route (Without turn) = ES + SE + PD(Upper East West + Left North South) 
    // L2 Traffic Light Logics
	North_South_route_Without_turn_L2 = NS_L2 + SN_L2 + Left_NS_L2 + Right_NS_L2; // North South route (Without turn) = NS + SN + PD (Left North South + Right North South) 
    North_South_route_With_NE_turn_L2 = NS_L2 + SN_L2 + NE_L2 + Left_NS_L2; // North South route (With NE turn) = NS + SN + NE + PD(Left North South) 
    North_South_route_With_SW_turn_L2 = NS_L2 + SN_L2 + SW_L2 + Right_NS_L2; // North South route (With SW turn) = NS + SN + SW + PD(Right North South) 
    North_South_route_With_NE_and_SW_turn_L2 = NS_L2 + SN_L2 + NE_L2 + SW_L2; // North South route (With NE and SW turn) NS + SN + NE + SW 
    West_South_route_East_North_route_With_WN_and_NE_turn_L2 = NE_L2 + SW_L2 + WS_L2 + WN_L2; // West South route and South West route (With WN and NE turn) = NE + SW + WS + WN 
    West_South_route_East_North_route_With_WN_and_WE_turn_L2 = WS_L2 + SW_L2 + WN_L2 + WE_L2; // West South route and South West route (With WN and WE turn) = WS + SW + WN + WE 
    West_South_route_East_North_route_With_WN_turn_L2 = SW_L2 + WS_L2 + WN_L2 + Right_NS_L2; // West South route and South West route (With WN turn) = SW + WS + WN + PD(Right North South)
    West_South_route_East_North_route_With_WE_turn_L2 = SW_L2 + WS_L2 + WE_L2 + Top_EW_L2; // West South route and South West route (With WE turn) = SW + WS + WE + PD(Top East West) 
    West_South_route_East_North_route_Without_turn_L2 = SW_L2 + WS_L2 + Top_EW_L2 + Right_NS_L2; // West South route and South West route (Without turn) = SW + WS + PD(Top East West + Right North South) 
    North_West_route_and_West_north_route_With_NS_and_NE_turn_L2 = NW_L2 + WN_L2 + NS_L2 + NE_L2; // North_West route and West north route (With NS, NE) = NW + WN + NS + NE 
    North_West_route_and_West_north_route_With_NE_and_ES_turn_L2 = NW_L2 + WN_L2 + NS_L2 + ES_L2; // North_West route and West north route (With NE, ES) = NW + WN + NE + ES 
    North_West_route_and_West_north_route_With_NE_turn_L2 = NW_L2 + WN_L2 + NE_L2 + Bottom_EW_L2; // North_West route and West north route (With NE) = NW + WN + NE + PD (Bottom East West) 
    North_West_route_and_West_north_route_With_NS_turn_L2 = NW_L2 + WN_L2 + NS_L2 + Right_NS_L2; // North_West route and West north route (With NS) = NW + WN + NS + PD (Right North South) 
    North_West_route_and_West_north_route_Without_turn_L2 = NW_L2 + WN_L2 + Bottom_EW_L2 + Right_NS_L2; // North_West route and West north route (Without) = NW + WN + PD (Bottom East West + Right North South) 
    East_West_route_With_WN_and_ES_turn_L2 = EW_L2 + WE_L2 + WN_L2 + ES_L2; // East West route and West to South (With WN and ES turn) = WN + WE + EW + ES 
    East_West_route_With_WN_turn_L2 = EW_L2 + WE_L2 + WN_L2 + Bottom_EW_L2; // East West route and West to South (With WN) = WN + WE + EW + PD(Bottom East West) 
    East_West_route_With_ES_turn_L2 = EW_L2 + WE_L2 + ES_L2 + Top_EW_L2; // East West route and West to South (With ES) = WE + EW + ES + PD(Top East West) 
    East_to_North_route_With_SW_and_ES_turn_L2 = NE_L2 + ES_L2 + EN_L2 + SW_L2; // East to North route (With SW and ES turn) = NE + ES + EN + SW 
    East_to_North_route_With_EW_and_ES_turn_L2 = NE_L2 + ES_L2 + EN_L2 + EW_L2; // East to North route (With EW and ES turn) = NE + ES + EN + EW 
    East_to_North_route_With_ES_turn_L2 = NE_L2 + ES_L2 + EN_L2 + Bottom_EW_L2; // East to North route (With ES turn) = NE + ES + EN + PD(Bottom East West)
    East_to_North_route_With_EW_turn_L2 = NE_L2 + EW_L2 + EN_L2 + Bottom_EW_L2; // East to North route (With EW turn) = NE + EW + EN + PD(Bottom East West)
    East_to_North_route_Without_turn_L2 = NE_L2 + Top_EW_L2 + EN_L2 + Bottom_EW_L2; // East to North route (Without turn) = NE + ES + PD(Left North South + Bottom East West) 
    South_to_East_route_With_WN_and_SW_turn_L2 = ES_L2 + SE_L2 + SW_L2 + WN_L2; // South to East route (With WN and SW) = ES + SE + SW + WN 
    South_to_East_route_With_SN_and_SW_turn_L2 = ES_L2 + SE_L2 + SW_L2 + SN_L2; // South to East route (With SN and SW) = ES + SE + SW + SN 
    South_to_East_route_With_SW_turn_L2 = ES_L2 + SE_L2 + SW_L2 + Bottom_EW_L2; // South to East route (With SW turn) = ES + SE + SW + PD(Upper East West) 
    South_to_East_route_With_SN_turn_L2 = ES_L2 + SE_L2 + SN_L2 + Bottom_EW_L2; // South to East route (With SN turn) = ES + SE + SN + PD(Left North South) 
    South_to_East_route_Without_turn_L2 = ES_L2 + SE_L2 + Top_EW_L2 + Bottom_EW_L2; // South to East route (Without turn) = ES + SE + PD(Upper East West + Left North South) 
}
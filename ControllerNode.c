#include <stddef.h>

// State Logics Global Variables
// L1 Traffic Light Logics
int North_South_route_L1[4];
int West_South_route_East_North_route_L1[5];
int North_West_route_and_West_north_route_L1[5];
int East_West_route_L1[4];
int East_to_North_route_L1[5];
int South_to_East_route_L1[5];
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
// L1 Traffic Lights Outputs
int NE_L1_Output = 0;
int NS_L1_Output = 0;
int NW_L1_Output = 0;
int EN_L1_Output = 0;
int ES_L1_Output = 0;
int EW_L1_Output = 0;
int SN_L1_Output = 0;
int SE_L1_Output = 0;
int SW_L1_Output = 0;
int WN_L1_Output = 0;
int WE_L1_Output = 0;
int WS_L1_Output = 0;
// L1 Pedestrian Lights
int Left_NS_L1 = 0;
int Right_NS_L1 = 0;
int Top_EW_L1 = 0;
int Bottom_EW_L1 = 0;
// L1 Pedestrian Lights Outputs
int Left_NS_L1_Output = 0;
int Right_NS_L1_Output = 0;
int Top_EW_L1_Output = 0;
int Bottom_EW_L1_Output = 0;
// L2 Traffic Light Logics
int North_South_route_L2[4];
int West_South_route_East_North_route_L2[5];
int North_West_route_and_West_north_route_L2[5];
int East_West_route_L2[4];
int East_to_North_route_L2[5];
int South_to_East_route_L2[5];
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
// L2 Traffic Lights Outputs
int NE_L2_Output = 0;
int NS_L2_Output = 0;
int NW_L2_Output = 0;
int EN_L2_Output = 0;
int ES_L2_Output = 0;
int EW_L2_Output = 0;
int SN_L2_Output = 0;
int SE_L2_Output = 0;
int SW_L2_Output = 0;
int WN_L2_Output = 0;
int WE_L2_Output = 0;
int WS_L2_Output = 0;
// L2 Pedestrian Lights
int Left_NS_L2 = 0;
int Right_NS_L2 = 0;
int Top_EW_L2 = 0;
int Bottom_EW_L2 = 0;
// L2 Pedestrian Lights Outputs
int Left_NS_L2_Output = 0;
int Right_NS_L2_Output = 0;
int Top_EW_L2_Output = 0;
int Bottom_EW_L2_Output = 0;
// State Machine Global Variables
enum states {L1_NS_and_L2_NS, L1_EW_and_L2_EW, L1_NW_and_L2_SE, L1_WS_and_L2_EN, L1_EN_and_L2_NW, L1_SE_and_L2_EW, L1_SW_and_L2_WS, L1_WE_and_L2_NW, L1_WE_and_L2_WS, L1_EN_and_L2_EW};
enum states CurState = L1_NS_and_L2_NS;
enum states RequestedState = L1_NS_and_L2_NS;
int Decided_route = 0;
int North_South_route_L1_max_index = 0;
int West_South_route_East_North_route_L1_max_index = 0;
int North_West_route_and_West_north_route_L1_max_index = 0;
int East_West_route_L1_max_index = 0;
int East_to_North_route_L1_max_index = 0;
int South_to_East_route_L1_max_index = 0;
int North_South_route_L2_max_index = 0;
int West_South_route_East_North_route_L2_max_index = 0;
int North_West_route_and_West_north_route_L2_max_index = 0;
int East_West_route_L2_max_index = 0;
int East_to_North_route_L2_max_index = 0;
int South_to_East_route_L2_max_index = 0;

// Function Prototypes
void Find_Maximum_Index(const int *array, int size, int *max_index);
void TrafficLight_Logics(void *inputs);
void Find_Maximum_Index(const int *array, int size, int *max_index);
void TrafficLight_Logics(void *inputs);
void TrafficLight_State_Machine(void *state_ptr, void *inputs);
void Reset_Traffic_Light_Outputs(int traffic_lights);
void North_South_route_Case_Statement(int route_index, int traffic_lights);
void West_South_route_East_North_route_Case_Statement(int route_index, int traffic_lights);
void North_West_route_and_West_north_route_Case_Statement(int route_index, int traffic_lights);
void East_West_route_Case_Statement(int route_index, int traffic_lights);
void East_to_North_route_Case_Statement(int route_index, int traffic_lights);
void South_to_East_route_Case_Statement(int route_index, int traffic_lights);

void Reset_Traffic_Light_Outputs(int traffic_lights)
{
    if (traffic_lights == 1) {
        NE_L1_Output = 0;
        NS_L1_Output = 0;
        NW_L1_Output = 0;
        EN_L1_Output = 0;
        ES_L1_Output = 0;
        EW_L1_Output = 0;
        SN_L1_Output = 0;
        SE_L1_Output = 0;
        SW_L1_Output = 0;
        WN_L1_Output = 0;
        WE_L1_Output = 0;
        WS_L1_Output = 0;
        Left_NS_L1_Output = 0;
        Right_NS_L1_Output = 0;
        Top_EW_L1_Output = 0;
        Bottom_EW_L1_Output = 0;
    }
    else if (traffic_lights == 2) {
        NE_L2_Output = 0;
        NS_L2_Output = 0;
        NW_L2_Output = 0;
        EN_L2_Output = 0;
        ES_L2_Output = 0;
        EW_L2_Output = 0;
        SN_L2_Output = 0;
        SE_L2_Output = 0;
        SW_L2_Output = 0;
        WN_L2_Output = 0;
        WE_L2_Output = 0;
        WS_L2_Output = 0;
        Left_NS_L2_Output = 0;
        Right_NS_L2_Output = 0;
        Top_EW_L2_Output = 0;
        Bottom_EW_L2_Output = 0;
    }
}

void TrafficLight_State_Machine(void *state_ptr, void *inputs)
{
    (void)inputs;
    Reset_Traffic_Light_Outputs(1);
    Reset_Traffic_Light_Outputs(2);

    if (state_ptr == NULL) {
        return;
    }

    const enum states *CurrentState = (const enum states *)state_ptr;
    switch (*CurrentState) {
        case L1_NS_and_L2_NS:
            North_South_route_Case_Statement(
                North_South_route_L1_max_index, 1);
            North_South_route_Case_Statement(
                North_South_route_L2_max_index, 2);
            break;

        case L1_EW_and_L2_EW:
            East_West_route_Case_Statement(
                East_West_route_L1_max_index, 1);
            East_West_route_Case_Statement(
                East_West_route_L2_max_index, 2);
            break;

        case L1_NW_and_L2_SE:
            North_West_route_and_West_north_route_Case_Statement(
                North_West_route_and_West_north_route_L1_max_index, 1);
            South_to_East_route_Case_Statement(
                South_to_East_route_L2_max_index, 2);
            break;

        case L1_WS_and_L2_EN:
            West_South_route_East_North_route_Case_Statement(
                West_South_route_East_North_route_L1_max_index, 1);
            East_to_North_route_Case_Statement(
                East_to_North_route_L2_max_index, 2);
            break;

        case L1_EN_and_L2_NW:
            East_to_North_route_Case_Statement(
                East_to_North_route_L1_max_index, 1);
            North_West_route_and_West_north_route_Case_Statement(
                North_West_route_and_West_north_route_L2_max_index, 2);
            break;

        case L1_SE_and_L2_EW:
            South_to_East_route_Case_Statement(
                South_to_East_route_L1_max_index, 1);
            East_West_route_Case_Statement(
                East_West_route_L2_max_index, 2);
            break;

        case L1_SW_and_L2_WS:
            South_to_East_route_Case_Statement(
                South_to_East_route_L1_max_index, 1);
            West_South_route_East_North_route_Case_Statement(
                West_South_route_East_North_route_L2_max_index, 2);
            break;

        case L1_WE_and_L2_NW:
            East_West_route_Case_Statement(
                East_West_route_L1_max_index, 1);
            North_West_route_and_West_north_route_Case_Statement(
                North_West_route_and_West_north_route_L2_max_index, 2);
            break;

        case L1_WE_and_L2_WS:
            East_West_route_Case_Statement(
                East_West_route_L1_max_index, 1);
            West_South_route_East_North_route_Case_Statement(
                West_South_route_East_North_route_L2_max_index, 2);
            break;

        case L1_EN_and_L2_EW:
            East_to_North_route_Case_Statement(
                East_to_North_route_L1_max_index, 1);
            East_West_route_Case_Statement(
                East_West_route_L2_max_index, 2);
            break;

        default:
            // Unknown state: all permission outputs remain zero.
            break;
    }
}

void North_South_route_Case_Statement(int route_index, int traffic_lights)
{
    if (traffic_lights != 1 && traffic_lights != 2) {
        return;
    }

    Reset_Traffic_Light_Outputs(traffic_lights);
    switch (route_index) {
        case 0:
            // Enable: NS, SN, Left_NS, Right_NS.
            if (traffic_lights == 1) {
                NS_L1_Output = 1;
                SN_L1_Output = 1;
                Left_NS_L1_Output = 1;
                Right_NS_L1_Output = 1;
            }
            else {
                NS_L2_Output = 1;
                SN_L2_Output = 1;
                Left_NS_L2_Output = 1;
                Right_NS_L2_Output = 1;
            }
            break;

        case 1:
            // Enable: NS, SN, NE, Left_NS.
            if (traffic_lights == 1) {
                NS_L1_Output = 1;
                SN_L1_Output = 1;
                NE_L1_Output = 1;
                Left_NS_L1_Output = 1;
            }
            else {
                NS_L2_Output = 1;
                SN_L2_Output = 1;
                NE_L2_Output = 1;
                Left_NS_L2_Output = 1;
            }
            break;

        case 2:
            // Enable: NS, SN, SW, Right_NS.
            if (traffic_lights == 1) {
                NS_L1_Output = 1;
                SN_L1_Output = 1;
                SW_L1_Output = 1;
                Right_NS_L1_Output = 1;
            }
            else {
                NS_L2_Output = 1;
                SN_L2_Output = 1;
                SW_L2_Output = 1;
                Right_NS_L2_Output = 1;
            }
            break;

        case 3:
            // Enable: NS, SN, NE, SW.
            if (traffic_lights == 1) {
                NS_L1_Output = 1;
                SN_L1_Output = 1;
                NE_L1_Output = 1;
                SW_L1_Output = 1;
            }
            else {
                NS_L2_Output = 1;
                SN_L2_Output = 1;
                NE_L2_Output = 1;
                SW_L2_Output = 1;
            }
            break;

        default:
            // Unknown variant: this intersection remains stopped.
            break;
    }
}

void West_South_route_East_North_route_Case_Statement(int route_index, int traffic_lights)
{
    if (traffic_lights != 1 && traffic_lights != 2) {
        return;
    }

    Reset_Traffic_Light_Outputs(traffic_lights);
    switch (route_index) {
        case 0:
            // Enable: NE, SW, WS, WN.
            if (traffic_lights == 1) {
                NE_L1_Output = 1;
                SW_L1_Output = 1;
                WS_L1_Output = 1;
                WN_L1_Output = 1;
            }
            else {
                NE_L2_Output = 1;
                SW_L2_Output = 1;
                WS_L2_Output = 1;
                WN_L2_Output = 1;
            }
            break;

        case 1:
            // Enable: WS, SW, WN, WE.
            if (traffic_lights == 1) {
                WS_L1_Output = 1;
                SW_L1_Output = 1;
                WN_L1_Output = 1;
                WE_L1_Output = 1;
            }
            else {
                WS_L2_Output = 1;
                SW_L2_Output = 1;
                WN_L2_Output = 1;
                WE_L2_Output = 1;
            }
            break;

        case 2:
            // Enable: SW, WS, WN, Right_NS.
            if (traffic_lights == 1) {
                SW_L1_Output = 1;
                WS_L1_Output = 1;
                WN_L1_Output = 1;
                Right_NS_L1_Output = 1;
            }
            else {
                SW_L2_Output = 1;
                WS_L2_Output = 1;
                WN_L2_Output = 1;
                Right_NS_L2_Output = 1;
            }
            break;

        case 3:
            // Enable: SW, WS, WE, Top_EW.
            if (traffic_lights == 1) {
                SW_L1_Output = 1;
                WS_L1_Output = 1;
                WE_L1_Output = 1;
                Top_EW_L1_Output = 1;
            }
            else {
                SW_L2_Output = 1;
                WS_L2_Output = 1;
                WE_L2_Output = 1;
                Top_EW_L2_Output = 1;
            }
            break;

        case 4:
            // Enable: SW, WS, Top_EW, Right_NS.
            if (traffic_lights == 1) {
                SW_L1_Output = 1;
                WS_L1_Output = 1;
                Top_EW_L1_Output = 1;
                Right_NS_L1_Output = 1;
            }
            else {
                SW_L2_Output = 1;
                WS_L2_Output = 1;
                Top_EW_L2_Output = 1;
                Right_NS_L2_Output = 1;
            }
            break;

        default:
            // Unknown variant: this intersection remains stopped.
            break;
    }
}

void North_West_route_and_West_north_route_Case_Statement(int route_index, int traffic_lights)
{
    if (traffic_lights != 1 && traffic_lights != 2) {
        return;
    }

    Reset_Traffic_Light_Outputs(traffic_lights);
    switch (route_index) {
        case 0:
            // Enable: NW, WN, NS, NE.
            if (traffic_lights == 1) {
                NW_L1_Output = 1;
                WN_L1_Output = 1;
                NS_L1_Output = 1;
                NE_L1_Output = 1;
            }
            else {
                NW_L2_Output = 1;
                WN_L2_Output = 1;
                NS_L2_Output = 1;
                NE_L2_Output = 1;
            }
            break;

        case 1:
            // Enable: NW, WN, NE, ES.
            if (traffic_lights == 1) {
                NW_L1_Output = 1;
                WN_L1_Output = 1;
                NE_L1_Output = 1;
                ES_L1_Output = 1;
            }
            else {
                NW_L2_Output = 1;
                WN_L2_Output = 1;
                NE_L2_Output = 1;
                ES_L2_Output = 1;
            }
            break;

        case 2:
            // Enable: NW, WN, NE, Bottom_EW.
            if (traffic_lights == 1) {
                NW_L1_Output = 1;
                WN_L1_Output = 1;
                NE_L1_Output = 1;
                Bottom_EW_L1_Output = 1;
            }
            else {
                NW_L2_Output = 1;
                WN_L2_Output = 1;
                NE_L2_Output = 1;
                Bottom_EW_L2_Output = 1;
            }
            break;

        case 3:
            // Enable: NW, WN, NS, Right_NS.
            if (traffic_lights == 1) {
                NW_L1_Output = 1;
                WN_L1_Output = 1;
                NS_L1_Output = 1;
                Right_NS_L1_Output = 1;
            }
            else {
                NW_L2_Output = 1;
                WN_L2_Output = 1;
                NS_L2_Output = 1;
                Right_NS_L2_Output = 1;
            }
            break;

        case 4:
            // Enable: NW, WN, Bottom_EW, Right_NS.
            if (traffic_lights == 1) {
                NW_L1_Output = 1;
                WN_L1_Output = 1;
                Bottom_EW_L1_Output = 1;
                Right_NS_L1_Output = 1;
            }
            else {
                NW_L2_Output = 1;
                WN_L2_Output = 1;
                Bottom_EW_L2_Output = 1;
                Right_NS_L2_Output = 1;
            }
            break;

        default:
            // Unknown variant: this intersection remains stopped.
            break;
    }
}

void East_West_route_Case_Statement(int route_index, int traffic_lights)
{
    if (traffic_lights != 1 && traffic_lights != 2) {
        return;
    }

    Reset_Traffic_Light_Outputs(traffic_lights);
    switch (route_index) {
        case 0:
            // Enable: EW, WE, WN, ES.
            if (traffic_lights == 1) {
                EW_L1_Output = 1;
                WE_L1_Output = 1;
                WN_L1_Output = 1;
                ES_L1_Output = 1;
            }
            else {
                EW_L2_Output = 1;
                WE_L2_Output = 1;
                WN_L2_Output = 1;
                ES_L2_Output = 1;
            }
            break;

        case 1:
            // Enable: EW, WE, WN, Bottom_EW.
            if (traffic_lights == 1) {
                EW_L1_Output = 1;
                WE_L1_Output = 1;
                WN_L1_Output = 1;
                Bottom_EW_L1_Output = 1;
            }
            else {
                EW_L2_Output = 1;
                WE_L2_Output = 1;
                WN_L2_Output = 1;
                Bottom_EW_L2_Output = 1;
            }
            break;

        case 2:
            // Enable: EW, WE, ES, Top_EW.
            if (traffic_lights == 1) {
                EW_L1_Output = 1;
                WE_L1_Output = 1;
                ES_L1_Output = 1;
                Top_EW_L1_Output = 1;
            }
            else {
                EW_L2_Output = 1;
                WE_L2_Output = 1;
                ES_L2_Output = 1;
                Top_EW_L2_Output = 1;
            }
            break;

        case 3:
            // Enable: EW, WE, Top_EW, Bottom_EW.
            if (traffic_lights == 1) {
                EW_L1_Output = 1;
                WE_L1_Output = 1;
                Top_EW_L1_Output = 1;
                Bottom_EW_L1_Output = 1;
            }
            else {
                EW_L2_Output = 1;
                WE_L2_Output = 1;
                Top_EW_L2_Output = 1;
                Bottom_EW_L2_Output = 1;
            }
            break;

        default:
            // Unknown variant: this intersection remains stopped.
            break;
    }
}

void East_to_North_route_Case_Statement(int route_index, int traffic_lights)
{
    if (traffic_lights != 1 && traffic_lights != 2) {
        return;
    }

    Reset_Traffic_Light_Outputs(traffic_lights);
    switch (route_index) {
        case 0:
            // Enable: NE, ES, EN, SW.
            if (traffic_lights == 1) {
                NE_L1_Output = 1;
                ES_L1_Output = 1;
                EN_L1_Output = 1;
                SW_L1_Output = 1;
            }
            else {
                NE_L2_Output = 1;
                ES_L2_Output = 1;
                EN_L2_Output = 1;
                SW_L2_Output = 1;
            }
            break;

        case 1:
            // Enable: NE, ES, EN, EW.
            if (traffic_lights == 1) {
                NE_L1_Output = 1;
                ES_L1_Output = 1;
                EN_L1_Output = 1;
                EW_L1_Output = 1;
            }
            else {
                NE_L2_Output = 1;
                ES_L2_Output = 1;
                EN_L2_Output = 1;
                EW_L2_Output = 1;
            }
            break;

        case 2:
            // Enable: NE, ES, EN, Bottom_EW.
            if (traffic_lights == 1) {
                NE_L1_Output = 1;
                ES_L1_Output = 1;
                EN_L1_Output = 1;
                Bottom_EW_L1_Output = 1;
            }
            else {
                NE_L2_Output = 1;
                ES_L2_Output = 1;
                EN_L2_Output = 1;
                Bottom_EW_L2_Output = 1;
            }
            break;

        case 3:
            // Enable: NE, EW, EN, Bottom_EW.
            if (traffic_lights == 1) {
                NE_L1_Output = 1;
                EW_L1_Output = 1;
                EN_L1_Output = 1;
                Bottom_EW_L1_Output = 1;
            }
            else {
                NE_L2_Output = 1;
                EW_L2_Output = 1;
                EN_L2_Output = 1;
                Bottom_EW_L2_Output = 1;
            }
            break;

        case 4:
            // Enable: NE, Top_EW, EN, Bottom_EW.
            if (traffic_lights == 1) {
                NE_L1_Output = 1;
                Top_EW_L1_Output = 1;
                EN_L1_Output = 1;
                Bottom_EW_L1_Output = 1;
            }
            else {
                NE_L2_Output = 1;
                Top_EW_L2_Output = 1;
                EN_L2_Output = 1;
                Bottom_EW_L2_Output = 1;
            }
            break;

        default:
            // Unknown variant: this intersection remains stopped.
            break;
    }
}

void South_to_East_route_Case_Statement(int route_index, int traffic_lights)
{
    if (traffic_lights != 1 && traffic_lights != 2) {
        return;
    }

    Reset_Traffic_Light_Outputs(traffic_lights);
    switch (route_index) {
        case 0:
            // Enable: ES, SE, SW, WN.
            if (traffic_lights == 1) {
                ES_L1_Output = 1;
                SE_L1_Output = 1;
                SW_L1_Output = 1;
                WN_L1_Output = 1;
            }
            else {
                ES_L2_Output = 1;
                SE_L2_Output = 1;
                SW_L2_Output = 1;
                WN_L2_Output = 1;
            }
            break;

        case 1:
            // Enable: ES, SE, SW, SN.
            if (traffic_lights == 1) {
                ES_L1_Output = 1;
                SE_L1_Output = 1;
                SW_L1_Output = 1;
                SN_L1_Output = 1;
            }
            else {
                ES_L2_Output = 1;
                SE_L2_Output = 1;
                SW_L2_Output = 1;
                SN_L2_Output = 1;
            }
            break;

        case 2:
            // Enable: ES, SE, SW, Bottom_EW.
            if (traffic_lights == 1) {
                ES_L1_Output = 1;
                SE_L1_Output = 1;
                SW_L1_Output = 1;
                Bottom_EW_L1_Output = 1;
            }
            else {
                ES_L2_Output = 1;
                SE_L2_Output = 1;
                SW_L2_Output = 1;
                Bottom_EW_L2_Output = 1;
            }
            break;

        case 3:
            // Enable: ES, SE, SN, Bottom_EW.
            if (traffic_lights == 1) {
                ES_L1_Output = 1;
                SE_L1_Output = 1;
                SN_L1_Output = 1;
                Bottom_EW_L1_Output = 1;
            }
            else {
                ES_L2_Output = 1;
                SE_L2_Output = 1;
                SN_L2_Output = 1;
                Bottom_EW_L2_Output = 1;
            }
            break;

        case 4:
            // Enable: ES, SE, Top_EW, Bottom_EW.
            if (traffic_lights == 1) {
                ES_L1_Output = 1;
                SE_L1_Output = 1;
                Top_EW_L1_Output = 1;
                Bottom_EW_L1_Output = 1;
            }
            else {
                ES_L2_Output = 1;
                SE_L2_Output = 1;
                Top_EW_L2_Output = 1;
                Bottom_EW_L2_Output = 1;
            }
            break;

        default:
            // Unknown variant: this intersection remains stopped.
            break;
    }
}

/* state_ptr points to an int that receives the chosen TrafficRoutes index. */
void TrafficLight_Logics(void *inputs) {

    // Calculation Variables
    int Max_indexes_L1[6];
    int Max_indexes_L2[6];
    // L1 Traffic Light Logics
	int North_South_route_Without_turn_L1 = NS_L1 + SN_L1 + Left_NS_L1 + Right_NS_L1; // North South route (Without turn) = NS + SN + PD (Left North South + Right North South) 
    int North_South_route_With_NE_turn_L1 = NS_L1 + SN_L1 + NE_L1 + Left_NS_L1; // North South route (With NE turn) = NS + SN + NE + PD(Left North South) 
    int North_South_route_With_SW_turn_L1 = NS_L1 + SN_L1 + SW_L1 + Right_NS_L1; // North South route (With SW turn) = NS + SN + SW + PD(Right North South) 
    int North_South_route_With_NE_and_SW_turn_L1 = NS_L1 + SN_L1 + NE_L1 + SW_L1; // North South route (With NE and SW turn) NS + SN + NE + SW 
    North_South_route_L1[0] = North_South_route_Without_turn_L1;
    North_South_route_L1[1] = North_South_route_With_NE_turn_L1;
    North_South_route_L1[2] = North_South_route_With_SW_turn_L1;
    North_South_route_L1[3] = North_South_route_With_NE_and_SW_turn_L1;
    Find_Maximum_Index(North_South_route_L1, 4, &Max_indexes_L1[0]);
    int West_South_route_East_North_route_With_WN_and_NE_turn_L1 = NE_L1 + SW_L1 + WS_L1 + WN_L1; // West South route and South West route (With WN and NE turn) = NE + SW + WS + WN 
    int West_South_route_East_North_route_With_WN_and_WE_turn_L1 = WS_L1 + SW_L1 + WN_L1 + WE_L1; // West South route and South West route (With WN and WE turn) = WS + SW + WN + WE 
    int West_South_route_East_North_route_With_WN_turn_L1 = SW_L1 + WS_L1 + WN_L1 + Right_NS_L1; // West South route and South West route (With WN turn) = SW + WS + WN + PD(Right North South)
    int West_South_route_East_North_route_With_WE_turn_L1 = SW_L1 + WS_L1 + WE_L1 + Top_EW_L1; // West South route and South West route (With WE turn) = SW + WS + WE + PD(Top East West) 
    int West_South_route_East_North_route_Without_turn_L1 = SW_L1 + WS_L1 + Top_EW_L1 + Right_NS_L1; // West South route and South West route (Without turn) = SW + WS + PD(Top East West + Right North South) 
    West_South_route_East_North_route_L1[0] = West_South_route_East_North_route_With_WN_and_NE_turn_L1;
    West_South_route_East_North_route_L1[1] = West_South_route_East_North_route_With_WN_and_WE_turn_L1;
    West_South_route_East_North_route_L1[2] = West_South_route_East_North_route_With_WN_turn_L1;
    West_South_route_East_North_route_L1[3] = West_South_route_East_North_route_With_WE_turn_L1;
    West_South_route_East_North_route_L1[4] = West_South_route_East_North_route_Without_turn_L1;
    Find_Maximum_Index(West_South_route_East_North_route_L1, 5, &Max_indexes_L1[1]);
    int North_West_route_and_West_north_route_With_NS_and_NE_turn_L1 = NW_L1 + WN_L1 + NS_L1 + NE_L1; // North_West route and West north route (With NS, NE) = NW + WN + NS + NE 
    int North_West_route_and_West_north_route_With_NE_and_ES_turn_L1 = NW_L1 + WN_L1 + NE_L1 + ES_L1; // North_West route and West north route (With NE, ES) = NW + WN + NE + ES 
    int North_West_route_and_West_north_route_With_NE_turn_L1 = NW_L1 + WN_L1 + NE_L1 + Bottom_EW_L1; // North_West route and West north route (With NE) = NW + WN + NE + PD (Bottom East West) 
    int North_West_route_and_West_north_route_With_NS_turn_L1 = NW_L1 + WN_L1 + NS_L1 + Right_NS_L1; // North_West route and West north route (With NS) = NW + WN + NS + PD (Right North South) 
    int North_West_route_and_West_north_route_Without_turn_L1 = NW_L1 + WN_L1 + Bottom_EW_L1 + Right_NS_L1; // North_West route and West north route (Without) = NW + WN + PD (Bottom East West + Right North South) 
    North_West_route_and_West_north_route_L1[0] = North_West_route_and_West_north_route_With_NS_and_NE_turn_L1;
    North_West_route_and_West_north_route_L1[1] = North_West_route_and_West_north_route_With_NE_and_ES_turn_L1;
    North_West_route_and_West_north_route_L1[2] = North_West_route_and_West_north_route_With_NE_turn_L1;
    North_West_route_and_West_north_route_L1[3] = North_West_route_and_West_north_route_With_NS_turn_L1;
    North_West_route_and_West_north_route_L1[4] = North_West_route_and_West_north_route_Without_turn_L1;
    Find_Maximum_Index(North_West_route_and_West_north_route_L1, 5, &Max_indexes_L1[2]);
    int East_West_route_With_WN_and_ES_turn_L1 = EW_L1 + WE_L1 + WN_L1 + ES_L1; // East West route and West to South (With WN and ES turn) = WN + WE + EW + ES 
    int East_West_route_With_WN_turn_L1 = EW_L1 + WE_L1 + WN_L1 + Bottom_EW_L1; // East West route and West to South (With WN) = WN + WE + EW + PD(Bottom East West) 
    int East_West_route_With_ES_turn_L1 = EW_L1 + WE_L1 + ES_L1 + Top_EW_L1; // East West route and West to South (With ES) = WE + EW + ES + PD(Top East West) 
    int East_West_route_Without_turn_L1 = EW_L1 + WE_L1 + Top_EW_L1 + Bottom_EW_L1; // East West route and West to South (Without turns) = WE + EW + PD(Upper East West + Bottom East West) 
    East_West_route_L1[0] = East_West_route_With_WN_and_ES_turn_L1;
    East_West_route_L1[1] = East_West_route_With_WN_turn_L1;
    East_West_route_L1[2] = East_West_route_With_ES_turn_L1;
    East_West_route_L1[3] = East_West_route_Without_turn_L1;
    Find_Maximum_Index(East_West_route_L1, 4, &Max_indexes_L1[3]);
    int East_to_North_route_With_SW_and_ES_turn_L1 = NE_L1 + ES_L1 + EN_L1 + SW_L1; // East to North route (With SW and ES turn) = NE + ES + EN + SW 
    int East_to_North_route_With_EW_and_ES_turn_L1 = NE_L1 + ES_L1 + EN_L1 + EW_L1; // East to North route (With EW and ES turn) = NE + ES + EN + EW 
    int East_to_North_route_With_ES_turn_L1 = NE_L1 + ES_L1 + EN_L1 + Bottom_EW_L1; // East to North route (With ES turn) = NE + ES + EN + PD(Bottom East West)
    int East_to_North_route_With_EW_turn_L1 = NE_L1 + EW_L1 + EN_L1 + Bottom_EW_L1; // East to North route (With EW turn) = NE + EW + EN + PD(Bottom East West)
    int East_to_North_route_Without_turn_L1 = NE_L1 + Top_EW_L1 + EN_L1 + Bottom_EW_L1; // East to North route (Without turn) = NE + ES + PD(Left North South + Bottom East West) 
    East_to_North_route_L1[0] = East_to_North_route_With_SW_and_ES_turn_L1;
    East_to_North_route_L1[1] = East_to_North_route_With_EW_and_ES_turn_L1;
    East_to_North_route_L1[2] = East_to_North_route_With_ES_turn_L1;
    East_to_North_route_L1[3] = East_to_North_route_With_EW_turn_L1;
    East_to_North_route_L1[4] = East_to_North_route_Without_turn_L1;
    Find_Maximum_Index(East_to_North_route_L1, 5, &Max_indexes_L1[4]);
    int South_to_East_route_With_WN_and_SW_turn_L1 = ES_L1 + SE_L1 + SW_L1 + WN_L1; // South to East route (With WN and SW) = ES + SE + SW + WN 
    int South_to_East_route_With_SN_and_SW_turn_L1 = ES_L1 + SE_L1 + SW_L1 + SN_L1; // South to East route (With SN and SW) = ES + SE + SW + SN 
    int South_to_East_route_With_SW_turn_L1 = ES_L1 + SE_L1 + SW_L1 + Bottom_EW_L1; // South to East route (With SW turn) = ES + SE + SW + PD(Upper East West) 
    int South_to_East_route_With_SN_turn_L1 = ES_L1 + SE_L1 + SN_L1 + Bottom_EW_L1; // South to East route (With SN turn) = ES + SE + SN + PD(Left North South) 
    int South_to_East_route_Without_turn_L1 = ES_L1 + SE_L1 + Top_EW_L1 + Bottom_EW_L1; // South to East route (Without turn) = ES + SE + PD(Upper East West + Left North South) 
    South_to_East_route_L1[0] = South_to_East_route_With_WN_and_SW_turn_L1;
    South_to_East_route_L1[1] = South_to_East_route_With_SN_and_SW_turn_L1;
    South_to_East_route_L1[2] = South_to_East_route_With_SW_turn_L1;
    South_to_East_route_L1[3] = South_to_East_route_With_SN_turn_L1;
    South_to_East_route_L1[4] = South_to_East_route_Without_turn_L1;
    Find_Maximum_Index(South_to_East_route_L1, 5, &Max_indexes_L1[5]);
    // L2 Traffic Light Logics
	int North_South_route_Without_turn_L2 = NS_L2 + SN_L2 + Left_NS_L2 + Right_NS_L2; // North South route (Without turn) = NS + SN + PD (Left North South + Right North South) 
    int North_South_route_With_NE_turn_L2 = NS_L2 + SN_L2 + NE_L2 + Left_NS_L2; // North South route (With NE turn) = NS + SN + NE + PD(Left North South) 
    int North_South_route_With_SW_turn_L2 = NS_L2 + SN_L2 + SW_L2 + Right_NS_L2; // North South route (With SW turn) = NS + SN + SW + PD(Right North South) 
    int North_South_route_With_NE_and_SW_turn_L2 = NS_L2 + SN_L2 + NE_L2 + SW_L2; // North South route (With NE and SW turn) NS + SN + NE + SW 
    North_South_route_L2[0] = North_South_route_Without_turn_L2;
    North_South_route_L2[1] = North_South_route_With_NE_turn_L2;
    North_South_route_L2[2] = North_South_route_With_SW_turn_L2;
    North_South_route_L2[3] = North_South_route_With_NE_and_SW_turn_L2;
    Find_Maximum_Index(North_South_route_L2, 4, &Max_indexes_L2[0]);
    int West_South_route_East_North_route_With_WN_and_NE_turn_L2 = NE_L2 + SW_L2 + WS_L2 + WN_L2; // West South route and South West route (With WN and NE turn) = NE + SW + WS + WN 
    int West_South_route_East_North_route_With_WN_and_WE_turn_L2 = WS_L2 + SW_L2 + WN_L2 + WE_L2; // West South route and South West route (With WN and WE turn) = WS + SW + WN + WE 
    int West_South_route_East_North_route_With_WN_turn_L2 = SW_L2 + WS_L2 + WN_L2 + Right_NS_L2; // West South route and South West route (With WN turn) = SW + WS + WN + PD(Right North South)
    int West_South_route_East_North_route_With_WE_turn_L2 = SW_L2 + WS_L2 + WE_L2 + Top_EW_L2; // West South route and South West route (With WE turn) = SW + WS + WE + PD(Top East West) 
    int West_South_route_East_North_route_Without_turn_L2 = SW_L2 + WS_L2 + Top_EW_L2 + Right_NS_L2; // West South route and South West route (Without turn) = SW + WS + PD(Top East West + Right North South) 
    West_South_route_East_North_route_L2[0] = West_South_route_East_North_route_With_WN_and_NE_turn_L2;
    West_South_route_East_North_route_L2[1] = West_South_route_East_North_route_With_WN_and_WE_turn_L2;
    West_South_route_East_North_route_L2[2] = West_South_route_East_North_route_With_WN_turn_L2;
    West_South_route_East_North_route_L2[3] = West_South_route_East_North_route_With_WE_turn_L2;
    West_South_route_East_North_route_L2[4] = West_South_route_East_North_route_Without_turn_L2;
    Find_Maximum_Index(West_South_route_East_North_route_L2, 5, &Max_indexes_L2[1]);
    int North_West_route_and_West_north_route_With_NS_and_NE_turn_L2 = NW_L2 + WN_L2 + NS_L2 + NE_L2; // North_West route and West north route (With NS, NE) = NW + WN + NS + NE 
    int North_West_route_and_West_north_route_With_NE_and_ES_turn_L2 = NW_L2 + WN_L2 + NE_L2 + ES_L2; // North_West route and West north route (With NE, ES) = NW + WN + NE + ES 
    int North_West_route_and_West_north_route_With_NE_turn_L2 = NW_L2 + WN_L2 + NE_L2 + Bottom_EW_L2; // North_West route and West north route (With NE) = NW + WN + NE + PD (Bottom East West) 
    int North_West_route_and_West_north_route_With_NS_turn_L2 = NW_L2 + WN_L2 + NS_L2 + Right_NS_L2; // North_West route and West north route (With NS) = NW + WN + NS + PD (Right North South) 
    int North_West_route_and_West_north_route_Without_turn_L2 = NW_L2 + WN_L2 + Bottom_EW_L2 + Right_NS_L2; // North_West route and West north route (Without) = NW + WN + PD (Bottom East West + Right North South) 
    North_West_route_and_West_north_route_L2[0] = North_West_route_and_West_north_route_With_NS_and_NE_turn_L2;
    North_West_route_and_West_north_route_L2[1] = North_West_route_and_West_north_route_With_NE_and_ES_turn_L2;
    North_West_route_and_West_north_route_L2[2] = North_West_route_and_West_north_route_With_NE_turn_L2;
    North_West_route_and_West_north_route_L2[3] = North_West_route_and_West_north_route_With_NS_turn_L2;
    North_West_route_and_West_north_route_L2[4] = North_West_route_and_West_north_route_Without_turn_L2;
    Find_Maximum_Index(North_West_route_and_West_north_route_L2, 5, &Max_indexes_L2[2]);
    int East_West_route_With_WN_and_ES_turn_L2 = EW_L2 + WE_L2 + WN_L2 + ES_L2; // East West route and West to South (With WN and ES turn) = WN + WE + EW + ES 
    int East_West_route_With_WN_turn_L2 = EW_L2 + WE_L2 + WN_L2 + Bottom_EW_L2; // East West route and West to South (With WN) = WN + WE + EW + PD(Bottom East West) 
    int East_West_route_With_ES_turn_L2 = EW_L2 + WE_L2 + ES_L2 + Top_EW_L2; // East West route and West to South (With ES) = WE + EW + ES + PD(Top East West) 
    int East_West_route_Without_turn_L2 = EW_L2 + WE_L2 + Top_EW_L2 + Bottom_EW_L2; // East West route and West to South (Without turns) = WE + EW + PD(Upper East West + Bottom East West)
    East_West_route_L2[0] = East_West_route_With_WN_and_ES_turn_L2;
    East_West_route_L2[1] = East_West_route_With_WN_turn_L2;
    East_West_route_L2[2] = East_West_route_With_ES_turn_L2;
    East_West_route_L2[3] = East_West_route_Without_turn_L2;
    Find_Maximum_Index(East_West_route_L2, 4, &Max_indexes_L2[3]);
    int East_to_North_route_With_SW_and_ES_turn_L2 = NE_L2 + ES_L2 + EN_L2 + SW_L2; // East to North route (With SW and ES turn) = NE + ES + EN + SW 
    int East_to_North_route_With_EW_and_ES_turn_L2 = NE_L2 + ES_L2 + EN_L2 + EW_L2; // East to North route (With EW and ES turn) = NE + ES + EN + EW 
    int East_to_North_route_With_ES_turn_L2 = NE_L2 + ES_L2 + EN_L2 + Bottom_EW_L2; // East to North route (With ES turn) = NE + ES + EN + PD(Bottom East West)
    int East_to_North_route_With_EW_turn_L2 = NE_L2 + EW_L2 + EN_L2 + Bottom_EW_L2; // East to North route (With EW turn) = NE + EW + EN + PD(Bottom East West)
    int East_to_North_route_Without_turn_L2 = NE_L2 + Top_EW_L2 + EN_L2 + Bottom_EW_L2; // East to North route (Without turn) = NE + ES + PD(Left North South + Bottom East West) 
    East_to_North_route_L2[0] = East_to_North_route_With_SW_and_ES_turn_L2;
    East_to_North_route_L2[1] = East_to_North_route_With_EW_and_ES_turn_L2;
    East_to_North_route_L2[2] = East_to_North_route_With_ES_turn_L2;
    East_to_North_route_L2[3] = East_to_North_route_With_EW_turn_L2;
    East_to_North_route_L2[4] = East_to_North_route_Without_turn_L2;
    Find_Maximum_Index(East_to_North_route_L2, 5, &Max_indexes_L2[4]);
    int South_to_East_route_With_WN_and_SW_turn_L2 = ES_L2 + SE_L2 + SW_L2 + WN_L2; // South to East route (With WN and SW) = ES + SE + SW + WN 
    int South_to_East_route_With_SN_and_SW_turn_L2 = ES_L2 + SE_L2 + SW_L2 + SN_L2; // South to East route (With SN and SW) = ES + SE + SW + SN 
    int South_to_East_route_With_SW_turn_L2 = ES_L2 + SE_L2 + SW_L2 + Bottom_EW_L2; // South to East route (With SW turn) = ES + SE + SW + PD(Upper East West) 
    int South_to_East_route_With_SN_turn_L2 = ES_L2 + SE_L2 + SN_L2 + Bottom_EW_L2; // South to East route (With SN turn) = ES + SE + SN + PD(Left North South) 
    int South_to_East_route_Without_turn_L2 = ES_L2 + SE_L2 + Top_EW_L2 + Bottom_EW_L2; // South to East route (Without turn) = ES + SE + PD(Upper East West + Left North South) 
    South_to_East_route_L2[0] = South_to_East_route_With_WN_and_SW_turn_L2;
    South_to_East_route_L2[1] = South_to_East_route_With_SN_and_SW_turn_L2;
    South_to_East_route_L2[2] = South_to_East_route_With_SW_turn_L2;
    South_to_East_route_L2[3] = South_to_East_route_With_SN_turn_L2;
    South_to_East_route_L2[4] = South_to_East_route_Without_turn_L2;
    Find_Maximum_Index(South_to_East_route_L2, 5, &Max_indexes_L2[5]);
    int TrafficRoutes[10];
    // L1 North South route + L2 North South route
    TrafficRoutes[0] = North_South_route_L1[Max_indexes_L1[0]] + North_South_route_L2[Max_indexes_L2[0]];
    // L1 East West route + L2 East West route
    TrafficRoutes[1] = East_West_route_L1[Max_indexes_L1[3]] + East_West_route_L2[Max_indexes_L2[3]];
    // L1 North West route + L2 South East route
    TrafficRoutes[2] = North_West_route_and_West_north_route_L1[Max_indexes_L1[2]] + South_to_East_route_L2[Max_indexes_L2[5]];
    // L1 West South route + L2 East North route
    TrafficRoutes[3] = West_South_route_East_North_route_L1[Max_indexes_L1[1]] + East_to_North_route_L2[Max_indexes_L2[4]];
    // L1 East North route + L2 North West route
    TrafficRoutes[4] = East_to_North_route_L1[Max_indexes_L1[4]] + North_West_route_and_West_north_route_L2[Max_indexes_L2[2]];
    // L1 South to East route + L2 East West route
    TrafficRoutes[5] = South_to_East_route_L1[Max_indexes_L1[5]] + East_West_route_L2[Max_indexes_L2[3]];
    // L1 South West route + L2 West South route
    TrafficRoutes[6] = South_to_East_route_L1[Max_indexes_L1[5]] + West_South_route_East_North_route_L2[Max_indexes_L2[1]];
    // L1 West East route + L2 North West route
    TrafficRoutes[7] = East_West_route_L1[Max_indexes_L1[3]] + North_West_route_and_West_north_route_L2[Max_indexes_L2[2]];
    // L1 West East route + L2 West South route
    TrafficRoutes[8] = East_West_route_L1[Max_indexes_L1[3]] + West_South_route_East_North_route_L2[Max_indexes_L2[1]];
    // L1 East North route + L2 East West route
    TrafficRoutes[9] = East_to_North_route_L1[Max_indexes_L1[4]] + East_West_route_L2[Max_indexes_L2[3]];
    Find_Maximum_Index(TrafficRoutes, 10, &Decided_route);
    RequestedState = (enum states)Decided_route;

    // Save L1's selected variants.
    North_South_route_L1_max_index = Max_indexes_L1[0];
    West_South_route_East_North_route_L1_max_index = Max_indexes_L1[1];
    North_West_route_and_West_north_route_L1_max_index = Max_indexes_L1[2];
    East_West_route_L1_max_index = Max_indexes_L1[3];
    East_to_North_route_L1_max_index = Max_indexes_L1[4];
    South_to_East_route_L1_max_index = Max_indexes_L1[5];

    // Save L2's selected variants.
    North_South_route_L2_max_index = Max_indexes_L2[0];
    West_South_route_East_North_route_L2_max_index = Max_indexes_L2[1];
    North_West_route_and_West_north_route_L2_max_index = Max_indexes_L2[2];
    East_West_route_L2_max_index = Max_indexes_L2[3];
    East_to_North_route_L2_max_index = Max_indexes_L2[4];
    South_to_East_route_L2_max_index = Max_indexes_L2[5];
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
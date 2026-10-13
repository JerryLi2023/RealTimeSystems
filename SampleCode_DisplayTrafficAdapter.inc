//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Display Adapter — Existing Light Logic and Existing Channels Stay Unchanged
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------
/* Include this AFTER the TrafficLight definition, BEFORE main. */
static void TrafficDisplaySnapshot(void *data, Display_Message *msg) {
    TrafficLight *light = data;
    const Movements *output = msg->node == DISPLAY_L1 ? &light->outputL1 : &light->outputL2;
    msg->colour = light->trafficState;
    msg->state = light->trafficDirection;
    msg->train_detected = !!light->train_detected;
    msg->active[0] = !!output->NE;
    msg->active[1] = !!output->NS;
    msg->active[2] = !!output->NW;
    msg->active[3] = !!output->EN;
    msg->active[4] = !!output->ES;
    msg->active[5] = !!output->EW;
    msg->active[6] = !!output->SN;
    msg->active[7] = !!output->SE;
    msg->active[8] = !!output->SW;
    msg->active[9] = !!output->WN;
    msg->active[10] = !!output->WE;
    msg->active[11] = !!output->WS;
    msg->active[12] = !!output->Left_NS;
    msg->active[13] = !!output->Right_NS;
    msg->active[14] = !!output->Top_EW;
    msg->active[15] = !!output->Bottom_EW;
    msg->cars[0] = light->storedDirection.NE;
    msg->cars[1] = light->storedDirection.NS;
    msg->cars[2] = light->storedDirection.NW;
    msg->cars[3] = light->storedDirection.EN;
    msg->cars[4] = light->storedDirection.ES;
    msg->cars[5] = light->storedDirection.EW;
    msg->cars[6] = light->storedDirection.SN;
    msg->cars[7] = light->storedDirection.SE;
    msg->cars[8] = light->storedDirection.SW;
    msg->cars[9] = light->storedDirection.WN;
    msg->cars[10] = light->storedDirection.WE;
    msg->cars[11] = light->storedDirection.WS;
    msg->buttons[0] = !!light->buttons.Left_NS;
    msg->buttons[1] = !!light->buttons.Right_NS;
    msg->buttons[2] = !!light->buttons.Top_EW;
    msg->buttons[3] = !!light->buttons.Bottom_EW;
}
static void TrafficDisplayInput(void *data, const Display_Message *msg) {
    TrafficLight *light = data;
    // Only replace car counts. Do not change light states or controller requests.
    light->storedDirection.NE = msg->cars[0];
    light->storedDirection.NS = msg->cars[1];
    light->storedDirection.NW = msg->cars[2];
    light->storedDirection.EN = msg->cars[3];
    light->storedDirection.ES = msg->cars[4];
    light->storedDirection.EW = msg->cars[5];
    light->storedDirection.SN = msg->cars[6];
    light->storedDirection.SE = msg->cars[7];
    light->storedDirection.SW = msg->cars[8];
    light->storedDirection.WN = msg->cars[9];
    light->storedDirection.WE = msg->cars[10];
    light->storedDirection.WS = msg->cars[11];
}

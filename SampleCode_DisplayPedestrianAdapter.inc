//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Display Adapter — Existing Light Logic and Existing Channels Stay Unchanged
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------
/* Include this AFTER the PedstrianLight definition, BEFORE main. */
static void PedestrianDisplaySnapshot(void *data, Display_Message *msg) {
    PedstrianLight *light = data;
    msg->colour = light->lightState;
    msg->state = light->currentState;
    msg->active[12] = !!light->LeftNorthSouthLight;
    msg->buttons[0] = !!light->LeftNorthSouth;
    msg->active[13] = !!light->RightNorthSouthLight;
    msg->buttons[1] = !!light->RightNorthSouth;
    msg->active[14] = !!light->TopEastWestLight;
    msg->buttons[2] = !!light->TopEastWest;
    msg->active[15] = !!light->BottomEastWestLight;
    msg->buttons[3] = !!light->BottomEastWest;
}
static void PedestrianDisplayInput(void *data, const Display_Message *msg) {
    PedstrianLight *light = data;
    // Latch requests like your existing button checker. A zero does not erase a request.
    // The existing pedestrian server clears requests when that crossing is served.
    if (msg->buttons[0]) light->LeftNorthSouth = 1;
    if (msg->buttons[1]) light->RightNorthSouth = 1;
    if (msg->buttons[2]) light->TopEastWest = 1;
    if (msg->buttons[3]) light->BottomEastWest = 1;
}

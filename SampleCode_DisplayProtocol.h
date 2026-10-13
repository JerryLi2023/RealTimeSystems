#ifndef SAMPLECODE_DISPLAY_PROTOCOL_H
#define SAMPLECODE_DISPLAY_PROTOCOL_H
#include <stdint.h>
#include <stddef.h>

//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Universal Display Client/Server Structures — Copy Identically to Every Node
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------
/* Based on Lab5_Task1's padded header, with no pointer in the message.
 * Both ends must use the same byte order. These fields are not serialization. */
typedef union {
    uint32_t sival_int;
    uint32_t dummy[4];
} Display_Sigval;
typedef struct {
    uint16_t type, subtype;
    int8_t code;
    uint8_t zero[3];
    Display_Sigval value;
    uint8_t zero2[2];
    int32_t scoid;
} Display_MessageHeader;

enum { DISPLAY_L1, DISPLAY_L2, DISPLAY_PED_L1, DISPLAY_PED_L2, DISPLAY_TRAIN, DISPLAY_NODE_COUNT };
enum {
    DISPLAY_NE, DISPLAY_NS, DISPLAY_NW, DISPLAY_EN, DISPLAY_ES, DISPLAY_EW,
    DISPLAY_SN, DISPLAY_SE, DISPLAY_SW, DISPLAY_WN, DISPLAY_WE, DISPLAY_WS,
    DISPLAY_LEFT_NS, DISPLAY_RIGHT_NS, DISPLAY_TOP_EW, DISPLAY_BOTTOM_EW
};
#define DISPLAY_DATA_TYPE       0x700 /* Application type outside QNX's reserved I/O range. */
#define DISPLAY_STATUS          1
#define DISPLAY_RANDOM_INPUT    2
#define DISPLAY_BUF_SIZE        100
#define DISPLAY_REPORT_MS       250
#define DISPLAY_INPUT_MS        2000
#define DISPLAY_RETRY_MS        2000
#define DISPLAY_STALE_MS        2000
#define DISPLAY_MAX_RANDOM_CARS 2

/* Status: colour uses the existing node enum: 0=green, 1=yellow/red flash, 2=red.
 * state = TrafficState (traffic) or PedestrianCombination (pedestrian).
 * active[] holds selected movements, NOT actual green lamps: combine with colour.
 * cars[] order: NE NS NW EN ES EW SN SE SW WN WE WS.
 * buttons[] order: Left_NS Right_NS Top_EW Bottom_EW.
 * Input: cars[] replaces traffic counts; buttons[] adds pedestrian requests. */
typedef struct {
    Display_MessageHeader hdr;
    int32_t node;
    int32_t colour, state, train_detected;
    int32_t active[16];
    int32_t cars[12];
    int32_t buttons[4];
} Display_Message;
typedef struct {
    Display_MessageHeader hdr;
    char buf[DISPLAY_BUF_SIZE];
} Display_Reply;

typedef char Display_Header_Check[(sizeof(Display_MessageHeader) == 32) ? 1 : -1];
typedef char Display_Message_Check[(sizeof(Display_Message) == 176 &&
    offsetof(Display_Message, active) == 48) ? 1 : -1];
typedef char Display_Reply_Check[(sizeof(Display_Reply) == 132) ? 1 : -1];

/* The display owns these four status servers. */
static const char *const Display_Status_Names[4] = {
    "Display_From_L1", "Display_From_L2", "Display_From_Pedestrian_L1", "Display_From_Pedestrian_L2"
};
/* Each original node owns one of these five additional input servers. */
static const char *const Display_Input_Names[5] = {
    "Simulator_To_L1", "Simulator_To_L2", "Simulator_To_Pedestrian_L1", "Simulator_To_Pedestrian_L2", "Simulator_To_Train"
};
#endif

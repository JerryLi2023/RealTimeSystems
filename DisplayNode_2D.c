/*
 * SampleCode_DisplayNode_2D.c — display node only.
 * Five QNX status servers: Traffic L1/L2, Pedestrian L1/L2, Train.
 * Five random-input clients: cars, buttons, and infrequent train detections.
 * Browser animation is served by main on TCP port 8080.
 * Build: qcc -std=gnu99 -Wall -Wextra SampleCode_DisplayNode_2D.c -lsocket -o display_2d
 * Run: ./display_2d [L1-input-path L2-input-path PedL1-input-path PedL2-input-path [Train-input-path]]
 * View: http://<DISPLAY_NODE_IP>:8080
 * Original four display protocols are unchanged; Train_To_Display accepts the
 * existing Train_Client_data message (0x23 / subtype 1, state 0/1/2).
 * No car positions or train travel directions exist in these messages:
 * movement animation is illustrative; reported states/counts remain authoritative.
 */
#include <pthread.h>
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
#define DISPLAY_CAR_INPUT_MS    8000
#define DISPLAY_PED_GAP_MIN_MS  20000
#define DISPLAY_PED_GAP_MAX_MS  40000
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
/* Each original node owns one of these four additional input servers. */
static const char *const Display_Input_Names[4] = {
    "Simulator_To_L1", "Simulator_To_L2", "Simulator_To_Pedestrian_L1", "Simulator_To_Pedestrian_L2"
};

typedef void (*Display_ReceiveFunction)(void *, const Display_Message *);

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/dispatch.h>
#include <sys/neutrino.h>
#include <sys/iomsg.h>

uint64_t DisplayNowMs(void) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (uint64_t)now.tv_sec * 1000 + (uint64_t)now.tv_nsec / 1000000;
}
void DisplaySleepMs(unsigned milliseconds) {
    struct timespec delay = {milliseconds / 1000, (milliseconds % 1000) * 1000000L};
    while (nanosleep(&delay, &delay) == -1 && errno == EINTR) {}
}
int DisplayValidMessage(const Display_Message *msg, int node, int command) {
    if (msg->hdr.type != DISPLAY_DATA_TYPE || msg->hdr.subtype != command || msg->node != node)
        return 0;
    for (int i = 0; i < 12; i++)
        if (msg->cars[i] < 0 || msg->cars[i] > 1000) return 0;
    for (int i = 0; i < 4; i++)
        if (msg->buttons[i] != 0 && msg->buttons[i] != 1) return 0;
    if (command == DISPLAY_STATUS) {
        if (msg->colour < 0 || msg->colour > 2 || msg->state < 0 ||
            msg->state > (node < DISPLAY_PED_L1 ? 33 : 15)) return 0;
        if (msg->train_detected != 0 && msg->train_detected != 1) return 0;
        for (int i = 0; i < 16; i++)
            if (msg->active[i] != 0 && msg->active[i] != 1) return 0;
    }
    return 1;
}

//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Named Client — Persistent Connection Is Owned by the Calling Thread
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------
int DisplaySend(int coid, const Display_Message *msg) {
    Display_Reply reply = {0};
    uint64_t timeout_ns = 1000000000ULL;
    if (TimerTimeout(CLOCK_MONOTONIC, _NTO_TIMEOUT_SEND | _NTO_TIMEOUT_REPLY,
                     NULL, &timeout_ns, NULL) == -1) return -1;
    int result = MsgSend(coid, msg, sizeof(*msg), &reply, sizeof(reply));
    int saved_errno = errno;
    /* Disarm after an immediate reply too, before the next blocking operation. */
    TimerTimeout(CLOCK_MONOTONIC, 0, NULL, NULL, NULL);
    errno = saved_errno;
    if (result == -1) return -1;
    if (reply.hdr.type != DISPLAY_DATA_TYPE || reply.hdr.subtype != msg->hdr.subtype) {
        errno = EPROTO;
        return -1;
    }
    return 0;
}

//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Named Server — Same name_attach / MsgReceive / MsgReply Format as the Lab
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------
int DisplayServer(const char *name, int node, int command,
                  Display_ReceiveFunction receive, void *argument) {
    name_attach_t *attach = name_attach(NULL, name, 0);
    if (attach == NULL) return -1;
    for (;;) {
        union { struct _pulse pulse; Display_Message message; } received = {0};
        struct _msg_info info;
        int rcvid = MsgReceive(attach->chid, &received, sizeof(received), &info);
        if (rcvid == -1) {
            if (errno == EINTR) continue;
            int saved_errno = errno;
            name_detach(attach, 0);
            errno = saved_errno;
            return -1;
        }
        if (rcvid == 0) {
            if (received.pulse.code == _PULSE_CODE_DISCONNECT)
                ConnectDetach(received.pulse.scoid);
            else if (received.pulse.code == _PULSE_CODE_UNBLOCK) {
                int waiting = received.pulse.value.sival_int;
                struct _msg_info pending;
                /* Ignore old unblock pulses for a request already replied to. */
                if (MsgInfo(waiting, &pending) != -1 && (pending.flags & _NTO_MI_UNBLOCK_REQ))
                    MsgError(waiting, ETIMEDOUT);
            }
            continue;
        }
        if (info.msglen >= 2 && received.message.hdr.type == _IO_CONNECT) {
            MsgReply(rcvid, EOK, NULL, 0);
            continue;
        }
        if (info.msglen >= 2 && received.message.hdr.type <= _IO_MAX) {
            MsgError(rcvid, ENOSYS);
            continue;
        }
        if (info.srcmsglen != (int)sizeof(Display_Message)) {
            MsgError(rcvid, EMSGSIZE);
            continue;
        }
        /* Qnet may initially deliver only part of a message. Read the rest. */
        if (info.msglen < (int)sizeof(Display_Message) &&
            MsgRead(rcvid, &received.message, sizeof(Display_Message), 0) != (int)sizeof(Display_Message)) {
            MsgError(rcvid, EMSGSIZE);
            continue;
        }
        if (!DisplayValidMessage(&received.message, node, command)) {
            MsgError(rcvid, EINVAL);
            continue;
        }
        receive(argument, &received.message);
        Display_Reply reply = {0};
        reply.hdr.type = DISPLAY_DATA_TYPE;
        reply.hdr.subtype = command;
        snprintf(reply.buf, sizeof(reply.buf), "Received");
        MsgReply(rcvid, EOK, &reply, sizeof(reply));
    }
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/dispatch.h>

//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Display Data — Four Independent Status Links and Four Random-Input Links
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------
typedef struct {
    Display_Message status;
    uint64_t received_ms, input_ack_ms;
    int seen, input_ok, input_error, server_error;
} Display_Slot;
typedef struct {
    pthread_mutex_t mutex;
    Display_Slot slots[DISPLAY_NODE_COUNT];
} Display_Data;
typedef struct {
    int node;
    const char *input_path;
    Display_Data *display;
    uint32_t random_seed; // Private generator state for this client thread.
    uint64_t next_pedestrian_ms; // Next occasional simulated button press.
} Display_ThreadArguments;

static void ReceiveStatus(void *argument, const Display_Message *msg) {
    Display_ThreadArguments *args = argument;
    pthread_mutex_lock(&args->display->mutex);
    Display_Slot *slot = &args->display->slots[args->node];
    slot->status = *msg;
    slot->received_ms = DisplayNowMs();
    slot->seen = 1;
    slot->server_error = 0;
    pthread_mutex_unlock(&args->display->mutex);
}
static void *DisplayServerThread(void *argument) {
    Display_ThreadArguments *args = argument;
    for (;;) {
        DisplayServer(Display_Status_Names[args->node], args->node,
                      DISPLAY_STATUS, ReceiveStatus, args);
        int error = errno;
        pthread_mutex_lock(&args->display->mutex);
        args->display->slots[args->node].server_error = error;
        pthread_mutex_unlock(&args->display->mutex);
        DisplaySleepMs(DISPLAY_RETRY_MS);
    }
    return NULL;
}
static uint32_t NextRandom(uint32_t *seed) {
    uint32_t x = *seed;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    *seed = x;
    return x;
}
static void GenerateInput(Display_ThreadArguments *args, Display_Message *msg) {
    *msg = (Display_Message){0};
    msg->hdr.type = DISPLAY_DATA_TYPE;
    msg->hdr.subtype = DISPLAY_RANDOM_INPUT;
    msg->node = args->node;
    if (args->node < DISPLAY_PED_L1) {
        // Replace the twelve simulated car counts with values from 0 to 2.
        for (int i = 0; i < 12; i++)
            msg->cars[i] = NextRandom(&args->random_seed) % (DISPLAY_MAX_RANDOM_CARS + 1);
    } else {
        // One random crossing press every 20–40 seconds per pedestrian node.
        // Other updates are zero (released); the receiving node latches requests.
        uint64_t now = DisplayNowMs();
        if (args->next_pedestrian_ms && now >= args->next_pedestrian_ms) {
            msg->buttons[NextRandom(&args->random_seed) % 4] = 1;
            args->next_pedestrian_ms = 0;
        }
        if (!args->next_pedestrian_ms)
            args->next_pedestrian_ms = now + DISPLAY_PED_GAP_MIN_MS +
                NextRandom(&args->random_seed) % (DISPLAY_PED_GAP_MAX_MS - DISPLAY_PED_GAP_MIN_MS + 1);
    }
}
static void SetInputResult(Display_ThreadArguments *args, int success, int error) {
    pthread_mutex_lock(&args->display->mutex);
    Display_Slot *slot = &args->display->slots[args->node];
    slot->input_ok = success;
    slot->input_error = error;
    if (success) slot->input_ack_ms = DisplayNowMs();
    pthread_mutex_unlock(&args->display->mutex);
}
static void *DisplayClientThread(void *argument) {
    Display_ThreadArguments *args = argument;
    for (;;) {
        int coid = name_open(args->input_path, 0);
        if (coid == -1) {
            SetInputResult(args, 0, errno);
            DisplaySleepMs(DISPLAY_RETRY_MS);
            continue;
        }
        for (;;) {
            Display_Message msg;
            GenerateInput(args, &msg);
            if (DisplaySend(coid, &msg) == -1) {
                SetInputResult(args, 0, errno);
                break;
            }
            SetInputResult(args, 1, 0);
            DisplaySleepMs(args->node < DISPLAY_PED_L1 ? DISPLAY_CAR_INPUT_MS : DISPLAY_INPUT_MS); // Slower car inputs; pedestrian polling unchanged.
        }
        name_close(coid);
        DisplaySleepMs(DISPLAY_RETRY_MS);
    }
    return NULL;
}

//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Browser Display — Main Serves the Animation and Read-only JSON Status
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdarg.h>
#define DISPLAY_HTTP_PORT 8080

/* The HTML below is embedded, so the QNX executable needs no separate web files. */
static const char Display_HTML[] =
"<!doctype html>\n"
"<html lang=\"en\">\n"
"<head>\n"
"<meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n"
"<title>QNX Traffic · Live 2D</title>\n"
"<style>\n"
":root{color-scheme:dark;--bg:#0b121a;--panel:#111c27;--edge:#273544;--muted:#8da0b4;--ink:#e5edf5;--green:#60e8ab;--amber:#ffc86b;--red:#ff737a}*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--ink);font-family:system-ui,-apple-system,Segoe UI,sans-serif}.wrap{max-width:1480px;margin:auto;padding:26px 32px}header{display:flex;justify-content:space-between;align-items:center;gap:18px;margin-bottom:22px}.eyebrow{color:var(--green);font:600 11px ui-monospace,monospace;letter-spacing:2px}h1{font-size:28px;letter-spacing:-.7px;margin:7px 0 0;font-weight:600}.actions{display:flex;gap:10px;align-items:center}button{border:1px solid #3b4b5b;color:var(--ink);background:#192633;border-radius:7px;padding:10px 14px;cursor:pointer;font:inherit;font-size:12px}button:hover{background:#25394a}button:focus-visible{outline:2px solid var(--green)}.pill{font:600 11px ui-monospace,monospace;border:1px solid var(--edge);border-radius:20px;padding:8px 11px;color:var(--muted)}.pill.live{color:var(--green);border-color:#35654f}.pill.demo{color:var(--amber);border-color:#735d34}.banner{border:1px solid var(--edge);padding:12px 16px;border-radius:8px;color:#a6b7c7;font-size:12px;margin-bottom:18px;background:#121e2a}.banner.demo{border-color:#6b5832;color:#ffcf85}.grid{display:grid;grid-template-columns:1fr 1fr;gap:18px}.panel{background:var(--panel);border:1px solid var(--edge);border-radius:12px;overflow:hidden}.panel-head{padding:17px 19px 13px;display:flex;justify-content:space-between;align-items:center;border-bottom:1px solid var(--edge)}.title{font-size:17px;font-weight:650}.subtitle{font-size:11px;color:var(--muted);margin-top:5px}.phase{font:600 11px ui-monospace,monospace;letter-spacing:.5px;color:var(--muted)}canvas{display:block;width:100%;aspect-ratio:1/1;background:#172a29}.details{padding:13px 17px 15px;background:#101b25}.stats{display:flex;gap:20px;font-size:11px;color:var(--muted);flex-wrap:wrap}.stats b{color:#dce8f2;font-weight:500}.requests{display:flex;gap:6px;margin-top:12px;flex-wrap:wrap}.request{font:10px ui-monospace,monospace;padding:6px 8px;border:1px solid #2c3c4b;border-radius:5px;color:#8da0b4}.request.pending{background:#382e1a;color:var(--amber);border-color:#7f6136}.rail-panel{margin-top:18px}.rail-panel canvas{aspect-ratio:6/1;background:#172a29}.rail-foot{padding:10px 18px;font-size:11px;color:var(--muted)}.legend{display:flex;gap:21px;align-items:center;flex-wrap:wrap;font-size:11px;color:#abbaca;margin:18px 0 10px}.dot{display:inline-block;width:7px;height:7px;border-radius:50%;margin-right:6px}.note{font-size:11px;color:#7f94a7;line-height:1.65}.bottom{display:flex;justify-content:space-between;gap:20px;align-items:flex-start}#error{color:var(--amber)}@media(max-width:760px){.wrap{padding:18px 12px}.grid{grid-template-columns:1fr}h1{font-size:23px}header{align-items:flex-start}.actions{flex-direction:column;align-items:flex-end}.bottom{display:block}}\n"
"\n"
".layout{display:grid;grid-template-columns:minmax(0,900px) 290px;gap:18px;align-items:start;justify-content:center}.map-panel canvas{aspect-ratio:900/1230}.grid{grid-template-columns:1fr;gap:12px}.rail-panel{margin-top:12px}.stats{gap:10px}.panel-head{padding:14px}.title{font-size:15px}.map-key{padding:12px 16px;font-size:12px;color:var(--muted)}@media(max-width:950px){.layout{grid-template-columns:1fr}.sidebar .grid{grid-template-columns:1fr 1fr}}@media(max-width:550px){.sidebar .grid{grid-template-columns:1fr}}\n"
"</style>\n"
"</head>\n"
"<body>\n"
"<div class=\"wrap\">\n"
"<header><div><div class=\"eyebrow\">REAL-TIME SYSTEMS / QNX</div><h1>Traffic in motion</h1></div><div class=\"actions\"><span id=\"mode\" class=\"pill\">WAITING FOR LIVE DATA</span><button id=\"preview\">Preview animation</button><button id=\"fullscreen\">Full screen</button></div></header>\n"
"<div id=\"banner\" class=\"banner\">Live view: waiting for the QNX display node. Open this page through http://DISPLAY_NODE_IP:8080 for live data.</div>\n"
"<div class=\"layout\"><section class=\"panel map-panel\"><div class=\"panel-head\"><div><div class=\"title\">L1 — Railway — L2</div><div class=\"subtitle\">One connected road network · north is up</div></div><span class=\"phase\">R3 CORRIDOR</span></div><canvas id=\"network\" aria-label=\"Whole road network: L1, railway crossing and L2 connected by R3\"></canvas><div class=\"map-key\">Cars follow their permitted turns. Pedestrians walk across the marked crossings.</div></section><aside class=\"sidebar\"><div class=\"grid\" id=\"panels\"></div>\n"
"<section class=\"panel rail-panel\"><div class=\"panel-head\"><div><div class=\"title\">X1 · Railway crossing</div><div class=\"subtitle\">R3 between L1 and L2 · direct report from the train node</div></div><span id=\"trainPhase\" class=\"phase\">NO REPORT</span></div><div class=\"rail-foot\" id=\"trainDetail\">Waiting for Train_To_Display. Train position and travel direction are illustrative; gate position is not reported.</div></section></aside></div>\n"
"<div class=\"bottom\"><div><div class=\"legend\"><span><i class=\"dot\" style=\"background:#60e8ab\"></i>Green / walk</span><span><i class=\"dot\" style=\"background:#ffc86b\"></i>Yellow</span><span><i class=\"dot\" style=\"background:#ff737a\"></i>Red / stop</span><span><i class=\"dot\" style=\"background:#8493a0\"></i>Unknown / stale</span></div><div class=\"note\">North is up · Left-hand traffic · NE = north → east · R3 connects L1, the railway crossing and L2 in one map.<br>Light states and counts come from the nodes. Vehicle positions and pedestrians are illustrative animation, not tracked positions.</div></div><div class=\"note\" id=\"error\"></div></div>\n"
"</div>\n"
"<script>\n"
"'use strict';\n"
"// Shared wire order: NE NS NW EN ES EW SN SE SW WN WE WS; then four crossings.\n"
"const movements=['NE','NS','NW','EN','ES','EW','SN','SE','SW','WN','WE','WS'];\n"
"const crossingNames=['Left NS','Right NS','Top EW','Bottom EW'];\n"
"const colours={green:'#60e8ab',yellow:'#ffc86b',red:'#ff737a',unknown:'#778795'};\n"
"const palette=['#6cbbe7','#f2bc72','#d6e3e8','#d58b8f','#8eccb0','#b99ee2'];\n"
"const STOP=110, START=245, SPEED=36, GAP=28;\n"
"let demo=new URLSearchParams(location.search).get('demo')==='1';\n"
"let packet=null, receivedAt=0, networkOK=false, modeToken=0;\n"
"const byId=id=>document.getElementById(id);\n"
"const panels=[0,1].map(index=>{\n"
"  const el=document.createElement('section');el.className='panel';\n"
"  el.innerHTML=`<div class=\"panel-head\"><div><div class=\"title\">Intersection L${index+1}</div><div class=\"subtitle\">R${index+1} × R3 · traffic + pedestrian node</div></div><span class=\"phase\">NO REPORT</span></div><div class=\"details\"><div class=\"stats\"><span>Route <b class=\"route\">—</b></span><span>Cars <b class=\"cars\">—</b></span><span>Traffic <b class=\"trafficAge\">—</b></span><span>Pedestrian <b class=\"pedAge\">—</b></span><span>Train <b class=\"train\">—</b></span><span>Input <b class=\"input\">—</b></span></div><div class=\"requests\">${crossingNames.map(n=>`<span class=\"request\">${n} · —</span>`).join('')}</div></div>`;\n"
"  byId('panels').append(el);\n"
"  return {index,el,arrivalTimer:1+index,routeCursor:index*5,cars:movements.map(()=>[]),people:[],pedRequests:[0,0,0,0],previousButtons:[0,0,0,0],serial:0};\n"
"});\n"
"const network=byId('network'),mapContext=network.getContext('2d');\n"
"const WORLD_W=900,WORLD_H=1230,centres=[{x:450,y:295},{x:450,y:935}];\n"
"function resize(){network.width=Math.round(network.clientWidth*Math.min(devicePixelRatio||1,2));network.height=Math.round(network.width*WORLD_H/WORLD_W);}\n"
"addEventListener('resize',resize);resize();\n"
"function rotate(p,angle){return [p[0]*Math.cos(angle)-p[1]*Math.sin(angle),p[0]*Math.sin(angle)+p[1]*Math.cos(angle)];}\n"
"const direction={N:0,E:1,S:2,W:3};\n"
"function makePath(code){\n"
"  const origin=direction[code[0]],turn=(direction[code[1]]-origin+4)%4;\n"
"  const lane=turn===1?54:turn===2?34:14;\n"
"  const entry=[lane,-64],exit=turn===1?[64,-54]:turn===2?[lane,64]:[-64,14];\n"
"  const end=turn===1?[START,-54]:turn===2?[lane,START]:[-START,14];\n"
"  const points=[[lane,-START],entry];\n"
"  if(turn===2)points.push(exit);\n"
"  else{const control=[lane,exit[1]];for(let i=1;i<=24;i++){const t=i/24,u=1-t;points.push([u*u*entry[0]+2*u*t*control[0]+t*t*exit[0],u*u*entry[1]+2*u*t*control[1]+t*t*exit[1]]);}}\n"
"  points.push(end);\n"
"  const rotated=points.map(p=>rotate(p,origin*Math.PI/2)),lengths=[0];\n"
"  for(let i=1;i<rotated.length;i++)lengths.push(lengths[i-1]+Math.hypot(rotated[i][0]-rotated[i-1][0],rotated[i][1]-rotated[i-1][1]));\n"
"  return {code,origin,turn,lane,points:rotated,lengths,length:lengths.at(-1)};\n"
"}\n"
"const paths=movements.map(makePath);\n"
"function position(path,s){s=Math.max(0,Math.min(s,path.length));let i=1;while(i<path.lengths.length-1&&path.lengths[i]<s)i++;const a=path.points[i-1],b=path.points[i],f=(s-path.lengths[i-1])/(path.lengths[i]-path.lengths[i-1]);return {x:a[0]+f*(b[0]-a[0]),y:a[1]+f*(b[1]-a[1]),angle:Math.atan2(b[1]-a[1],b[0]-a[0])};}\n"
"const demoTrain={present:true,next:10000};\n"
"// One occasional request per pedestrian node, rather than people every phase.\n"
"const pedestrianGap=()=>20000+Math.random()*20000;\n"
"const demoPedestrians=[0,1].map(()=>({next:performance.now()+pedestrianGap(),buttons:[0,0,0,0]}));\n"
"function demoPacket(now){\n"
"  if(now>=demoTrain.next){demoTrain.present=!demoTrain.present;demoTrain.next=now+(demoTrain.present?8000+Math.random()*4000:60000+Math.random()*60000);}\n"
"  for(const ped of demoPedestrians){\n"
"    if(now>=ped.next){ped.buttons[Math.floor(Math.random()*4)]=1;ped.next=now+pedestrianGap();}\n"
"  }\n"
"  const trainState=demoTrain.present?1:0;\n"
"  return {nodes:[0,1,2,3].map(id=>{\n"
"    const phase=(now/1000+(id%2)*.6)%24;\n"
"    const ns=phase<12,t=phase%12,colour=t<9?0:t<11?1:2;\n"
"    const active=Array(16).fill(0);(ns?[1,6,12,13]:[5,10,14,15]).forEach(i=>active[i]=1);\n"
"    const isPed=id>=2,pc=t<6?0:t<9?1:2;\n"
"    return {id,seen:true,age_ms:0,colour:trainState===2?2:trainState===1&&ns?2:isPed?pc:colour,state:isPed?(ns?3:12):(ns?0:17),train:0,active,cars:movements.map((_,i)=>((i*7+id*11+Math.floor(now/8000))%3)),buttons:[...demoPedestrians[id%2].buttons],input_ok:true,input_age_ms:0};\n"
"  }).concat([{...emptyNode(4),seen:true,state:trainState,train:trainState===1?1:0}])};\n"
"}\n"
"function validPacket(data){return data&&Array.isArray(data.nodes)&&data.nodes.length===5&&data.nodes.every((n,i)=>n&&n.id===i&&typeof n.seen==='boolean'&&Number.isFinite(n.age_ms)&&n.age_ms>=0&&Number.isInteger(n.colour)&&n.colour>=0&&n.colour<=2&&Number.isInteger(n.state)&&n.state>=0&&n.state<=(i<2?33:i<4?15:2)&&Array.isArray(n.active)&&n.active.length===16&&n.active.every(x=>x===0||x===1)&&Array.isArray(n.cars)&&n.cars.length===12&&n.cars.every(x=>Number.isInteger(x)&&x>=0&&x<=1000)&&Array.isArray(n.buttons)&&n.buttons.length===4&&n.buttons.every(x=>x===0||x===1));}\n"
"function emptyNode(id){return {id,seen:false,age_ms:0,colour:2,state:id<2?28:0,train:0,active:Array(16).fill(0),cars:Array(12).fill(0),buttons:Array(4).fill(0),input_ok:false,input_age_ms:0};}\n"
"function getNodes(now){const data=demo?demoPacket(now):(packet||{nodes:[0,1,2,3,4].map(emptyNode)});return data.nodes.map(n=>{const age=n.age_ms+(demo?0:Math.max(0,now-receivedAt));return {...n,age,live:!!n.seen&&age<=2000};});}\n"
"async function poll(){\n"
"  if(!demo && location.protocol!=='file:'){\n"
"    const token=modeToken,controller=new AbortController(),timer=setTimeout(()=>controller.abort(),1200);\n"
"    try{const r=await fetch('/state',{cache:'no-store',signal:controller.signal});if(!r.ok)throw Error('HTTP '+r.status);const data=await r.json();if(!validPacket(data))throw Error('Invalid status data');if(token===modeToken&&!demo){packet=data;receivedAt=performance.now();networkOK=true;}}\n"
"    catch(e){if(token===modeToken){networkOK=false;byId('error').textContent='Connection unavailable. Retrying…';}}\n"
"    finally{clearTimeout(timer);}\n"
"  }\n"
"  setTimeout(poll,250);\n"
"}\n"
"const trainMotion={offset:400,state:null};\n"
"function resetScene(){for(const ped of demoPedestrians){ped.buttons=[0,0,0,0];ped.next=performance.now()+pedestrianGap();}demoTrain.present=true;demoTrain.next=performance.now()+10000;trainMotion.offset=400;trainMotion.state=null;for(const p of panels){p.cars=movements.map(()=>[]);p.people=[];p.pedRequests=[0,0,0,0];p.previousButtons=[0,0,0,0];p.arrivalTimer=1+p.index;}}\n"
"function updateMode(){byId('preview').textContent=demo?'Return to live':'Preview animation';byId('banner').classList.toggle('demo',demo);}\n"
"byId('preview').onclick=()=>{demo=!demo;modeToken++;packet=null;networkOK=false;resetScene();updateMode();};\n"
"byId('fullscreen').onclick=async()=>{try{if(!document.fullscreenElement)await document.documentElement.requestFullscreen();else await document.exitFullscreen();}catch(e){byId('error').textContent='Full screen is unavailable in this preview.';}};\n"
"updateMode();poll();\n"
"\n"
"// Draw a north-up junction with separate straight/turn lanes and left-hand traffic.\n"
"function rect(g,x,y,w,h,c){g.fillStyle=c;g.fillRect(x,y,w,h);}\n"
"function line(g,points,c,w=1,dash=[]){g.beginPath();points.forEach((p,i)=>i?g.lineTo(...p):g.moveTo(...p));g.strokeStyle=c;g.lineWidth=w;g.setLineDash(dash);g.stroke();g.setLineDash([]);}\n"
"function circle(g,x,y,r,c){g.beginPath();g.arc(x,y,r,0,Math.PI*2);g.fillStyle=c;g.fill();}\n"
"function text(g,s,x,y,size=10,c='#b8c8c7',align='center'){g.font=`500 ${size}px system-ui`;g.textAlign=align;g.fillStyle=c;g.fillText(s,x,y);}\n"
"function drawGround(g,p){\n"
"  rect(g,-285,-285,570,570,'#182e2d');\n"
"  for(const sx of [-1,1])for(const sy of [-1,1]){\n"
"    g.save();g.scale(sx,sy);\n"
"    rect(g,84,84,201,201,'#2e3e3b');rect(g,102,103,163,162,'#203c32');\n"
"    rect(g,144,155,103,62,'#24323b');rect(g,139,150,103,62,'#45575d');\n"
"    rect(g,149,159,83,4,'#536b72');rect(g,149,171,83,4,'#536b72');\n"
"    rect(g,132,225,112,5,'#50615a');\n"
"    for(const [x,y] of [[119,133],[248,122],[122,246],[256,252]]){circle(g,x+3,y+4,12,'#162d25');circle(g,x,y,12,'#386449');circle(g,x-3,y-4,6,'#467650');}\n"
"    g.restore();\n"
"  }\n"
"  rect(g,-76,-285,152,570,'#98a29b');rect(g,-285,-76,570,152,'#98a29b');\n"
"  rect(g,-68,-285,136,570,'#34404a');rect(g,-285,-68,570,136,'#34404a');\n"
"  for(let side=0;side<4;side++){\n"
"    g.save();g.rotate(side*Math.PI/2);\n"
"    line(g,[[0,-285],[0,-101]],'#d9bd74',2);\n"
"    for(const x of [-44,-24,24,44])line(g,[[x,-285],[x,-120]],'#82909b',1,[9,11]);\n"
"    // Stop bar before the zebra crossing, across the three incoming lanes.\n"
"    rect(g,4,-123,62,3,'#edf0dd');\n"
"    for(let x=-64;x<64;x+=13)rect(g,x,-99,7,17,'#c8d1c8');\n"
"    \n"
"    text(g,side%2===0?'R3':`R${p.index+1}`,-40,-225,10,'#a9bbc7');\n"
"    for(const path of paths.filter(x=>x.origin===side)){\n"
"      const x=path.lane;let endX=x,endY=-178;\n"
"      const pts=[[x,-200],[x,-178]];if(path.turn!==2){endX+=path.turn===1?7:-7;pts.push([endX,endY]);}\n"
"      line(g,pts,'#9dacb7',1.5);\n"
"      const a=path.turn===2?Math.PI/2:path.turn===1?0:Math.PI;\n"
"      line(g,[[endX-5*Math.cos(a-.6),endY-5*Math.sin(a-.6)],[endX,endY],[endX-5*Math.cos(a+.6),endY-5*Math.sin(a+.6)]],'#9dacb7',1.5);\n"
"    }\n"
"    g.restore();\n"
"    const label=rotate([-13,-259],side*Math.PI/2);text(g,['N','E','S','W'][side],label[0],label[1],14,'#f0efe2');\n"
"  }\n"
"  // Keep the interior clear; turning paths are drawn by the moving vehicles.\n"
"  rect(g,-67,-67,134,134,'#34404a');\n"
"  text(g,`L${p.index+1}`,0,5,20,'#596875');\n"
"}\n"
"function drawSignal(g,path,phase,live){\n"
"  g.save();g.rotate(path.origin*Math.PI/2);const x=path.lane,y=-112;\n"
"  rect(g,x-9,y-4,18,8,'#111a20');\n"
"  for(let i=0;i<3;i++)circle(g,x-5+i*5,y,1.8,live&&phase===i?[colours.green,colours.yellow,colours.red][i]:'#46505b');\n"
"  if(!live)circle(g,x,y,2,colours.unknown);\n"
"  g.restore();\n"
"}\n"
"function drawCar(g,car,path){\n"
"  if(car.s<0||car.s>path.length)return;\n"
"  const pos=position(path,car.s);g.save();g.translate(pos.x,pos.y);g.rotate(pos.angle);\n"
"  rect(g,-11,-5,24,12,'#162029');rect(g,-11,-6,22,11,car.colour);\n"
"  rect(g,2,-4,4,7,'#274657');rect(g,-7,-4,3,7,'#3c5965');\n"
"  rect(g,-1,-4,2,7,car.colour);rect(g,9,-5,2,2,'#f8e7b7');rect(g,9,2,2,2,'#f8e7b7');\n"
"  rect(g,-11,-5,2,2,'#dc6f69');rect(g,-11,2,2,2,'#dc6f69');g.restore();\n"
"}\n"
"// Gradually admit one car every 2.5–5 seconds per intersection.\n"
"// Counts remain reported demand; animation is not a measured vehicle tracker.\n"
"function updateCars(p,status,dt){\n"
"  if(!status.live)return;\n"
"  p.arrivalTimer-=dt;\n"
"  if(p.arrivalTimer<=0){\n"
"    for(let step=0;step<12;step++){\n"
"      const i=(p.routeCursor+step)%12,cars=p.cars[i];\n"
"      if(cars.length<Math.min(status.cars[i],8)&&cars.every(c=>c.s>GAP)){\n"
"        cars.push({s:0,colour:palette[(p.serial++)%palette.length]});\n"
"        p.routeCursor=(i+1)%12;break;\n"
"      }\n"
"    }\n"
"    p.arrivalTimer=2.5+Math.random()*2.5;\n"
"  }\n"
"  paths.forEach((path,i)=>{\n"
"    const cars=p.cars[i];cars.sort((a,b)=>b.s-a.s);\n"
"    const mayEnter=status.colour===0&&status.active[i]===1;\n"
"    let leader=Infinity;\n"
"    for(const car of cars){\n"
"      const committed=car.s>STOP+.1;\n"
"      let target=car.s+SPEED*dt;\n"
"      if(!mayEnter&&!committed)target=Math.min(target,STOP);\n"
"      target=Math.min(target,leader-GAP);\n"
"      car.s=Math.max(car.s,target);leader=car.s;\n"
"    }\n"
"    // Departed cars leave; they are not immediately recycled into the queue.\n"
"    p.cars[i]=cars.filter(c=>c.s<=path.length+12);\n"
"  });\n"
"}\n"
"const crossings=[{a:[-90,-76],b:[-90,76],lamp:[-111,-76]},{a:[90,-76],b:[90,76],lamp:[111,-76]},{a:[-76,-90],b:[76,-90],lamp:[-76,-111]},{a:[-76,90],b:[76,90],lamp:[-76,111]}];\n"
"function updatePeople(p,status,dt){\n"
"  if(!status.live)return;\n"
"  for(let i=0;i<4;i++){\n"
"    // Remember one observed press until its crossing receives WALK.\n"
"    if(status.buttons[i]&&!p.previousButtons[i])p.pedRequests[i]=1;\n"
"    p.previousButtons[i]=status.buttons[i];\n"
"    const walk=status.colour===0&&status.active[12+i];\n"
"    if(walk&&p.pedRequests[i]){\n"
"      p.people.push({crossing:i,t:0,reverse:(p.serial++)%2===0,shirt:palette[p.serial%palette.length]});\n"
"      p.pedRequests[i]=0;\n"
"      // Only the explicit preview owns synthetic requests; live reports are untouched.\n"
"      if(demo)demoPedestrians[p.index].buttons[i]=0;\n"
"    }\n"
"  }\n"
"  for(const person of p.people)person.t+=dt/5.2;\n"
"  p.people=p.people.filter(person=>person.t<=1);\n"
"}\n"
"function drawPeople(g,p,status,now){\n"
"  for(let i=0;i<4;i++){\n"
"    const c=crossings[i],phase=status.active[12+i]?status.colour:2;\n"
"    let color=!status.live?colours.unknown:phase===0?colours.green:colours.red;\n"
"    if(status.live&&phase===1&&Math.floor(now/350)%2===0)color='#473033';\n"
"    circle(g,...c.lamp,7,'#132029');circle(g,...c.lamp,4,color);\n"
"    if(status.live&&status.buttons[i]){circle(g,c.lamp[0],c.lamp[1]+15,4,colours.yellow);}\n"
"  }\n"
"  for(const person of p.people){\n"
"    const c=crossings[person.crossing],t=person.reverse?1-person.t:person.t;\n"
"    const x=c.a[0]+(c.b[0]-c.a[0])*t,y=c.a[1]+(c.b[1]-c.a[1])*t;\n"
"    const heading=Math.atan2(c.b[1]-c.a[1],c.b[0]-c.a[0])+(person.reverse?Math.PI:0);\n"
"    // Gait uses travelled progress, so limbs also freeze on stale reports.\n"
"    const swing=Math.sin(person.t*5.2*9)*3.5;\n"
"    g.save();g.translate(x,y);g.rotate(heading);\n"
"    circle(g,1,2,6,'#13251e88');\n"
"    line(g,[[-3+swing,-2],[-1,-1],[-3-swing,2]],'#182332',2.3);\n"
"    line(g,[[swing,-5],[0,-2],[0,2],[-swing,5]],status.live?'#e7bb92':colours.unknown,2);\n"
"    line(g,[[-1,-2],[-1,2]],status.live?person.shirt:colours.unknown,5);\n"
"    circle(g,3,0,3.1,status.live?'#efc49b':colours.unknown);\n"
"    circle(g,4,-.6,1.4,'#654c38');g.restore();\n"
"  }\n"
"}\n"
"\n"
"function drawPanel(p,traffic,ped,now,dt){\n"
"  updateCars(p,traffic,dt);updatePeople(p,ped,dt);\n"
"  const g=mapContext;g.save();g.translate(centres[p.index].x,centres[p.index].y);\n"
"  drawGround(g,p);\n"
"  for(let i=0;i<12;i++)for(const car of p.cars[i])drawCar(g,car,paths[i]);\n"
"  for(let i=0;i<12;i++)drawSignal(g,paths[i],traffic.active[i]?traffic.colour:2,traffic.live);\n"
"  drawPeople(g,p,ped,now);\n"
"  if(!traffic.live){rect(g,-132,-22,264,43,'#10202bee');text(g,traffic.seen?'STATUS STALE · MOTION PAUSED':'WAITING FOR TRAFFIC NODE',0,4,11,'#d6e1e8');}\n"
"  g.restore();\n"
"}\n"
"function drawTrain(status,now,dt){\n"
" const g=mapContext;g.save();g.translate(0,580);\n"
" for(const y of [22,52]){\n"
"   rect(g,0,y-12,WORLD_W,24,'#354239');\n"
"   for(let x=0;x<WORLD_W;x+=12)rect(g,x,y-12,4,24,'#615d4b');\n"
"   line(g,[[0,y-6],[WORLD_W,y-6]],'#b3bcb2',2);\n"
"   line(g,[[0,y+6],[WORLD_W,y+6]],'#b3bcb2',2);\n"
" }\n"
" if(status.live){\n"
"   if(status.state!==trainMotion.state&&status.state===1)trainMotion.offset=340;\n"
"   if(status.state===1){trainMotion.offset+=65*dt;if(trainMotion.offset>WORLD_W+240)trainMotion.offset=-30;}\n"
"   if(status.state===2&&trainMotion.state!==1&&trainMotion.state!==2)trainMotion.offset=340;\n"
"   trainMotion.state=status.state;\n"
" }\n"
" if(trainMotion.state===1||trainMotion.state===2){\n"
"   for(let i=4;i>=0;i--){const x=trainMotion.offset-i*47;\n"
"     rect(g,x-22,10,44,23,status.live?'#c98261':'#667b76');\n"
"     rect(g,x-20,11,40,3,status.live?'#efb387':'#8a9993');\n"
"     for(let j=0;j<4;j++)rect(g,x-17+j*9,17,6,9,'#234453');\n"
"     circle(g,x-14,33,2,'#132524');circle(g,x+14,33,2,'#132524');\n"
"     if(i===0)rect(g,x+17,16,4,13,'#eed69d');\n"
"   }\n"
" }\n"
" const color=!status.live?colours.unknown:status.state===2?colours.red:status.state===1?colours.yellow:colours.green;\n"
" rect(g,28,-20,285,23,'#10251f');\n"
" text(g,!status.live?'X1 · TRAIN STATUS UNKNOWN':status.state===2?'X1 · TRAIN ERROR':status.state===1?'X1 · TRAIN PRESENT':'X1 · RAILWAY CLEAR',40,-4,12,color,'left');\n"
" g.restore();\n"
"}\n"
"function drawNetwork(nodes,now,dt){\n"
" const g=mapContext;g.setTransform(network.width/WORLD_W,0,0,network.height/WORLD_H,0,0);\n"
" rect(g,0,0,WORLD_W,WORLD_H,'#182e2d');\n"
" // R3 is continuous through both junctions and the railway crossing.\n"
" rect(g,374,0,152,WORLD_H,'#98a29b');rect(g,382,0,136,WORLD_H,'#34404a');\n"
" line(g,[[450,0],[450,WORLD_H]],'#d9bd74',2);\n"
" for(const x of [406,426,474,494])line(g,[[x,0],[x,WORLD_H]],'#82909b',1,[9,11]);\n"
" for(const c of centres){\n"
"   rect(g,0,c.y-76,WORLD_W,152,'#98a29b');rect(g,0,c.y-68,WORLD_W,136,'#34404a');\n"
"   line(g,[[0,c.y],[WORLD_W,c.y]],'#d9bd74',2);\n"
"   for(const offset of [-44,-24,24,44])line(g,[[0,c.y+offset],[WORLD_W,c.y+offset]],'#82909b',1,[9,11]);\n"
" }\n"
" panels.forEach(p=>drawPanel(p,nodes[p.index],nodes[p.index+2],now,dt));\n"
" drawTrain(nodes[4],now,dt);\n"
" for(let i=0;i<2;i++){\n"
"   rect(g,28,centres[i].y-110,100,26,'#0d211c');text(g,`L${i+1} · R${i+1}`,78,centres[i].y-92,13,'#cce4d6');\n"
" }\n"
" line(g,[[840,88],[840,35]],'#d4e8de',2);line(g,[[833,45],[840,35],[847,45]],'#d4e8de',2);text(g,'N',840,24,14,'#d4e8de');\n"
"}\n"
"let lastUI=0;\n"
"function updateUI(nodes,now){\n"
" const liveCount=nodes.filter(n=>n.live).length;\n"
" const label=demo?'DEMO · SYNTHETIC DATA':networkOK?`${liveCount}/5 NODES LIVE`:'WAITING FOR LIVE DATA';\n"
" byId('mode').textContent=label;byId('mode').className='pill '+(demo?'demo':liveCount===5?'live':'');\n"
" byId('banner').textContent=demo?'PREVIEW ONLY — synthetic light states and counts. This animation does not send commands to your nodes.':networkOK?'Live from QNX · status sampled every 250 ms · smooth animation between reports · one connected north-up map.':'Live connection unavailable. Open http://DISPLAY_NODE_IP:8080, or use Preview animation to inspect the drawing.';\n"
" if(demo||networkOK)byId('error').textContent='';\n"
" const tr=nodes[4];byId('trainPhase').textContent=!tr.live?(tr.seen?'STALE':'NO REPORT'):['CLEAR','TRAIN PRESENT','TRAIN ERROR'][tr.state];\n"
" byId('trainPhase').style.color=!tr.live?colours.unknown:tr.state===2?colours.red:tr.state===1?colours.yellow:colours.green;\n"
" byId('trainDetail').textContent=(tr.seen?`Report age ${(tr.age/1000).toFixed(1)}s · `:'Waiting for Train_To_Display · ')+'Random train gap: 60–120s; presence: 8–12s. Position/direction illustrative; gate position not reported.';\n"
" for(const p of panels){const t=nodes[p.index],pd=nodes[p.index+2],q=s=>p.el.querySelector(s),phase=['GREEN','YELLOW','RED'];\n"
"  q('.phase').textContent=t.live?phase[t.colour]:(t.seen?'STALE':'NO REPORT');q('.phase').style.color=t.live?[colours.green,colours.yellow,colours.red][t.colour]:colours.unknown;\n"
"  q('.route').textContent=t.seen?t.state:'—';q('.cars').textContent=t.seen?t.cars.reduce((a,b)=>a+b,0):'—';\n"
"  q('.trafficAge').textContent=t.seen?(t.live?'live':'stale')+` ${(t.age/1000).toFixed(1)}s`:'waiting';q('.pedAge').textContent=pd.seen?(pd.live?'live':'stale')+` ${(pd.age/1000).toFixed(1)}s`:'waiting';\n"
"  q('.train').textContent=t.live?(t.train?'detected':'clear'):'unknown';\n"
"  q('.input').textContent=t.input_ok&&t.input_age_ms+(demo?0:now-receivedAt)<11000?'ACK':'waiting';\n"
"  p.el.querySelectorAll('.request').forEach((el,i)=>{el.textContent=crossingNames[i]+' · '+(!pd.live?'?':pd.buttons[i]?'request':'clear');el.classList.toggle('pending',pd.live&&pd.buttons[i]===1);});\n"
" }\n"
"}\n"
"let previous=performance.now();\n"
"function frame(now){const dt=Math.min((now-previous)/1000,.06);previous=now;const nodes=getNodes(now);drawNetwork(nodes,now,dt);if(now-lastUI>200){updateUI(nodes,now);lastUI=now;}requestAnimationFrame(frame);}\n"
"requestAnimationFrame(frame);\n"
"</script>\n"
"</body>\n"
"</html>\n";

static int AppendJSON(char *buffer, size_t size, size_t *used, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int written = vsnprintf(buffer + *used, size - *used, format, args);
    va_end(args);
    if (written < 0 || (size_t)written >= size - *used) return -1;
    *used += written;
    return 0;
}
static int BuildDisplayJSON(Display_Data *display, char *buffer, size_t size) {
    Display_Slot slots[DISPLAY_NODE_COUNT];
    pthread_mutex_lock(&display->mutex);
    memcpy(slots, display->slots, sizeof(slots));
    pthread_mutex_unlock(&display->mutex);
    uint64_t now = DisplayNowMs();
    size_t used = 0;
#define JSON_ADD(...) do { if (AppendJSON(buffer, size, &used, __VA_ARGS__) == -1) return -1; } while (0)
    JSON_ADD("{\"nodes\":[");
    for (int node = 0; node < DISPLAY_NODE_COUNT; node++) {
        Display_Slot *s = &slots[node];
        uint64_t age = s->seen && now >= s->received_ms ? now - s->received_ms : 0;
        uint64_t input_age = s->input_ok && now >= s->input_ack_ms ? now - s->input_ack_ms : 0;
        /* Unseen nodes use red/all-stop defaults, but 'seen:false' makes them unknown in the UI. */
        int colour = s->seen ? s->status.colour : 2;
        int state = s->seen ? s->status.state : node < DISPLAY_PED_L1 ? 28 : 0;
        JSON_ADD("%s{\"id\":%d,\"seen\":%s,\"age_ms\":%llu,\"colour\":%d,\"state\":%d,\"train\":%d,\"input_ok\":%s,\"input_age_ms\":%llu,\"active\":[",
                 node ? "," : "", node, s->seen ? "true" : "false",
                 (unsigned long long)age, colour, state, s->status.train_detected,
                 s->input_ok ? "true" : "false", (unsigned long long)input_age);
        for (int i = 0; i < 16; i++) JSON_ADD("%s%d", i ? "," : "", s->status.active[i]);
        JSON_ADD("],\"cars\":[");
        for (int i = 0; i < 12; i++) JSON_ADD("%s%d", i ? "," : "", s->status.cars[i]);
        JSON_ADD("],\"buttons\":[");
        for (int i = 0; i < 4; i++) JSON_ADD("%s%d", i ? "," : "", s->status.buttons[i]);
        JSON_ADD("]}");
    }
    JSON_ADD("]}");
#undef JSON_ADD
    return (int)used;
}
static int SendHTTPBytes(int socket_fd, const char *data, size_t length) {
    while (length > 0) {
        ssize_t sent = send(socket_fd, data, length, 0);
        if (sent == -1 && errno == EINTR) continue;
        if (sent <= 0) return -1;
        data += sent;
        length -= (size_t)sent;
    }
    return 0;
}
static void HTTPReply(int fd, const char *status, const char *type, const char *body, size_t size) {
    char header[512];
    int length = snprintf(header, sizeof(header),
        "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\n"
        "Cache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\nConnection: close\r\n\r\n",
        status, type, size);
    if (length > 0 && (size_t)length < sizeof(header) &&
        SendHTTPBytes(fd, header, (size_t)length) == 0) SendHTTPBytes(fd, body, size);
}
static void ServeBrowser(Display_Data *display, int listen_fd) {
    for (;;) {
        int fd = accept(listen_fd, NULL, NULL);
        if (fd == -1) { if (errno == EINTR) continue; perror("accept"); DisplaySleepMs(100); continue; }
        struct timeval timeout = {1, 0};
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        char request[2048] = {0};
        size_t used = 0;
        /* Bound each request as well as each read; this is a small lab HTTP server. */
        uint64_t deadline = DisplayNowMs() + 1500;
        while (used < sizeof(request) - 1 && !strstr(request, "\r\n\r\n") && DisplayNowMs() < deadline) {
            ssize_t n = recv(fd, request + used, sizeof(request) - used - 1, 0);
            if (n == -1 && errno == EINTR) continue;
            if (n <= 0) break;
            used += (size_t)n;
            request[used] = '\0';
        }
        char method[8], path[256];
        if (!strstr(request, "\r\n\r\n") || sscanf(request, "%7s %255s", method, path) != 2) {
            HTTPReply(fd, "400 Bad Request", "text/plain", "Bad request\n", 12);
        } else if (strcmp(method, "GET") != 0) {
            HTTPReply(fd, "405 Method Not Allowed", "text/plain", "GET only\n", 9);
        } else {
            char *query = strchr(path, '?');
            if (query) *query = '\0';
            if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
                HTTPReply(fd, "200 OK", "text/html; charset=utf-8", Display_HTML, sizeof(Display_HTML) - 1);
            } else if (strcmp(path, "/state") == 0) {
                char json[8192];
                int length = BuildDisplayJSON(display, json, sizeof(json));
                if (length >= 0) HTTPReply(fd, "200 OK", "application/json", json, (size_t)length);
                else HTTPReply(fd, "500 Internal Server Error", "text/plain", "Status too large\n", 17);
            } else HTTPReply(fd, "404 Not Found", "text/plain", "Not found\n", 10);
        }
        close(fd);
    }
}
static int OpenBrowserServer(void) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) return -1;
    int enabled = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(DISPLAY_HTTP_PORT);
    if (bind(fd, (struct sockaddr *)&address, sizeof(address)) == -1 || listen(fd, 8) == -1) {
        int error = errno; close(fd); errno = error; return -1;
    }
    return fd;
}

//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Train Status Server — Train Node Is the Client; Same Existing Train Message
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------
#define TRAIN_DISPLAY_ATTACH_POINT "Train_To_Display"
#define TRAIN_DISPLAY_DATA_TYPE    0x23
#define TRAIN_DISPLAY_STATUS_UPDATE 1
/* Same values as your train node's TrainState. */
enum { DISPLAY_TRAIN_NOT_PRESENT, DISPLAY_TRAIN_PRESENT, DISPLAY_TRAIN_ERROR };
typedef struct {
    Display_MessageHeader hdr;
    int32_t ClientID;
    int32_t trainState;
} Train_Display_Message;
typedef char Train_Display_Size_Check[(sizeof(Train_Display_Message) == 40) ? 1 : -1];
static int TrainDisplayServer(Display_Data *display) {
    name_attach_t *attach = name_attach(NULL, TRAIN_DISPLAY_ATTACH_POINT, 0);
    if (!attach) return -1;
    for (;;) {
        union { struct _pulse pulse; Train_Display_Message message; } received = {0};
        struct _msg_info info;
        int rcvid = MsgReceive(attach->chid, &received, sizeof(received), &info);
        if (rcvid == -1) {
            if (errno == EINTR) continue;
            int error = errno; name_detach(attach, 0); errno = error; return -1;
        }
        if (rcvid == 0) {
            if (received.pulse.code == _PULSE_CODE_DISCONNECT) ConnectDetach(received.pulse.scoid);
            else if (received.pulse.code == _PULSE_CODE_UNBLOCK) {
                struct _msg_info pending;
                int id = received.pulse.value.sival_int;
                if (MsgInfo(id, &pending) != -1 && (pending.flags & _NTO_MI_UNBLOCK_REQ)) MsgError(id, ETIMEDOUT);
            }
            continue;
        }
        if (info.msglen >= 2 && received.message.hdr.type == _IO_CONNECT) { MsgReply(rcvid, EOK, NULL, 0); continue; }
        if (info.msglen >= 2 && received.message.hdr.type <= _IO_MAX && received.message.hdr.type != TRAIN_DISPLAY_DATA_TYPE) {
            MsgError(rcvid, ENOSYS); continue;
        }
        if (info.srcmsglen != (int)sizeof(Train_Display_Message) ||
            (info.msglen < (int)sizeof(Train_Display_Message) &&
             MsgRead(rcvid, &received.message, sizeof(Train_Display_Message), 0) != (int)sizeof(Train_Display_Message))) {
            MsgError(rcvid, EMSGSIZE); continue;
        }
        Train_Display_Message *msg = &received.message;
        if (msg->hdr.type != TRAIN_DISPLAY_DATA_TYPE || msg->hdr.subtype != TRAIN_DISPLAY_STATUS_UPDATE ||
            msg->trainState < DISPLAY_TRAIN_NOT_PRESENT || msg->trainState > DISPLAY_TRAIN_ERROR) {
            MsgError(rcvid, EINVAL); continue;
        }
        pthread_mutex_lock(&display->mutex);
        Display_Slot *slot = &display->slots[DISPLAY_TRAIN];
        slot->status.state = msg->trainState;
        slot->status.colour = 2;
        slot->status.train_detected = msg->trainState == DISPLAY_TRAIN_PRESENT;
        slot->seen = 1;
        slot->received_ms = DisplayNowMs();
        slot->server_error = 0;
        pthread_mutex_unlock(&display->mutex);
        Display_Reply reply = {0};
        reply.hdr.type = TRAIN_DISPLAY_DATA_TYPE;
        reply.hdr.subtype = TRAIN_DISPLAY_STATUS_UPDATE;
        snprintf(reply.buf, sizeof(reply.buf), "Train display status received");
        MsgReply(rcvid, EOK, &reply, sizeof(reply));
    }
}
static void *TrainDisplayServerThread(void *argument) {
    Display_Data *display = argument;
    for (;;) {
        TrainDisplayServer(display);
        int error = errno;
        pthread_mutex_lock(&display->mutex);
        display->slots[DISPLAY_TRAIN].server_error = error;
        pthread_mutex_unlock(&display->mutex);
        DisplaySleepMs(DISPLAY_RETRY_MS);
    }
    return NULL;
}

//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Random Train Client — Long Gaps; Send Detection Changes to the Train Node
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------
#define DISPLAY_TRAIN_INPUT_NAME "Simulator_To_Train"
#define DISPLAY_TRAIN_GAP_MIN_MS 60000
#define DISPLAY_TRAIN_GAP_MAX_MS 120000
#define DISPLAY_TRAIN_DURATION_MIN_MS 8000
#define DISPLAY_TRAIN_DURATION_MAX_MS 12000
static unsigned TrainRandomDelay(uint32_t *seed, unsigned minimum, unsigned maximum) {
    return minimum + NextRandom(seed) % (maximum - minimum + 1);
}
static void *DisplayTrainClientThread(void *argument) {
    Display_ThreadArguments *args = argument;
    int coid = -1, present = 0, last_sent = -1;
    uint64_t next_change = DisplayNowMs() + TrainRandomDelay(&args->random_seed,
        DISPLAY_TRAIN_GAP_MIN_MS, DISPLAY_TRAIN_GAP_MAX_MS);
    for (;;) {
        uint64_t now = DisplayNowMs();
        if (now >= next_change) {
            present = !present;
            unsigned duration = present
                ? TrainRandomDelay(&args->random_seed, DISPLAY_TRAIN_DURATION_MIN_MS, DISPLAY_TRAIN_DURATION_MAX_MS)
                : TrainRandomDelay(&args->random_seed, DISPLAY_TRAIN_GAP_MIN_MS, DISPLAY_TRAIN_GAP_MAX_MS);
            next_change = now + duration;
        }
        if (coid == -1) {
            coid = name_open(args->input_path, 0);
            if (coid == -1) { SetInputResult(args, 0, errno); DisplaySleepMs(DISPLAY_RETRY_MS); continue; }
            last_sent = -1; // After reconnecting, send the current detection rather than old transitions.
        }
        if (last_sent != present) {
            Display_Message msg = {0};
            msg.hdr.type = DISPLAY_DATA_TYPE;
            msg.hdr.subtype = DISPLAY_RANDOM_INPUT;
            msg.node = DISPLAY_TRAIN;
            msg.train_detected = present;
            msg.state = present; // 0=not present, 1=present; no simulated hardware errors.
            if (DisplaySend(coid, &msg) == -1) {
                SetInputResult(args, 0, errno);
                name_close(coid); coid = -1;
                DisplaySleepMs(DISPLAY_RETRY_MS);
                continue;
            }
            SetInputResult(args, 1, 0);
            last_sent = present;
        }
        DisplaySleepMs(100); // Check the deadline without busy-spinning; do not repeatedly send.
    }
    return NULL;
}

//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Main — Five QNX Status Servers, Five Random-Input Clients; Main Serves the Browser
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------
int main(int argc, char **argv) {
    if (argc != 1 && argc != 5 && argc != 6) {
        fprintf(stderr, "Usage: %s [L1-input-path L2-input-path PedL1-input-path PedL2-input-path [Train-input-path]]\n", argv[0]);
        return EXIT_FAILURE;
    }
    signal(SIGPIPE, SIG_IGN);
    int http_fd = OpenBrowserServer();
    if (http_fd == -1) { perror("HTTP server on port 8080"); return EXIT_FAILURE; }
    Display_Data display = {.mutex = PTHREAD_MUTEX_INITIALIZER};
    Display_ThreadArguments arguments[5];
    pthread_t servers[5], clients[5];
    for (int node = 0; node < 4; node++) {
        arguments[node] = (Display_ThreadArguments){
            .node = node,
            .input_path = argc >= 5 ? argv[node + 1] : Display_Input_Names[node],
            .display = &display,
            .random_seed = ((uint32_t)time(NULL) ^ (uint32_t)getpid() ^ (0x9e3779b9U * (node + 1))) | 1U
        };
        int error = pthread_create(&servers[node], NULL, DisplayServerThread, &arguments[node]);
        if (!error) error = pthread_create(&clients[node], NULL, DisplayClientThread, &arguments[node]);
        if (error) { fprintf(stderr, "pthread_create: %s\n", strerror(error)); close(http_fd); return EXIT_FAILURE; }
    }
    int error = pthread_create(&servers[4], NULL, TrainDisplayServerThread, &display);
    if (error) { fprintf(stderr, "train display thread: %s\n", strerror(error)); close(http_fd); return EXIT_FAILURE; }
    arguments[4] = (Display_ThreadArguments){
        .node = DISPLAY_TRAIN,
        .input_path = argc == 6 ? argv[5] : DISPLAY_TRAIN_INPUT_NAME,
        .display = &display,
        .random_seed = ((uint32_t)time(NULL) ^ (uint32_t)getpid() ^ 0xa53c917bU) | 1U
    };
    error = pthread_create(&clients[4], NULL, DisplayTrainClientThread, &arguments[4]);
    if (error) { fprintf(stderr, "train simulator thread: %s\n", strerror(error)); close(http_fd); return EXIT_FAILURE; }
    printf("2D display ready. Open http://<DISPLAY_NODE_IP>:%d in your computer's browser.\n", DISPLAY_HTTP_PORT);
    printf("Train status server: %s\n", TRAIN_DISPLAY_ATTACH_POINT);
    fflush(stdout);
    ServeBrowser(&display, http_fd);
    close(http_fd);
    return EXIT_SUCCESS;
}

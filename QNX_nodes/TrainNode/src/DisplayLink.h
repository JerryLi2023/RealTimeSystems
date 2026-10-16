#ifndef DISPLAY_LINK_H
#define DISPLAY_LINK_H

#include <pthread.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/dispatch.h>
#include <sys/neutrino.h>

/* Hostname of the display node VM, used to build /net/<host>/dev/name/local/<name> paths. */
#define DISPLAY_NODE_HOST        "VM_x86_DisplayNode"
#define DISPLAY_REMOTE_PATH(n)   "/net/" DISPLAY_NODE_HOST "/dev/name/local/" n

#define DISPLAY_DATA_TYPE        0x700
#define DISPLAY_STATUS           1
#define DISPLAY_REPORT_MS        250
#define DISPLAY_RETRY_MS         2000
#define DISPLAY_SEND_TIMEOUT_NS  1000000000ULL

enum { DISPLAY_L1, DISPLAY_L2, DISPLAY_PED_L1, DISPLAY_PED_L2, DISPLAY_TRAIN, DISPLAY_NODE_COUNT };

typedef union { uint32_t sival_int; uint32_t dummy[4]; } Display_Sigval;
typedef struct {
    uint16_t type, subtype;
    int8_t code;
    uint8_t zero[3];
    Display_Sigval value;
    uint8_t zero2[2];
    int32_t scoid;
} Display_MessageHeader;

/* What a node reports. */
typedef struct {
    int32_t colour, state, train_detected;
    int32_t active[16];
    int32_t cars[12];
    int32_t buttons[4];
} Display_Status;
/* What the display hands back: simulated inputs for that node. */
typedef struct {
    int32_t cars[12];     /* level: current waiting-car count per movement (traffic nodes)   */
    int32_t buttons[4];   /* one-shot: a 1 means "pressed since last report" (pedestrian)    */
    int32_t train;        /* level: 1 = train detected (train node)                          */
} Display_Input;

typedef struct { Display_MessageHeader hdr; int32_t node; Display_Status status; } Display_Message;
typedef struct { Display_MessageHeader hdr; Display_Input input; } Display_Reply;

typedef char Display_Header_Check[(sizeof(Display_MessageHeader) == 32) ? 1 : -1];
typedef char Display_Message_Check[(sizeof(Display_Message) == 176) ? 1 : -1];
typedef char Display_Reply_Check[(sizeof(Display_Reply) == 100) ? 1 : -1];

typedef void (*Display_SnapshotFn)(void *node_data, Display_Status *out); /* called with mutex held */
typedef void (*Display_ApplyFn)(void *node_data, const Display_Input *in); /* called with mutex held */

typedef struct {
    int node;                    /* DISPLAY_L1 ... DISPLAY_TRAIN                         */
    const char *path;            /* DISPLAY_REMOTE_PATH("Display_From_L1") or argv value  */
    void *data;                  /* passed straight to snapshot / apply                   */
    pthread_mutex_t *mutex;      /* the mutex that protects 'data'                        */
    Display_SnapshotFn snapshot;
    Display_ApplyFn apply;       /* may be NULL if the node takes no inputs               */
} Display_Link;

static void Display_SleepMs(unsigned ms) {
    struct timespec d = { ms / 1000, (ms % 1000) * 1000000L };
    while (nanosleep(&d, &d) == -1 && errno == EINTR) {}
}

static int Display_Exchange(int coid, const Display_Message *msg, Display_Reply *reply) {
    uint64_t timeout_ns = DISPLAY_SEND_TIMEOUT_NS;
    if (TimerTimeout(CLOCK_MONOTONIC, _NTO_TIMEOUT_SEND | _NTO_TIMEOUT_REPLY, NULL, &timeout_ns, NULL) == -1)
        return -1;
    int result = MsgSend(coid, msg, sizeof(*msg), reply, sizeof(*reply));
    int saved = errno;
    TimerTimeout(CLOCK_MONOTONIC, 0, NULL, NULL, NULL);      /* disarm even after an immediate reply */
    errno = saved;
    if (result == -1) return -1;
    if (reply->hdr.type != DISPLAY_DATA_TYPE || reply->hdr.subtype != DISPLAY_STATUS) { errno = EPROTO; return -1; }
    return 0;
}

static int Display_InputValid(const Display_Input *in) {
    for (int i = 0; i < 12; i++) if (in->cars[i] < 0 || in->cars[i] > 1000) return 0;
    for (int i = 0; i < 4; i++)  if (in->buttons[i] != 0 && in->buttons[i] != 1) return 0;
    return in->train == 0 || in->train == 1;
}

static void *Display_LinkThread(void *argument) {
    Display_Link *link = argument;
    int coid = -1;
    for (;;) {
        if (coid == -1) {
            coid = name_open(link->path, 0);
            if (coid == -1) { Display_SleepMs(DISPLAY_RETRY_MS); continue; }
        }
        Display_Message msg;
        memset(&msg, 0, sizeof(msg));
        msg.hdr.type = DISPLAY_DATA_TYPE;
        msg.hdr.subtype = DISPLAY_STATUS;
        msg.node = link->node;
        pthread_mutex_lock(link->mutex);
        link->snapshot(link->data, &msg.status);
        pthread_mutex_unlock(link->mutex);

        Display_Reply reply;
        memset(&reply, 0, sizeof(reply));
        if (Display_Exchange(coid, &msg, &reply) == -1) {          /* display down or link lost: retry later */
            name_close(coid);
            coid = -1;
            Display_SleepMs(DISPLAY_RETRY_MS);
            continue;
        }
        if (link->apply && Display_InputValid(&reply.input)) {
            pthread_mutex_lock(link->mutex);
            link->apply(link->data, &reply.input);
            pthread_mutex_unlock(link->mutex);
        }
        Display_SleepMs(DISPLAY_REPORT_MS);
    }
    return NULL;
}

/* Call once from main(). Returns 0 on success or a pthread_create error code. */
static int Display_StartLink(Display_Link *link) {
    pthread_t thread;
    int error = pthread_create(&thread, NULL, Display_LinkThread, link);
    if (!error) pthread_detach(thread);
    return error;
}

#endif /* DISPLAY_LINK_H */
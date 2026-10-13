#include "SampleCode_DisplayTransport.h"
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
    if (node == DISPLAY_TRAIN && msg->train_detected != 0 && msg->train_detected != 1) return 0;
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

//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Original Node Connections — One Input Server and One Status Client per Node
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------
static void ApplyDisplayInput(void *argument, const Display_Message *msg) {
    Display_NodeLink *link = argument;
    pthread_mutex_lock(link->mutex);
    link->apply_input(link->light, msg);
    pthread_mutex_unlock(link->mutex);
}
static void *DisplayInputThread(void *argument) {
    Display_NodeLink *link = argument;
    for (;;) {
        DisplayServer(Display_Input_Names[link->node], link->node,
                      DISPLAY_RANDOM_INPUT, ApplyDisplayInput, link);
        perror("display input server");
        DisplaySleepMs(DISPLAY_RETRY_MS);
    }
    return NULL;
}
static void *DisplayStatusThread(void *argument) {
    Display_NodeLink *link = argument;
    for (;;) {
        int coid = name_open(link->status_path, 0);
        if (coid == -1) { DisplaySleepMs(DISPLAY_RETRY_MS); continue; }
        for (;;) {
            Display_Message msg = {0};
            msg.hdr.type = DISPLAY_DATA_TYPE;
            msg.hdr.subtype = DISPLAY_STATUS;
            msg.node = link->node;
            pthread_mutex_lock(link->mutex);
            link->snapshot(link->light, &msg);
            pthread_mutex_unlock(link->mutex);
            /* Network work always happens after unlocking the light data. */
            if (DisplaySend(coid, &msg) == -1) break;
            DisplaySleepMs(DISPLAY_REPORT_MS);
        }
        name_close(coid);
        DisplaySleepMs(DISPLAY_RETRY_MS);
    }
    return NULL;
}
int StartDisplayNodeLink(Display_NodeLink *link) {
    int error = pthread_create(&link->input_thread, NULL, DisplayInputThread, link);
    if (error != 0) return error;
    return pthread_create(&link->status_thread, NULL, DisplayStatusThread, link);
}

int StartDisplayInputLink(Display_NodeLink *link) {
    return pthread_create(&link->input_thread, NULL, DisplayInputThread, link);
}

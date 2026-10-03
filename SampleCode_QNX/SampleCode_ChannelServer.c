/*
 * SampleCode_ChannelServer.c
 * Source: Lab5_Task2_Server (2).c (user-supplied lab example).
 * Purpose: Create a local channel and advance the traffic machine on each request.
 * Adaptation: Extracted setup/server loop; kept message-driven state steps, added validation and q shutdown.
 * Copy includes, types and functions together. Keep client/server types identical.
 * Define SAMPLECODE_DEMO for the standalone example main().
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <sys/netmgr.h>
#include <sys/iomsg.h>

#define BUF_SIZE 100
#define SERVER_INFO "/tmp/myServer.info"

/* Teacher's native _pulse header is retained for this SAME-NODE pair.
 * Both processes must use compatible ABI/message definitions.
 * Use the NamedClient/NamedServer pair for the Qnet example. */
typedef struct {
    struct _pulse hdr;
    int ClientID;
    char data;
} my_data;
typedef struct {
    struct _pulse hdr;
    char buf[BUF_SIZE];
} my_reply;

enum states {State0, State1, State2, State3, State4, State5, State6, State7};

/* Performs ONE step; changes *CurrentState to the next state.
 * input is a snapshot ('e', 'n' or '\0'), not a pointer to shared data.
 * Keeps the lab's blocking sleeps: 2 seconds for green, 1 for other states.
 * Prints the current state's outputs before returning with the NEXT state. */
void SingleStep_TrafficLight_SM(enum states *CurrentState, char input)
{
    switch (*CurrentState) {
        case 0:
            *CurrentState = State1;
            printf ("East and West traffic light: Red | North and South traffic light: Red\n");
            sleep(1);
            break;
        case 1:
            *CurrentState = State2;
            printf ("East and West traffic light: Red | North and South traffic light: Red\n");
            sleep(1);
            break;
        case 2:
            printf ("East and West traffic light: Green | North and South traffic light: Red\n");
            if (input == 'n') {
                *CurrentState = State3;
            } else {
                *CurrentState = State2;
            }
            sleep(2);
            break;
        case 3:
            *CurrentState = State4;
            printf ("East and West traffic light: Yellow | North and South traffic light: Red\n");
            sleep(1);
            break;
        case 4:
            *CurrentState = State5;
            printf ("East and West traffic light: Red | North and South traffic light: Red\n");
            sleep(1);
            break;
        case 5:
            printf ("East and West traffic light: Red | North and South traffic light: Green\n");
            if (input == 'e') {
                *CurrentState = State6;
            } else {
                *CurrentState = State5;
            }
            sleep(2);
            break;
        case 6:
            *CurrentState = State7;
            printf ("East and West traffic light: Red | North and South traffic light: Yellow\n");
            sleep(1);
            break;
        case 7:
            *CurrentState = State0;
            printf ("East and West traffic light: Red | North and South traffic light: Red\n");
            sleep(1);
            break;
    }
}

/* Returns channel ID or -1/errno; caller later destroys it and removes info_path.
 * Use a unique info_path per server; start clients after this function completes. */
int CreateLocalServer(const char *info_path)
{
    int chid = ChannelCreate(_NTO_CHF_DISCONNECT);
    int written, closed, saved;
    FILE *serverFile;
    if (chid == -1) return -1;
    serverFile = fopen(info_path, "w");
    if (!serverFile) { saved = errno; ChannelDestroy(chid); errno = saved; return -1; }
    written = fprintf(serverFile, "%ld\n%d\n", (long)getpid(), chid);
    closed = fclose(serverFile);
    if (written < 0 || closed == EOF) {
        saved = errno; ChannelDestroy(chid); unlink(info_path); errno = saved; return -1;
    }
    return chid;
}

/* Every e/n request runs ONE state step, including its sleep, before replying.
 * This preserves the lab: no incoming messages means NO state advancement.
 * q is a new demo stop command. It replies before destroying the channel. */
int RunLocalTrafficServer(const char *info_path)
{
    int chid = CreateLocalServer(info_path), rcvid, result = 0, saved = 0;
    enum states CurrentState = State0;
    my_data msg;
    my_reply reply = {0};
    struct _msg_info info;
    if (chid == -1) return -1;
    reply.hdr.type = 0x01;
    for (;;) {
        msg = (my_data){0};
        rcvid = MsgReceive(chid, &msg, sizeof(msg), &info);
        if (rcvid == -1) {
            if (errno == EINTR) continue;
            result = -1; saved = errno; break;
        }
        if (rcvid == 0) {
            if (msg.hdr.code == _PULSE_CODE_DISCONNECT) ConnectDetach(msg.hdr.scoid);
            continue;
        }
        if (msg.hdr.type == _IO_CONNECT) { MsgReply(rcvid, EOK, NULL, 0); continue; }
        if (msg.hdr.type >= _IO_BASE && msg.hdr.type <= _IO_MAX) {
            MsgError(rcvid, ENOSYS); continue;
        }
        if ((size_t)info.msglen != sizeof(msg) || (size_t)info.srcmsglen != sizeof(msg) || msg.hdr.type != 0) {
            MsgError(rcvid, EINVAL); continue;
        }
        if (msg.data == 'q') {
            snprintf(reply.buf, sizeof(reply.buf), "Server stopping");
            MsgReply(rcvid, EOK, &reply, sizeof(reply));
            break;
        }
        if (msg.data != 'e' && msg.data != 'n') { MsgError(rcvid, EINVAL); continue; }
        SingleStep_TrafficLight_SM(&CurrentState, msg.data);
        snprintf(reply.buf, sizeof(reply.buf), "Sensor '%c' received", msg.data);
        if (MsgReply(rcvid, EOK, &reply, sizeof(reply)) == -1) perror("MsgReply");
    }
    ChannelDestroy(chid);
    unlink(info_path);
    if (result == -1) errno = saved;
    return result;
}
#ifdef SAMPLECODE_DEMO
int main(void)
{
    if (RunLocalTrafficServer(SERVER_INFO) == -1) { perror("RunLocalTrafficServer"); return 1; }
    return 0;
}
#endif

/*
 * SampleCode_NamedServer.c
 * Source: Lab5_Task1_Server(1).c (user-supplied lab example).
 * Purpose: Register a server name, receive requests and reply.
 * Adaptation: Kept name_attach/receive/reply loop; separated native pulses from custom application messages.
 * Copy includes, types and functions together. Keep client/server types identical.
 * Define SAMPLECODE_DEMO for the standalone example main().
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include <sys/dispatch.h>
#include <sys/neutrino.h>
#include <sys/iomsg.h>

#define BUF_SIZE 100
#define DATA_TYPE 0x22

/* Teacher's padded header, with its UNUSED pointer removed.
 * A pointer must never be dereferenced in another process/node.
 * Fixed-width fields + layout checks avoid selecting a 32/64-bit build manually.
 * These structs still require matching byte order; this is not serialization. */
typedef union {
    uint32_t sival_int;
    uint32_t dummy[4];
} _mysigval;
typedef struct {
    uint16_t type, subtype;
    int8_t code;
    uint8_t zero[3];
    _mysigval value;
    uint8_t zero2[2];
    int32_t scoid;
} msg_header_t;
typedef struct {
    msg_header_t hdr;
    int32_t ClientID;
    int32_t data;
} my_data;
typedef struct {
    msg_header_t hdr;
    char buf[BUF_SIZE];
} my_reply;

/* A negative array bound stops compilation on an incompatible layout. */
typedef char check_header[(sizeof(msg_header_t) == 32) ? 1 : -1];
typedef char check_data[(sizeof(my_data) == 40 && offsetof(my_data, data) == 36) ? 1 : -1];
typedef char check_reply[(sizeof(my_reply) == 132 && offsetof(my_reply, buf) == 32) ? 1 : -1];

/* Native pulses MUST be decoded with struct _pulse, not msg_header_t.
 * The kernel's pulse layout is independent of our application protocol. */
typedef union {
    struct _pulse pulse;
    my_data data;
} receive_buffer;

/* Returns 0 on requested disconnect shutdown, -1/errno on receive/setup error.
 * stop_on_disconnect=1 matches the original single-client demo.
 * Use 0 to keep serving other clients after a disconnect. */
int RunNamedServer(const char *attach_point, int stop_on_disconnect)
{
    name_attach_t *attach = name_attach(NULL, attach_point, 0);
    int rcvid, msgnum = 0, result = 0, saved = 0;
    receive_buffer msg;
    struct _msg_info info;
    my_reply reply = {0};
    if (!attach) return -1;
    reply.hdr.type = 0x01;
    printf("Listening on %s\n", attach_point);
    for (;;) {
        msg = (receive_buffer){0};
        rcvid = MsgReceive(attach->chid, &msg, sizeof(msg), &info);
        if (rcvid == -1) {
            if (errno == EINTR) continue;
            result = -1; saved = errno; break;
        }
        if (rcvid == 0) {
            if (msg.pulse.code == _PULSE_CODE_DISCONNECT) {
                ConnectDetach(msg.pulse.scoid);
                if (stop_on_disconnect) break;
            } else if (msg.pulse.code == _PULSE_CODE_UNBLOCK) {
                /* Release the receive ID carried by the pulse, not rcvid=0. */
                MsgError(msg.pulse.value.sival_int, EINTR);
            }
            continue; /* Pulses have no application ClientID; never MsgReply(0). */
        }
        if (msg.data.hdr.type == _IO_CONNECT) {
            MsgReply(rcvid, EOK, NULL, 0); /* name_open/GNS connect handshake. */
            continue;
        }
        if (msg.data.hdr.type >= _IO_BASE && msg.data.hdr.type <= _IO_MAX) {
            MsgError(rcvid, ENOSYS); continue;
        }
        if ((size_t)info.msglen != sizeof(my_data) || (size_t)info.srcmsglen != sizeof(my_data) ||
            msg.data.hdr.type != DATA_TYPE) {
            MsgError(rcvid, EINVAL); continue;
        }
        /* Replace ONLY this block with your controller's data processing. */
        ++msgnum;
        printf("Client %" PRId32 ": data=%" PRId32 "\n", msg.data.ClientID, msg.data.data);
        snprintf(reply.buf, sizeof(reply.buf), "Message %d received", msgnum);
        if (MsgReply(rcvid, EOK, &reply, sizeof(reply)) == -1)
            perror("MsgReply"); /* Client may have disappeared; serve the next one. */
    }
    name_detach(attach, 0);
    if (result == -1) errno = saved;
    return result;
}
#ifdef SAMPLECODE_DEMO
int main(void)
{
    if (RunNamedServer("myname", 1) == -1) { perror("RunNamedServer"); return 1; }
    return 0;
}
#endif

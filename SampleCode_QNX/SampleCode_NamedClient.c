/*
 * SampleCode_NamedClient.c
 * Source: Lab5_Task1_Client(1).c (user-supplied lab example).
 * Purpose: Connect by name locally or over Qnet and send one integer request.
 * Adaptation: Extracted one send/reply operation; kept padded header with fixed-width data and no pointer.
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

/* Open ONCE with name_open(), send repeatedly, then name_close().
 * MsgSend blocks until the server receives AND replies (no timeout in this lab).
 * Returns 0 or -1/errno. reply belongs to the caller. */
int SendNamedData(int server_coid, int32_t ClientID, int32_t value, my_reply *reply)
{
    my_data msg = {0};
    *reply = (my_reply){0};
    msg.hdr.type = DATA_TYPE;
    msg.ClientID = ClientID;
    msg.data = value;
    return MsgSend(server_coid, &msg, sizeof(msg), reply, sizeof(*reply));
}
#ifdef SAMPLECODE_DEMO
int main(int argc, char **argv)
{
    /* Local: myname
     * Remote: /net/<server-hostname>/dev/name/local/myname (Qnet required).
     * Same message definitions must be used by BOTH new sample files. */
    const char *sname = argc > 1 ? argv[1] : "myname";
    int server_coid = name_open(sname, 0);
    int index, status = 0;
    my_reply reply;
    if (server_coid == -1) { perror("name_open"); return 1; }
    for (index = 0; index < 5; ++index) {
        if (SendNamedData(server_coid, 800, 10 + index, &reply) == -1) {
            perror("MsgSend"); status = 1; break;
        }
        printf("Reply: %.*s\n", BUF_SIZE, reply.buf);
    }
    name_close(server_coid); /* Closes connection; does not send application data. */
    return status;
}
#endif

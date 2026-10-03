/*
 * SampleCode_ChannelClient.c
 * Source: Lab5_Task2_Client (1).c (user-supplied lab example).
 * Purpose: Read PID/CHID from a file, attach to a local server, and send a sensor value.
 * Adaptation: Extracted connect/send functions; checked file/input errors and added q shutdown.
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

/* Server must already be running and have written a current PID/CHID file.
 * This uses ND_LOCAL_NODE: it can connect separate PROCESSES on the same node,
 * not just threads. A remote pathname alone would not make this cross-node. */
int ConnectLocalServer(const char *info_path)
{
    int serverPID, serverCHID, count;
    FILE *serverFile = fopen(info_path, "r");
    if (!serverFile) return -1;
    count = fscanf(serverFile, "%d%d", &serverPID, &serverCHID);
    fclose(serverFile);
    if (count != 2 || serverPID <= 0 || serverCHID < 0) { errno = EINVAL; return -1; }
    return ConnectAttach(ND_LOCAL_NODE, serverPID, serverCHID, _NTO_SIDE_CHANNEL, 0);
}
int SendTrafficData(int server_coid, int ClientID, char input, my_reply *reply)
{
    my_data msg = {0};
    *reply = (my_reply){0};
    msg.ClientID = ClientID;
    msg.data = input;
    /* Original type/subtype are both zero. */
    return MsgSend(server_coid, &msg, sizeof(msg), reply, sizeof(*reply));
}
#ifdef SAMPLECODE_DEMO
int main(void)
{
    int server_coid = ConnectLocalServer(SERVER_INFO), status = 0;
    char input;
    my_reply reply;
    if (server_coid == -1) { perror("ConnectLocalServer"); return 1; }
    puts("e = East-West, n = North-South, q = stop both demos");
    for (;;) {
        if (scanf(" %c", &input) != 1) input = 'q';
        if (input != 'e' && input != 'n' && input != 'q') continue;
        if (SendTrafficData(server_coid, 500, input, &reply) == -1) {
            perror("MsgSend"); status = 1; break;
        }
        printf("Reply: %.*s\n", BUF_SIZE, reply.buf);
        if (input == 'q') break;
    }
    ConnectDetach(server_coid);
    return status;
}
#endif

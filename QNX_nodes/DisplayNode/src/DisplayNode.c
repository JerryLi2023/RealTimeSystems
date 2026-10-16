#include "DisplayLink.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/iomsg.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define DISPLAY_UDP_PORT 9000
#define DISPLAY_SERVE_MS 2000    // Maximum time before system considers the car served

static const char *const Display_Status_Names[DISPLAY_NODE_COUNT] = {
    "Display_From_L1", "Display_From_L2", "Display_From_Pedestrian_L1",
    "Display_From_Pedestrian_L2", "Display_From_Train"
};

typedef struct {
    Display_Status status;
    uint64_t received_ms;
    int seen, server_error;
} Display_Slot;
typedef struct {
    pthread_mutex_t mutex;
    Display_Slot slots[DISPLAY_NODE_COUNT];
    Display_Input command[DISPLAY_NODE_COUNT];   /* what the next reply to each node will carry */
    uint64_t green_since[DISPLAY_NODE_COUNT][12]; /* when each movement started showing green (0 = not green) */
} Display_Data;
typedef struct { int node; Display_Data *display; } Display_ServerArguments;

static uint64_t NowMs(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000 + (uint64_t)t.tv_nsec / 1000000;
}

static int StatusValid(const Display_Message *m, int node) {
    if (m->hdr.type != DISPLAY_DATA_TYPE || m->hdr.subtype != DISPLAY_STATUS || m->node != node) return 0;
    const Display_Status *s = &m->status;
    if (s->colour < 0 || s->colour > 2 || s->state < 0 || s->state > (node < DISPLAY_PED_L1 ? 33 : 15)) return 0;
    if (s->train_detected != 0 && s->train_detected != 1) return 0;
    for (int i = 0; i < 16; i++) if (s->active[i] != 0 && s->active[i] != 1) return 0;
    for (int i = 0; i < 12; i++) if (s->cars[i] < 0 || s->cars[i] > 1000) return 0;
    for (int i = 0; i < 4; i++)  if (s->buttons[i] != 0 && s->buttons[i] != 1) return 0;
    return 1;
}

// One status server per node: name_attach / MsgReceive / MsgReply
static int StatusServer(Display_Data *display, int node) {
    name_attach_t *attach = name_attach(NULL, Display_Status_Names[node], 0);
    if (attach == NULL) return -1;
    for (;;) {
        union { struct _pulse pulse; Display_Message message; } received;
        memset(&received, 0, sizeof(received));
        struct _msg_info info;
        int rcvid = MsgReceive(attach->chid, &received, sizeof(received), &info);
        if (rcvid == -1) {
            if (errno == EINTR) continue;
            int saved = errno; name_detach(attach, 0); errno = saved; return -1;
        }
        if (rcvid == 0) {                                           /* pulses */
            if (received.pulse.code == _PULSE_CODE_DISCONNECT) ConnectDetach(received.pulse.scoid);
            else if (received.pulse.code == _PULSE_CODE_UNBLOCK) {
                struct _msg_info pending;
                int waiting = received.pulse.value.sival_int;
                if (MsgInfo(waiting, &pending) != -1 && (pending.flags & _NTO_MI_UNBLOCK_REQ))
                    MsgError(waiting, ETIMEDOUT);
            }
            continue;
        }
        if (info.msglen >= 2 && received.message.hdr.type == _IO_CONNECT) { MsgReply(rcvid, EOK, NULL, 0); continue; }
        if (info.msglen >= 2 && received.message.hdr.type <= _IO_MAX) { MsgError(rcvid, ENOSYS); continue; }
        if (info.srcmsglen != (int)sizeof(Display_Message)) { MsgError(rcvid, EMSGSIZE); continue; }
        if (info.msglen < (int)sizeof(Display_Message) &&           /* Qnet may deliver only part of it */
            MsgRead(rcvid, &received.message, sizeof(Display_Message), 0) != (int)sizeof(Display_Message)) {
            MsgError(rcvid, EMSGSIZE); continue;
        }
        if (!StatusValid(&received.message, node)) { MsgError(rcvid, EINVAL); continue; }

        Display_Reply reply;
        memset(&reply, 0, sizeof(reply));
        reply.hdr.type = DISPLAY_DATA_TYPE;
        reply.hdr.subtype = DISPLAY_STATUS;
        pthread_mutex_lock(&display->mutex);
        Display_Slot *slot = &display->slots[node];
        slot->status = received.message.status;
        slot->received_ms = NowMs();
        slot->seen = 1;
        slot->server_error = 0;
        if (node < DISPLAY_PED_L1) {            /* traffic nodes: cars leave once their movement has had a green */
            for (int i = 0; i < 12; i++) {
                uint64_t *since = &display->green_since[node][i];
                if (slot->status.colour == 0 && slot->status.active[i]) {
                    if (*since == 0) *since = slot->received_ms;
                    else if (slot->received_ms - *since >= DISPLAY_SERVE_MS) display->command[node].cars[i] = 0;
                } else *since = 0;
            }
        }
        reply.input = display->command[node];
        memset(display->command[node].buttons, 0, sizeof(display->command[node].buttons)); /* presses are one-shot */
        pthread_mutex_unlock(&display->mutex);
        MsgReply(rcvid, EOK, &reply, sizeof(reply));
    }
}
static void *StatusServerThread(void *argument) {
    Display_ServerArguments *args = argument;
    for (;;) {                                  /* if attach/receive fails, retry */
        StatusServer(args->display, args->node);
        int error = errno;
        pthread_mutex_lock(&args->display->mutex);
        args->display->slots[args->node].server_error = error;
        pthread_mutex_unlock(&args->display->mutex);
        Display_SleepMs(DISPLAY_RETRY_MS);
    }
    return NULL;
}

// Convert to JSON to send to python stuff
static int AppendJSON(char *buffer, size_t size, size_t *used, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int written = vsnprintf(buffer + *used, size - *used, format, args);
    va_end(args);
    if (written < 0 || (size_t)written >= size - *used) return -1;
    *used += written;
    return 0;
}
static int BuildJSON(Display_Data *display, char *buffer, size_t size) {
    Display_Slot slots[DISPLAY_NODE_COUNT];
    pthread_mutex_lock(&display->mutex);
    memcpy(slots, display->slots, sizeof(slots));
    pthread_mutex_unlock(&display->mutex);
    uint64_t now = NowMs();
    size_t used = 0;
#define ADD(...) do { if (AppendJSON(buffer, size, &used, __VA_ARGS__) == -1) return -1; } while (0)
    ADD("{\"nodes\":[");
    for (int n = 0; n < DISPLAY_NODE_COUNT; n++) {
        const Display_Slot *s = &slots[n];
        uint64_t age = s->seen && now >= s->received_ms ? now - s->received_ms : 0;
        int colour = s->seen ? s->status.colour : 2;                 // unseen = red, false = unknown
        int state = s->seen ? s->status.state : n < DISPLAY_PED_L1 ? 28 : 0;
        ADD("%s{\"id\":%d,\"seen\":%s,\"age_ms\":%llu,\"colour\":%d,\"state\":%d,\"train\":%d,\"active\":[",
            n ? "," : "", n, s->seen ? "true" : "false", (unsigned long long)age, colour, state,
            s->status.train_detected);
        for (int i = 0; i < 16; i++) ADD("%s%d", i ? "," : "", s->status.active[i]);
        ADD("],\"cars\":[");
        for (int i = 0; i < 12; i++) ADD("%s%d", i ? "," : "", s->status.cars[i]);
        ADD("],\"buttons\":[");
        for (int i = 0; i < 4; i++) ADD("%s%d", i ? "," : "", s->status.buttons[i]);
        ADD("]}");
    }
    ADD("]}");
#undef ADD
    return (int)used;
}

// UDP connection
static int RunCommand(Display_Data *display, const char *line) {
    int a, b, c, ok = 0;
    pthread_mutex_lock(&display->mutex);
    if (sscanf(line, "CAR %d %d %d", &a, &b, &c) == 3) {            /* node, move, count */
        if (a >= DISPLAY_L1 && a <= DISPLAY_L2 && b >= 0 && b < 12 && c >= 0 && c <= 1000) {
            display->command[a].cars[b] = c; ok = 1;
        }
    } else if (sscanf(line, "BUTTON %d %d", &a, &b) == 2) {         /* node, idx */
        if (a >= DISPLAY_PED_L1 && a <= DISPLAY_PED_L2 && b >= 0 && b < 4) {
            display->command[a].buttons[b] = 1; ok = 1;
        }
    } else if (sscanf(line, "TRAIN %d", &a) == 1) {                 /* present */
        if (a == 0 || a == 1) { display->command[DISPLAY_TRAIN].train = a; ok = 1; }
    }
    pthread_mutex_unlock(&display->mutex);
    return ok;
}
static void ServeUDP(Display_Data *display, int fd) {
    for (;;) {
        char request[128], reply[2048];
        struct sockaddr_in from;
        socklen_t from_length = sizeof(from);
        ssize_t n = recvfrom(fd, request, sizeof(request) - 1, 0, (struct sockaddr *)&from, &from_length);
        if (n <= 0) { if (errno != EINTR) Display_SleepMs(10); continue; }
        request[n] = '\0';
        int length;
        if (strncmp(request, "STATE", 5) == 0) length = BuildJSON(display, reply, sizeof(reply));
        else length = snprintf(reply, sizeof(reply), RunCommand(display, request) ? "ok\n" : "bad\n");
        if (length > 0) sendto(fd, reply, (size_t)length, 0, (struct sockaddr *)&from, from_length);
    }
}
static int OpenUDPServer(void) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) return -1;
    struct sockaddr_in a;
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_ANY);
    a.sin_port = htons(DISPLAY_UDP_PORT);
    if (bind(fd, (struct sockaddr *)&a, sizeof(a)) == -1) { int error = errno; close(fd); errno = error; return -1; }
    return fd;
}

int main(void) {
    int udp_fd = OpenUDPServer();
    if (udp_fd == -1) { perror("UDP port 9000"); return EXIT_FAILURE; }
    static Display_Data display = {.mutex = PTHREAD_MUTEX_INITIALIZER};
    static Display_ServerArguments arguments[DISPLAY_NODE_COUNT];
    for (int node = 0; node < DISPLAY_NODE_COUNT; node++) {
        arguments[node] = (Display_ServerArguments){ node, &display };
        pthread_t thread;
        int error = pthread_create(&thread, NULL, StatusServerThread, &arguments[node]);
        if (error) { fprintf(stderr, "pthread_create: %s\n", strerror(error)); return EXIT_FAILURE; }
        pthread_detach(thread);
    }
    printf("Display node ready: status servers Display_From_*, Python UDP port %d\n", DISPLAY_UDP_PORT);
    fflush(stdout);
    ServeUDP(&display, udp_fd);
    close(udp_fd);
    return EXIT_SUCCESS;
}

#ifndef SAMPLECODE_DISPLAY_TRANSPORT_H
#define SAMPLECODE_DISPLAY_TRANSPORT_H
#include "SampleCode_DisplayProtocol.h"
#include <pthread.h>

typedef void (*Display_ReceiveFunction)(void *, const Display_Message *);
uint64_t DisplayNowMs(void);
void DisplaySleepMs(unsigned milliseconds);
int DisplayValidMessage(const Display_Message *msg, int node, int command);
int DisplaySend(int coid, const Display_Message *msg);
int DisplayServer(const char *name, int node, int command,
                  Display_ReceiveFunction receive, void *argument);

/* Local thread arguments only. These pointers are never sent through a channel. */
typedef struct {
    int node;
    void *light;
    pthread_mutex_t *mutex;
    const char *status_path;
    void (*snapshot)(void *, Display_Message *);
    void (*apply_input)(void *, const Display_Message *);
    pthread_t input_thread, status_thread;
} Display_NodeLink;
int StartDisplayInputLink(Display_NodeLink *link);
int StartDisplayNodeLink(Display_NodeLink *link);
#endif

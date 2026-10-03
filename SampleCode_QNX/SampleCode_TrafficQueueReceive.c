/*
 * SampleCode_TrafficQueueReceive.c
 * Source: Lab4_Task2D_Recieve.c (user-supplied lab example).
 * Purpose: Receive traffic input in a thread and safely read its latest value.
 * Adaptation: Added mutex-protected snapshots, error flag and q shutdown; kept separate receiver/state loop.
 * Copy the includes, types and functions you need into your project.
 * Define SAMPLECODE_DEMO to enable the example main(); otherwise no main is built.
 */
#include <stdio.h>
#include <pthread.h>
#include <mqueue.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#define MESSAGESIZE 2
#define QUEUE_NAME "/traffic_v3"

typedef struct {
    const char *queue_name; /* Set before creating the thread. */
    char input;
    int stopped, error;
    pthread_mutex_t mutex;
} traffic_input;

/* Returns a snapshot; no lock is held during a state-machine sleep. */
char ReadTrafficInput(traffic_input *td, int *stopped, int *error)
{
    char input;
    pthread_mutex_lock(&td->mutex);
    input = td->input;
    *stopped = td->stopped;
    *error = td->error;
    pthread_mutex_unlock(&td->mutex);
    return input;
}
void *ThreadReceive(void *data)
{
    traffic_input *td = data;
    char message[MESSAGESIZE];
    struct mq_attr attr;
    int error = 0;
    ssize_t bytes;
    mqd_t queue = mq_open(td->queue_name, O_RDONLY);
    if (queue == (mqd_t)-1) { error = errno; goto finished; }
    if (mq_getattr(queue, &attr) == -1) { error = errno; goto close_queue; }
    if (attr.mq_msgsize != MESSAGESIZE) { error = EMSGSIZE; goto close_queue; }
    for (;;) {
        do { bytes = mq_receive(queue, message, sizeof(message), NULL); }
        while (bytes == -1 && errno == EINTR);
        if (bytes == -1) { error = errno; break; }
        if (bytes != MESSAGESIZE) continue;
        if (message[0] == 'q') break; /* Added demo shutdown command. */
        pthread_mutex_lock(&td->mutex);
        td->input = message[0];
        pthread_mutex_unlock(&td->mutex);
    }
close_queue:
    mq_close(queue);
finished:
    pthread_mutex_lock(&td->mutex);
    td->stopped = 1;
    td->error = error;
    pthread_mutex_unlock(&td->mutex);
    return NULL;
}
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

#ifdef SAMPLECODE_DEMO
int main(void)
{
    traffic_input data = {.queue_name = QUEUE_NAME, .mutex = PTHREAD_MUTEX_INITIALIZER};
    pthread_t th1;
    enum states CurrentState = State0;
    int stopped, error, err = pthread_create(&th1, NULL, ThreadReceive, &data);
    if (err) { errno = err; perror("pthread_create"); return 1; }
    for (;;) {
        char input = ReadTrafficInput(&data, &stopped, &error);
        if (stopped) break;
        SingleStep_TrafficLight_SM(&CurrentState, input);
    }
    pthread_join(th1, NULL);
    pthread_mutex_destroy(&data.mutex);
    if (error) { errno = error; perror("ThreadReceive"); return 1; }
    return 0;
}
#endif

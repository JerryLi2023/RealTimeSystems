/*
 * SampleCode_TrafficQueueSend.c
 * Source: Lab4_Task2D_Send.c (user-supplied lab example).
 * Purpose: Send a two-byte traffic sensor message to a queue.
 * Adaptation: Extracted create/send calls; added q/EOF shutdown so cleanup is reachable.
 * Copy the includes, types and functions you need into your project.
 * Define SAMPLECODE_DEMO to enable the example main(); otherwise no main is built.
 */
#include <stdio.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#define MESSAGESIZE 2
#define QUEUE_NAME "/traffic_v3"

mqd_t CreateTrafficQueue(const char *name)
{
    struct mq_attr attr = {0};
    attr.mq_maxmsg = 1;
    attr.mq_msgsize = MESSAGESIZE;
    return mq_open(name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, &attr);
}
int SendTrafficInput(mqd_t queue, char input)
{
    char message[MESSAGESIZE] = {input, '\0'};
    int result;
    do { result = mq_send(queue, message, sizeof(message), 0); }
    while (result == -1 && errno == EINTR);
    return result;
}
#ifdef SAMPLECODE_DEMO
int main(void)
{
    char input;
    int status = 0;
    mqd_t queue = CreateTrafficQueue(QUEUE_NAME);
    if (queue == (mqd_t)-1) { perror("mq_open"); return 1; }
    puts("Start TrafficQueueReceive. Enter e/n; q stops both programs.");
    for (;;) {
        if (scanf(" %c", &input) != 1) input = 'q';
        if (input != 'e' && input != 'n' && input != 'q') continue;
        if (SendTrafficInput(queue, input) == -1) { perror("mq_send"); status = 1; break; }
        if (input == 'q') break;
    }
    /* Receiver must already have opened the queue, including for the q message. */
    mq_close(queue);
    if (mq_unlink(QUEUE_NAME) == -1) { perror("mq_unlink"); status = 1; }
    return status;
}
#endif

/*
 * SampleCode_MessageQueueReceive.c
 * Source: Lab2_Task3A_recieve.c (user-supplied lab example).
 * Purpose: Open a queue and receive one text message at a time.
 * Adaptation: Added buffer-size/termination checks before using received text.
 * Copy the includes, types and functions you need into your project.
 * Define SAMPLECODE_DEMO to enable the example main(); otherwise no main is built.
 */
#include <stdio.h>
#include <mqueue.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define MESSAGESIZE 1000

/* Queue must already exist. mq_receive blocks until a message arrives.
 * Provide mq_msgsize + 1 bytes: the extra byte safely terminates any received text.
 * Returns byte count, or -1 with errno. Caller opens/closes the descriptor. */
ssize_t ReceiveQueueText(mqd_t qd, char *buf, size_t capacity)
{
    struct mq_attr attr;
    ssize_t bytes;
    if (mq_getattr(qd, &attr) == -1) return -1;
    if (capacity <= (size_t)attr.mq_msgsize) { errno = EMSGSIZE; return -1; }
    do { bytes = mq_receive(qd, buf, capacity - 1, NULL); }
    while (bytes == -1 && errno == EINTR);
    if (bytes >= 0) buf[bytes] = '\0';
    return bytes;
}
#ifdef SAMPLECODE_DEMO
int main(void)
{
    char buf[MESSAGESIZE + 1];
    int status = 0;
    mqd_t qd = mq_open("/test_queue", O_RDONLY);
    if (qd == (mqd_t)-1) { perror("mq_open"); return 1; }
    for (;;) {
        if (ReceiveQueueText(qd, buf, sizeof(buf)) == -1) {
            perror("mq_receive"); status = 1; break;
        }
        printf("dequeue: '%s'\n", buf);
        if (!strcmp(buf, "done")) break;
    }
    mq_close(qd); /* Sender owns unlinking the queue. */
    return status;
}
#endif

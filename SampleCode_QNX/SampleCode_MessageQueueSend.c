/*
 * SampleCode_MessageQueueSend.c
 * Source: Lab2_Task3A_send.c (user-supplied lab example).
 * Purpose: Create a queue and send text messages.
 * Adaptation: Extracted create/send calls, checked errors, and removed long fixed sleeps.
 * Copy the includes, types and functions you need into your project.
 * Define SAMPLECODE_DEMO to enable the example main(); otherwise no main is built.
 */
#include <stdio.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#define MESSAGESIZE 1000

/* Same O_CREAT | O_EXCL ownership as the lab: an existing name is an error.
 * Caller owns the returned descriptor and must mq_close it.
 * Only the queue owner should mq_unlink(name), after receivers have opened it.
 * QNX 7 lab remote path: /net/<hostname>/test_queue; requires Qnet + mqueue. */
mqd_t CreateMessageQueue(const char *name)
{
    struct mq_attr attr = {0};
    attr.mq_maxmsg = 100;
    attr.mq_msgsize = MESSAGESIZE;
    return mq_open(name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, &attr);
}

/* Sends the terminating '\0' too. Blocks if the queue is full. */
int SendQueueText(mqd_t qd, const char *text)
{
    size_t bytes = strlen(text) + 1;
    int result;
    if (bytes > MESSAGESIZE) { errno = EMSGSIZE; return -1; }
    do { result = mq_send(qd, text, bytes, 0); } while (result == -1 && errno == EINTR);
    return result;
}
#ifdef SAMPLECODE_DEMO
int main(void)
{
    const char *name = "/test_queue";
    char buf[MESSAGESIZE];
    int i, status = 0;
    mqd_t qd = CreateMessageQueue(name);
    if (qd == (mqd_t)-1) { perror("mq_open"); return 1; }
    for (i = 1; i <= 5; ++i) {
        snprintf(buf, sizeof(buf), "message %d", i);
        if (SendQueueText(qd, buf) == -1) { perror("mq_send"); status = 1; break; }
    }
    if (!status && SendQueueText(qd, "done") == -1) { perror("mq_send"); status = 1; }
    puts("Start the receiver now. Press Enter AFTER it receives done.");
    getchar();
    mq_close(qd);
    if (mq_unlink(name) == -1) { perror("mq_unlink"); status = 1; }
    return status;
}
#endif

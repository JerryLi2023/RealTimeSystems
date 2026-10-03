/*
 * SampleCode_ThreadScheduling.c
 * Source: Lab2_Task1D.c (user-supplied lab example).
 * Purpose: Create a thread with explicit round-robin priority and optional stack size.
 * Adaptation: Moved attribute setup into CreatePriorityThread; default stack avoids the hard-coded 8000 bytes.
 * Copy the includes, types and functions you need into your project.
 * Define SAMPLECODE_DEMO to enable the example main(); otherwise no main is built.
 */
#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>

/* Returns a pthread error NUMBER (0 = success), not -1/errno.
 * stack_bytes=0 retains the target default; priority limits depend on target/permissions.
 * start and data are the same arguments you normally give pthread_create(). */
int CreatePriorityThread(pthread_t *thread, void *(*start)(void *), void *data,
                         int priority, size_t stack_bytes)
{
    pthread_attr_t attr;
    struct sched_param param = {0};
    int err = pthread_attr_init(&attr);
    if (err) return err;
    param.sched_priority = priority;
    err = pthread_attr_setschedpolicy(&attr, SCHED_RR);
    if (!err) err = pthread_attr_setschedparam(&attr, &param);
    if (!err) err = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    if (!err && stack_bytes) err = pthread_attr_setstacksize(&attr, stack_bytes);
    if (!err) err = pthread_create(thread, &attr, start, data);
    pthread_attr_destroy(&attr);
    return err;
}
#ifdef SAMPLECODE_DEMO
static void *Print_Numbers(void *data)
{
    int i, j;
    (void)data;
    for (i = 0; i < 10; ++i) {
        for (j = 0; j < 10; ++j) printf("%d", j);
        printf("\n");
    }
    return NULL;
}
int main(void)
{
    pthread_t th1;
    int err = CreatePriorityThread(&th1, Print_Numbers, NULL, 1, 0);
    if (err) { fprintf(stderr, "CreatePriorityThread: %s\n", strerror(err)); return 1; }
    pthread_join(th1, NULL);
    return 0;
}
#endif

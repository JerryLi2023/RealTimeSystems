/*
 * SampleCode_Threads.c
 * Source: Lab2_Task1A.c (user-supplied lab example).
 * Purpose: Create threads, pass data and wait for completion.
 * Adaptation: Combined duplicate workers into one parameterised thread function.
 * Copy the includes, types and functions you need into your project.
 * Define SAMPLECODE_DEMO to enable the example main(); otherwise no main is built.
 */
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/neutrino.h>

typedef struct {
    int number;
    unsigned int seconds;
} thread_data;

/* pthread_create requires exactly this void * -> void * function signature. */
void *thread_ex(void *data)
{
    thread_data *td = data;
    printf("Thread %d: PID=%ld, TID=%d\n", td->number, (long)getpid(), gettid());
    /* Replace this work with your sensor-reading/calculation loop. */
    sleep(td->seconds);
    return NULL;
}
#ifdef SAMPLECODE_DEMO
int main(void)
{
    pthread_t th1, th2;
    thread_data first = {1, 2}, second = {2, 2};
    int err;
    /* These structures must remain alive until their threads have finished. */
    err = pthread_create(&th1, NULL, thread_ex, &first);
    if (err) { fprintf(stderr, "pthread_create: %s\n", strerror(err)); return 1; }
    err = pthread_create(&th2, NULL, thread_ex, &second);
    if (err) {
        fprintf(stderr, "pthread_create: %s\n", strerror(err));
        pthread_join(th1, NULL);
        return 1;
    }
    pthread_join(th1, NULL);
    pthread_join(th2, NULL);
    return 0;
}
#endif

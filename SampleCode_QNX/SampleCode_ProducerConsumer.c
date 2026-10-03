/*
 * SampleCode_ProducerConsumer.c
 * Source: Lab3_Task5B.c (user-supplied lab example).
 * Purpose: Reusable circular buffer with semaphore counts and a mutex.
 * Adaptation: Extracted BufferInit/Put/Get/Destroy; removed unused condvar and unrelated scheduling setup.
 * Copy the includes, types and functions you need into your project.
 * Define SAMPLECODE_DEMO to enable the example main(); otherwise no main is built.
 */
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>

#define Buffer_size 10
#define Number_of_packets 15

typedef struct {
    char buffer[Buffer_size];
    int write_index, read_index, count;
    pthread_mutex_t mutex;
    sem_t empty, full;
} app_data;

/* All public buffer operations return 0 or -1/errno. Do not copy an initialised buffer. */
int BufferInit(app_data *td)
{
    int err;
    td->write_index = td->read_index = td->count = 0;
    err = pthread_mutex_init(&td->mutex, NULL);
    if (err) { errno = err; return -1; }
    if (sem_init(&td->empty, 0, Buffer_size) == -1) {
        pthread_mutex_destroy(&td->mutex); return -1;
    }
    if (sem_init(&td->full, 0, 0) == -1) {
        sem_destroy(&td->empty); pthread_mutex_destroy(&td->mutex); return -1;
    }
    return 0;
}
static int WaitSemaphore(sem_t *sem)
{
    int result;
    do { result = sem_wait(sem); } while (result == -1 && errno == EINTR);
    return result;
}
int BufferPut(app_data *td, char item)
{
    int err;
    if (WaitSemaphore(&td->empty) == -1) return -1; /* Wait for a free slot. */
    err = pthread_mutex_lock(&td->mutex);
    if (err) { sem_post(&td->empty); errno = err; return -1; }
    td->buffer[td->write_index] = item;
    td->write_index = (td->write_index + 1) % Buffer_size;
    ++td->count;
    pthread_mutex_unlock(&td->mutex);
    return sem_post(&td->full); /* One more item is ready. */
}
int BufferGet(app_data *td, char *item)
{
    int err;
    if (WaitSemaphore(&td->full) == -1) return -1; /* Wait for an item. */
    err = pthread_mutex_lock(&td->mutex);
    if (err) { sem_post(&td->full); errno = err; return -1; }
    *item = td->buffer[td->read_index];
    td->read_index = (td->read_index + 1) % Buffer_size;
    --td->count;
    pthread_mutex_unlock(&td->mutex);
    return sem_post(&td->empty); /* One more slot is free. */
}
void BufferDestroy(app_data *td)
{
    /* Call only AFTER all producer/consumer threads have finished. */
    sem_destroy(&td->empty);
    sem_destroy(&td->full);
    pthread_mutex_destroy(&td->mutex);
}
#ifdef SAMPLECODE_DEMO
#include <stdlib.h>
static void *producer(void *data)
{
    int i;
    for (i = 0; i < Number_of_packets; ++i)
        if (BufferPut(data, (char)('A' + i)) == -1) { perror("BufferPut"); exit(1); }
    return NULL;
}
static void *consumer(void *data)
{
    int i;
    char item;
    for (i = 0; i < Number_of_packets; ++i) {
        if (BufferGet(data, &item) == -1) { perror("BufferGet"); exit(1); }
        printf("consumer: %c\n", item);
    }
    return NULL;
}
int main(void)
{
    app_data data;
    pthread_t th1, th2;
    int err;
    if (BufferInit(&data) == -1) { perror("BufferInit"); return 1; }
    err = pthread_create(&th1, NULL, producer, &data);
    if (err) { errno = err; perror("pthread_create"); return 1; }
    err = pthread_create(&th2, NULL, consumer, &data);
    if (err) { errno = err; perror("pthread_create"); return 1; }
    pthread_join(th1, NULL);
    pthread_join(th2, NULL);
    BufferDestroy(&data);
    return 0;
}
#endif

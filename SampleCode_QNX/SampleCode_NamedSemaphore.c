/*
 * SampleCode_NamedSemaphore.c
 * Source: Lab3_Task3A.c (user-supplied lab example).
 * Original lab also credits John Fehr, "Protecting Your Data in a Multi-Threaded App".
 * http://www.qnx.com/developers/articles/article_301_2.html
 * Purpose: Protect shared data with a named semaphore.
 * Adaptation: Kept user/changer logic; moved lock into data, removed unused fields, protected loop condition.
 * Copy the includes, types and functions you need into your project.
 * Define SAMPLECODE_DEMO to enable the example main(); otherwise no main is built.
 */
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

typedef struct {
    int a, b, result, use_count, max_use;
    sem_t *sem;
} app_data;

/* Retry an interrupted wait; never touch shared data without taking the lock. */
static int WaitSemaphore(sem_t *sem)
{
    int result;
    do { result = sem_wait(sem); } while (result == -1 && errno == EINTR);
    return result;
}

void *user_thread(void *data)
{
    app_data *td = data;
    int uses = 0;
    while (uses < td->max_use) { /* max_use is fixed before threads start. */
        if (WaitSemaphore(td->sem) == -1) return (void *)1;
        if (td->a == 5) {
            td->result += td->a + td->b;
            ++td->use_count;
            ++uses;
        }
        sem_post(td->sem);
        usleep(1);
    }
    return NULL;
}

void *changer_thread(void *data)
{
    app_data *td = data;
    for (;;) {
        if (WaitSemaphore(td->sem) == -1) return (void *)1;
        /* FIX: this shared count was read outside the lock in the original. */
        if (td->use_count >= td->max_use) { sem_post(td->sem); break; }
        td->a = (td->a == 5) ? 50 : 5;
        usleep(1000); /* Original simulated calculation, while protected. */
        td->b = td->a;
        sem_post(td->sem);
        usleep(1);
    }
    return NULL;
}
#ifdef SAMPLECODE_DEMO
int main(void)
{
    app_data td = {.a = 5, .b = 5, .max_use = 100};
    pthread_t ut, ct;
    void *ur, *cr;
    int err;
    /* Exclusive creation avoids reusing an old semaphore with an unknown value.
     * Other processes can sem_open("/my_sem", 0) to join this SAME lock.
     * It does NOT make td shared between processes; that needs shared memory. */
    td.sem = sem_open("/my_sem", O_CREAT | O_EXCL, 0600, 1);
    if (td.sem == SEM_FAILED) { perror("sem_open"); return 1; }
    err = pthread_create(&ut, NULL, user_thread, &td);
    if (err) { fprintf(stderr, "pthread_create: %s\n", strerror(err)); sem_close(td.sem); sem_unlink("/my_sem"); return 1; }
    err = pthread_create(&ct, NULL, changer_thread, &td);
    if (err) {
        /* a,b stay at 5 without the changer, so user_thread can finish. */
        pthread_join(ut, NULL);
        fprintf(stderr, "pthread_create: %s\n", strerror(err));
        sem_close(td.sem); sem_unlink("/my_sem");
        return 1;
    }
    pthread_join(ut, &ur);
    pthread_join(ct, &cr);
    sem_close(td.sem); sem_unlink("/my_sem");
    printf("result should be %d, is %d\n", td.max_use * 10, td.result);
    return ur != NULL || cr != NULL || td.result != td.max_use * 10;
}
#endif

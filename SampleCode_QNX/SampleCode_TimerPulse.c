/*
 * SampleCode_TimerPulse.c
 * Source: Lab4_Task3B.c (user-supplied lab example).
 * Purpose: Create a periodic timer that sends QNX pulses to a channel.
 * Adaptation: Extracted Start/StopPulseTimer; fixed error printing, priority query, interval comments and cleanup.
 * Copy the includes, types and functions you need into your project.
 * Define SAMPLECODE_DEMO to enable the example main(); otherwise no main is built.
 */
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <sched.h>
#include <errno.h>
#include <sys/netmgr.h>
#include <sys/neutrino.h>

#define MY_PULSE_CODE _PULSE_CODE_MINAVAIL

typedef struct {
    int chid, coid;
    timer_t timer_id;
} pulse_timer;

/* seconds/nanoseconds set BOTH initial delay and repeat interval.
 * Returns 0 or -1/errno. Call only on a fresh/inactive object.
 * Owner receives from t->chid. Timer and connection belong to that owner. */
int StartPulseTimer(pulse_timer *t, time_t seconds, long nanoseconds)
{
    struct sigevent event = {0};
    struct itimerspec itime = {0};
    struct sched_param param;
    int policy, err, saved;
    if (seconds < 0 || nanoseconds < 0 || nanoseconds >= 1000000000L ||
        (seconds == 0 && nanoseconds == 0)) { errno = EINVAL; return -1; }
    t->chid = ChannelCreate(0);
    if (t->chid == -1) return -1;
    t->coid = ConnectAttach(ND_LOCAL_NODE, 0, t->chid, _NTO_SIDE_CHANNEL, 0);
    if (t->coid == -1) goto fail_channel;
    err = pthread_getschedparam(pthread_self(), &policy, &param);
    if (err) { errno = err; goto fail_connection; }
    SIGEV_PULSE_INIT(&event, t->coid, param.sched_priority, MY_PULSE_CODE, 0);
    if (timer_create(CLOCK_REALTIME, &event, &t->timer_id) == -1) goto fail_connection;
    itime.it_value.tv_sec = seconds;
    itime.it_value.tv_nsec = nanoseconds;
    itime.it_interval = itime.it_value;
    if (timer_settime(t->timer_id, 0, &itime, NULL) == 0) return 0;
    saved = errno;
    timer_delete(t->timer_id);
    errno = saved;
fail_connection:
    saved = errno;
    ConnectDetach(t->coid);
    errno = saved;
fail_channel:
    saved = errno;
    ChannelDestroy(t->chid);
    errno = saved;
    return -1;
}
void StopPulseTimer(pulse_timer *t)
{
    /* Call once, only after successful StartPulseTimer and after receiving stops. */
    timer_delete(t->timer_id);
    ConnectDetach(t->coid);
    ChannelDestroy(t->chid);
}

enum states {State0, State1, State2, State3, State4, State5, State6, State7};
/* Original non-sleeping, tick-counted state machine: green lasts two ticks. */
void SingleStep_TrafficLight_SM(enum states *CurrentState, int *ticks_in_state) {

    enum states NextState = *CurrentState;
    int required_ticks = 1;

    switch (*CurrentState){
        case State0:
            printf("East-West: Red | North-South: Red\n");
            NextState = State1;
            required_ticks = 1;
            break;

        case State1:
            printf("East-West: Red | North-South: Red\n");
            NextState = State2;
            required_ticks = 1;
            break;

        case State2:
            printf("East-West: Green | North-South: Red\n");
            NextState = State3;
            required_ticks = 2;
            break;

        case State3:
            printf("East-West: Yellow | North-South: Red\n");
            NextState = State4;
            required_ticks = 1;
            break;

        case State4:
            printf("East-West: Red | North-South: Red\n");
            NextState = State5;
            required_ticks = 1;
            break;

        case State5:
            printf("East-West: Red | North-South: Green\n");
            NextState = State6;
            required_ticks = 2;
            break;

        case State6:
            printf("East-West: Red | North-South: Yellow\n");
            NextState = State7;
            required_ticks = 1;
            break;

        case State7:
            printf("East-West: Red | North-South: Red\n");
            NextState = State0;
            required_ticks = 1;
            break;
    }

    (*ticks_in_state)++;

    if (*ticks_in_state >= required_ticks) {
        *CurrentState = NextState;
        *ticks_in_state = 0;
    }

    fflush(stdout);
}

#ifdef SAMPLECODE_DEMO
int main(void)
{
    pulse_timer timer;
    struct _pulse msg;
    enum states CurrentState = State0;
    int counter = 0, ticks_in_state = 0, rcvid, status = 0;
    /* Original executable used 1.0 s despite comments saying 1.5 s.
     * For 1.5 s use StartPulseTimer(&timer, 1, 500000000L). */
    if (StartPulseTimer(&timer, 1, 0) == -1) { perror("StartPulseTimer"); return 1; }
    while (counter < 10) {
        rcvid = MsgReceive(timer.chid, &msg, sizeof(msg), NULL);
        if (rcvid == -1) {
            if (errno == EINTR) continue;
            perror("MsgReceive"); status = 1; break;
        }
        if (rcvid == 0 && msg.code == MY_PULSE_CODE) {
            SingleStep_TrafficLight_SM(&CurrentState, &ticks_in_state);
            ++counter;
        } else if (rcvid > 0) {
            MsgError(rcvid, ENOSYS); /* Don't leave an unexpected sender blocked. */
        }
    }
    StopPulseTimer(&timer);
    return status;
}
#endif

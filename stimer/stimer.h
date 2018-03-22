/*
 * Simple Linux timer wrapper
 * File: "stimer.h"
 */

#ifndef STIMER_H
#define STIMER_H
//-----------------------------------------------------------------------------
#include <time.h>   // clock_gettime(), clock_getres(), time_t, nanosleep(),...
#include <signal.h> // sigaction(),  sigemptyset(), sigprocmask()
#include <sched.h>  // sched_setscheduler(), SCHED_FIFO, ...
#include <stdint.h> // `uint32_t`
#include <stdio.h>  // `FILE`, fprintf()
//-----------------------------------------------------------------------------
// timer used
//   CLOCK_REALTIME CLOCK_MONOTONIC
//   CLOCK_PROCESS_CPUTIME_ID CLOCK_THREAD_CPUTIME_ID (since Linux 2.6.12)
//   CLOCK_REALTIME_HR CLOCK_MONOTONIC_HR (MontaVista)
#define STIMER_CLOCKID CLOCK_REALTIME

// timer signal
#define STIMER_SIG SIGRTMIN

// seconds per day (24*60*60)
#define STIMER_SECONDS_PER_DAY 86400.

//----------------------------------------------------------------------------
// `ti_t` type structure
typedef struct stimer_ {
  int stop;
  int (*fn)(void *context);
  void *context;
  unsigned overrun;
  sigset_t mask;
  struct sigevent sev;
  struct sigaction sa;
  struct itimerspec ival;
  timer_t timerid;
} stimer_t;
//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
//----------------------------------------------------------------------------
// set the process to real-time privs via call sched_setscheduler()
int stimer_realtime();
//----------------------------------------------------------------------------
// get day time (0...86400 seconds)
double stimer_daytime();
//----------------------------------------------------------------------------
// limit daytime to 0..24h
double stimer_limit_daytime(double t);
//----------------------------------------------------------------------------
// limit difference of daytime to -12h..12h
double stimer_limit_delta(double t);
//----------------------------------------------------------------------------
// convert time in seconds to `struct timespec`
struct timespec stimer_double_to_ts(double t);
//----------------------------------------------------------------------------
// sleep [ms] (based on standart nanosleep())
void stimer_sleep_ms(double ms);
//----------------------------------------------------------------------------
// print day time to file in next format: HH:MM:SS.mmmuuu
void stimer_fprint_daytime(FILE *stream, double daytime);
//----------------------------------------------------------------------------
// set SIGINT (CTRL+C) user handler
int stimer_sigint(void (*fn)(void *context), void *context);
//----------------------------------------------------------------------------
// init timer
int stimer_init(stimer_t *self, int (*fn)(void *context), void *context);
//----------------------------------------------------------------------------
// start timer
int stimer_start(stimer_t *self, double interval_ms);
//----------------------------------------------------------------------------
// stop timer
void stimer_stop(stimer_t *self);
//----------------------------------------------------------------------------
// timer main loop
int stimer_loop(stimer_t *self);
//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif // __cplusplus
//----------------------------------------------------------------------------
#endif // STIMER_H

/*** end of "stimer.h" file ***/


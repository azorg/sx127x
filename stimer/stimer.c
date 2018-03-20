/*
 * Simple Linux timer wrapper
 * File: "stimer.c"
 */
//----------------------------------------------------------------------------
#include "stimer.h" // `stimer_t`
#include <string.h> // memset()
#include <stdio.h>  // perror()
#include <unistd.h> // pause()
//-----------------------------------------------------------------------------
typedef struct stimer_sigint_ {
  void (*fn)(void *context);
  void *context;
} stimer_sigint_t;
//-----------------------------------------------------------------------------
static stimer_sigint_t stimer_si;
//-----------------------------------------------------------------------------
// set the process to real-time privs via call sched_setscheduler()
int stimer_realtime()
{
  int retv;
  struct sched_param schp;
  memset(&schp, 0, sizeof(schp));
  schp.sched_priority = sched_get_priority_max(SCHED_FIFO);

  retv = sched_setscheduler(0, SCHED_FIFO, &schp);
  if (retv)
    perror("error: sched_setscheduler(SCHED_FIFO)");

  return retv;
}
//-----------------------------------------------------------------------------
// get day time (0...86400 seconds)
double stimer_daytime()
{
  struct timespec ts;
  struct tm tm;
  time_t time;
  double t;

  //!!! FIXME
#if 1
  clock_gettime(CLOCK_REALTIME, &ts);
#else
  clock_gettime(CLOCK_MONOTONIC, &ts);
#endif

  time = (time_t) ts.tv_sec;
  localtime_r(&time, &tm);
  time = tm.tm_sec + tm.tm_min * 60 + tm.tm_hour * 3600;

  t  = ((double) ts.tv_nsec) * 1e-9;
  t += ((double) time);
  return t;
}
//----------------------------------------------------------------------------
// limit daytime to 0..24h
double stimer_limit_daytime(double t)
{
  if (t < 0.)
    do
      t += STIMER_SECONDS_PER_DAY;
    while (t < 0.);
  else
    while (t >= STIMER_SECONDS_PER_DAY)
      t -= STIMER_SECONDS_PER_DAY;

  return t;
}
//----------------------------------------------------------------------------
// limit difference of daytime to -12h..12h
double stimer_limit_delta(double t)
{
  if (t < (-0.5 * STIMER_SECONDS_PER_DAY))
    do
      t += STIMER_SECONDS_PER_DAY;
    while (t < (-0.5 * STIMER_SECONDS_PER_DAY));
  else
    while (t > (0.5 * STIMER_SECONDS_PER_DAY))
      t -= STIMER_SECONDS_PER_DAY;

  return t;
}
//-----------------------------------------------------------------------------
// convert time in seconds to `struct timespec`
struct timespec stimer_double_to_ts(double t)
{
  struct timespec ts;
  ts.tv_sec  = (time_t) t;
  ts.tv_nsec = (long) ((t - (double) ts.tv_sec) * 1e9);
  return ts;
}
//----------------------------------------------------------------------------
// print day time to file in next format: HH:MM:SS.mmmuuu
void stimer_fprint_daytime(FILE *stream, double t)
{
  unsigned h, m, s, us;
  s = (unsigned) t;
  h =  s / 3600;     // hours
  m = (s / 60) % 60; // minutes
  s =  s       % 60; // seconds
  t -= (double) (h * 3600 + m * 60 + s);
  us = (unsigned) (t * 1e6);
  fprintf(stream, "%02u:%02u:%02u.%06u", h, m, s, us);
}
//-----------------------------------------------------------------------------
// SIGINT handler (Ctrl-C)
static void stimer_sigint_handler(int signo)
{
  if (signo == SIGINT)
  {
    if (stimer_si.fn != (void (*)(void*)) NULL)
      stimer_si.fn(stimer_si.context); // callback user function
  }
} 
//-----------------------------------------------------------------------------
// SIGINT sigaction (Ctrl-C)
static void stimer_sigint_action(int signo, siginfo_t *si, void *ucontext)
{
  stimer_sigint_handler(signo);
} 
//-----------------------------------------------------------------------------
// timer handler
static void stimer_handler(int signo, siginfo_t *si, void *ucontext)
{
  if (signo == STIMER_SIG && si->si_code == SI_TIMER)
  {
    stimer_t *timer = (stimer_t*) si->si_value.sival_ptr;
    int retv = timer_getoverrun(timer->timerid);
    if (retv < 0)
    {
      perror("timer_getoverrun() failed");
    }
    else
      timer->overrun += retv;
  }
}
//----------------------------------------------------------------------------
// set SIGINT (CTRL+C) user handler
int stimer_sigint(void (*fn)(void *context), void *context)
{
#if 1
  struct sigaction sa;
  memset((void*) &sa, 0, sizeof(sa));
#if 1
  sa.sa_sigaction = stimer_sigint_action;
#else
  sa.sa_handler = stimer_sigint_handler;
#endif
  sigemptyset(&sa.sa_mask);
  //sa.sa_flags = 0;

  stimer_si.fn      = fn;
  stimer_si.context = context;
  
  if (sigaction(SIGINT, &sa, NULL) < 0)
  {
    perror("error in stimer_sigint_handler(): sigaction() failed; return -1");
    return -1;
  }
#else
  stimer_si.fn      = fn;
  stimer_si.context = context;
  
  if (signal(SIGINT, stimer_sigint_handler) < 0) // old school ;-)
  {
    perror("error in stimer_sigint_handler(): signal() failed; return -1");
    return -1;
  }
#endif
  return 0;
}
//----------------------------------------------------------------------------
// init timer
int stimer_init(stimer_t *self, int (*fn)(void *context), void *context)
{
  // save user callback funcion
  self->fn = fn;

  // save context
  self->context = context;

  // reset stop flag
  self->stop = 0;

  // reset overrrun counter
  self->overrun = 0;

  // establishing handler for signal STIMER_SIG
  memset((void*) &self->sa, 0, sizeof(self->sa));
  self->sa.sa_flags = SA_SIGINFO | SA_RESTART;
  self->sa.sa_sigaction = stimer_handler;
  sigemptyset(&self->sa.sa_mask);
  if (sigaction(STIMER_SIG, &self->sa, NULL) < 0)
  {
    perror("error in stimer_init(): sigaction() failed; return -1");
    return -1;
  }

  // block signal STIMER_SIG temporarily
  sigemptyset(&self->mask);
  sigaddset(&self->mask, STIMER_SIG);
  if (sigprocmask(SIG_SETMASK, &self->mask, NULL) < 0)
  {
    perror("error in stimer_init(): sigprocmask(SIG_SETMASK) failed; return -2");
    return -2;
  }

  // create timer with ID = timerid
  memset((void*) &self->sev, 0, sizeof(self->sev));
  self->sev.sigev_notify = SIGEV_SIGNAL; // SIGEV_NONE SIGEV_SIGNAL SIGEV_THREAD...
  self->sev.sigev_signo  = STIMER_SIG;
#if 1
  self->sev.sigev_value.sival_ptr = (void*) self; // very important!
#else
  self->sev.sigev_value.sival_int = 1; // use sival_ptr instead!
#endif
  //self->sev.sigev_notify_function     = ...; // for SIGEV_THREAD
  //self->sev.sigev_notify_attributes   = ...; // for SIGEV_THREAD
  //self->sigev.sigev_notify_thread_id  = ...; // for SIGEV_THREAD_ID
  if (timer_create(STIMER_CLOCKID, &self->sev, &self->timerid) < 0)
  {
    perror("error in stimer_init(): timer_create() failed; return -3");
    return -3;
  }

  return 0;
}
//----------------------------------------------------------------------------
// start timer
int stimer_start(stimer_t *self, double interval_ms)
{
  // unblock signal STIMER_SIG
  sigemptyset(&self->mask);
  sigaddset(&self->mask, STIMER_SIG);
  if (sigprocmask(SIG_UNBLOCK, &self->mask, NULL) < 0)
  {
    perror("error in stimer_start(): sigprocmask(SIG_UNBLOCK) failed; return -1");
    return -1;
  }

  // start time
  self->ival.it_value    = stimer_double_to_ts(((double) interval_ms) * 1e-3);
  self->ival.it_interval = self->ival.it_value;
  if (timer_settime(self->timerid, 0, &self->ival, NULL) < 0)
  {
    perror("error in stimer_start(): timer_settime() failed; return -2");
    return -2;
  }

  return 0;
}
//----------------------------------------------------------------------------
// stop timer
void stimer_stop(stimer_t *self)
{
  self->stop = 1;
}
//----------------------------------------------------------------------------
// timer main loop
int stimer_loop(stimer_t *self)
{
  for (;;)
  {
    // unlock timer signal
    sigemptyset(&self->mask);
    sigaddset(&self->mask, STIMER_SIG);
    if (sigprocmask(SIG_UNBLOCK, &self->mask, NULL) < 0)
    {
      perror("error in stimer_main_loop(): sigprocmask() failed #1; exit");
      return -1;
    }

    // sleep and wait signal
    pause();
    
    if (self->stop) return 0;

    // lock timer signal for normal select() work
    if (sigprocmask(SIG_BLOCK, &self->mask, NULL) < 0)
    {
      perror("error in stimer_main_loop(): sigprocmask() failed #2; exit");
      return -1;
    }

    if (self->fn != (int (*)(void*)) NULL)
    { // callback user function
      int retv = self->fn(self->context);
      if (retv)
        return retv;
    }
  } // for(;;)
}
//----------------------------------------------------------------------------
/*** end of "stimer.c" file ***/


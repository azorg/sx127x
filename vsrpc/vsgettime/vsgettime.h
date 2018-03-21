/*
 * Very simple POSIX clock_gettime() wrapper
 * File: "vsgettime.h"
 * Windows implemtation from
 *   http://stackoverflow.com/questions/5404277/porting-clock-gettime-to-windows
 */

//----------------------------------------------------------------------------
#ifndef VSGETTIME_H
#define VSGETTIME_H
//----------------------------------------------------------------------------
#include <time.h> // time_t, struct timespec, clock_gettime()
//----------------------------------------------------------------------------
#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))
#  ifndef VSWIN32
#    define VSWIN32
#  endif
#endif
//----------------------------------------------------------------------------
#ifdef VSWIN32
#  ifndef CLOCK_REALTIME
#    define CLOCK_REALTIME 0
#  endif

struct timespec {
  time_t tv_sec;  // seconds
  long   tv_nsec; // nanoseconds
};

typedef int clockid_t;
#endif // VSWIN32
//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
//----------------------------------------------------------------------------
#ifdef VSWIN32
int clock_gettime(clockid_t clk_id, struct timespec *tp);
#endif
//----------------------------------------------------------------------------
// return real time in seconds
     double vsgettime();    // ~64 bit
long double vsgettime_ng(); // ~96..128 bit
//----------------------------------------------------------------------------
// convert seconds in `double` to `struct timespec`
struct timespec vsgettime_timespec(double t);
//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif // __cplusplus
//----------------------------------------------------------------------------
#endif // VSGETTIME_H
//----------------------------------------------------------------------------

/*** end of "vsgettime.h" file ***/

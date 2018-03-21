/*
 * Very simple POSIX clock_gettime() wrapper
 * File: "vsgettime.h"
 * Windows implemtation from
 *   http://stackoverflow.com/questions/5404277/porting-clock-gettime-to-windows
 */

//----------------------------------------------------------------------------
#include "vsgettime.h"
//----------------------------------------------------------------------------
#ifdef VSWIN32
#include <windows.h>
//----------------------------------------------------------------------------
static LARGE_INTEGER _getFILETIMEoffset()
{
  SYSTEMTIME s;
  FILETIME f;
  LARGE_INTEGER t;

  s.wYear = 1970;
  s.wMonth = 1;
  s.wDay = 1;
  s.wHour = 0;
  s.wMinute = 0;
  s.wSecond = 0;
  s.wMilliseconds = 0;

  SystemTimeToFileTime(&s, &f);

  t.QuadPart = f.dwHighDateTime;
  t.QuadPart <<= 32;
  t.QuadPart |= f.dwLowDateTime;

  return (t);
}
//----------------------------------------------------------------------------
int clock_gettime(clockid_t clk_id, struct timespec *tp)
{
  LARGE_INTEGER t;
  FILETIME f;
  double microseconds;
  static LARGE_INTEGER offset;
  static double frequencyToMicroseconds;
  static int initialized = 0;
  static BOOL usePerformanceCounter = 0;

  if (!initialized)
  { // first run
    LARGE_INTEGER performanceFrequency;
    clk_id = clk_id; // unused
    initialized = 1;
    usePerformanceCounter = QueryPerformanceFrequency(&performanceFrequency);
    if (usePerformanceCounter)
    {
      QueryPerformanceCounter(&offset);
      frequencyToMicroseconds = (double) performanceFrequency.QuadPart / 1000000.;
    }
    else
    {
      offset = _getFILETIMEoffset();
      frequencyToMicroseconds = 10.;
    }
  }

  if (usePerformanceCounter)
    QueryPerformanceCounter(&t);
  else
  {
    GetSystemTimeAsFileTime(&f);
    t.QuadPart = f.dwHighDateTime;
    t.QuadPart <<= 32;
    t.QuadPart |= f.dwLowDateTime;
  }

  t.QuadPart -= offset.QuadPart;
  microseconds = (double)t.QuadPart / frequencyToMicroseconds;
  t.QuadPart = microseconds;

  tp->tv_sec  =  t.QuadPart / 1000000;
  tp->tv_nsec = (t.QuadPart % 1000000) * 1000;
  return 0;
}
#endif // VSWIN32
//----------------------------------------------------------------------------
// return real time in seconds
double vsgettime()
{
  struct timespec ts;
  double t;

  clock_gettime(CLOCK_REALTIME, &ts);
  t  = ((double) ts.tv_nsec) * 1e-9; // ns -> sec
  t += ((double) ts.tv_sec);

  return t;
}
//----------------------------------------------------------------------------
// return real time in seconds
long double vsgettime_ng()
{
  struct timespec ts;
  long double t;

  clock_gettime(CLOCK_REALTIME, &ts);
  t  = ((long double) ts.tv_nsec) * 1e-9L; // ns -> sec
  t += ((long double) ts.tv_sec);

  return t;
}
//-----------------------------------------------------------------------------
// convert seconds in `double` to `struct timespec`
struct timespec vsgettime_timespec(double t)
{
  struct timespec ts;
  ts.tv_sec  = (time_t) t;
  ts.tv_nsec = (long) ((t - (double) ts.tv_sec) * 1e9);
  return ts;
}
//----------------------------------------------------------------------------
#ifdef VSGETTIME_TEST
#include <stdio.h>
int main()
{
  double t1, t2;
  long double T1, T2;
  int i, j;
  
  T1 = vsgettime_ng();
  T2 = vsgettime_ng();
  printf("minimal dt = %.3Lf [us]\n", (T2 - T1) * 1e6L);
  printf("\"real\" time = %.3Lf [sec]\n", T2);

  t1 = vsgettime();
  for (i = 0; i < 1000000; i++)
    for (j = 0; j < 1000; j++)
      ; // do nothing
  t2 = vsgettime();

  printf("stupid cycle dt = %.9f [sec]\n", t2 - t1);
  printf("\"real\" time = %.3f [sec]\n", t2);

  return 0;
}
//----------------------------------------------------------------------------
#endif // VSGETTIME_TEST

/*** end of "vsgettime.c" file ***/


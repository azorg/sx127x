/*
 * Very Simple Thread Implementation (VSRPC project)
 * Version: 0.9
 * File: "vsthread.h"
 * (C) 2007-2015 Alex Grinkov <a.grinkov@gmail.com>
 * (C) 2008 Anton Shmigirilov <shmigirilov@gmail.com>
 */

#ifndef VSTHREAD_H
#define VSTHREAD_H
//----------------------------------------------------------------------------
#include "vssync.h"
//----------------------------------------------------------------------------
// version for Linux
//#define VSTHREAD_LINUX

// version for Linux with "Real Time threads"
//#define VSTHREAD_LINUX_RT

// version for eCos
//#define VSTHREAD_ECOS

// version for OS2000 (WxWorks clone)
//#define VSTHREAD_OS2000

// include debuging output
//#define VSTHREAD_DEBUG
//----------------------------------------------------------------------------
#ifdef VSTHREAD_ECOS

// create threads pool
#  define VSTHREAD_POOL

// threads stack size [bytes] (if use pool)
// may be not defined
#  ifndef VSTHREAD_STACKSIZE
#    define VSTHREAD_STACKSIZE 8192
#  endif

#endif // VSTHREAD_ECOS
//----------------------------------------------------------------------------
#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))
#  ifndef VSWIN32
#    define VSWIN32
#  endif
#  undef VSTHREAD_POOL
#endif
//----------------------------------------------------------------------------
#ifdef VSTHREAD_DEBUG
#  include <stdio.h>  // fprintf(), vfprintf()
#  include <string.h> // strerror()
#  if defined(__GNUC__)
#    define VSTHREAD_DBG(fmt, arg...) fprintf(stderr, "VSTHREAD: " fmt "\n", ## arg)
#  elif defined(VSWIN32)
#    define VSTHREAD_DBG(fmt, ...) fprintf(stderr, "VSTHREAD: " fmt "\n", __VA_ARGS__)
#  elif defined(__BORLANDC__)
#    include <stdarg.h> // va_list, va_start(), va_end()
void VSTHREAD_DBG(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "VSTHREAD: ");
  vfprintf(stderr, fmt, ap);
  va_end(ap);
}
#  else
#    warning "unknown compiler"
#    define VSTHREAD_DBG(fmt, arg...) fprintf(stderr, "VSTHREAD: " fmt "\n", ## arg)
#  endif
#else
#  define VSTHREAD_DBG(fmt, ...) // debug output off
#endif // VSTHREADS_DEBUG
//----------------------------------------------------------------------------
#ifdef VSWIN32
// SCHED_* constants unused under Windows
# ifndef SCHED_OTHER
#   define SCHED_OTHER    1
# endif
# ifndef SCHED_RR
#   define SCHED_RR       2
# endif
# ifndef SCHED_FIFO
#   define SCHED_FIFO     3
# endif
# ifndef SCHED_DEADLINE
#   define SCHED_DEADLINE 4
# endif
#endif // VSWIN32
//----------------------------------------------------------------------------
//
// Structures
//

// Very Simple Thread handle
typedef struct {
#ifdef VSWIN32
  HANDLE handle;    // handle of WIN32 thread
  DWORD  id;        // ID of thread
#else // VSWIN32
  pthread_t pthread; // handle of POSIX thread
#endif
} vsthread_t;

#ifdef VSTHREAD_POOL
typedef struct {
  pthread_t pthread;    // handle of POSIX thread
  vssem_t sem_start;      // semaphore to start thread
  vssem_t sem_free;       // semaphore set if thread free
  vssem_t sem_finish;     // semaphore set if thread finish
  vssem_t sem_kill;       // semaphore to kill thread
  void *(*func)(void*); // thread function
  void *arg;            // argument for thread function
  void *ret;            // return value of thread function
#ifdef VSTHREAD_STACKSIZE
  char stack[VSTHREAD_STACKSIZE];
#endif // VSTHREAD_STACKSIZE
} vsthread_record_t;

typedef struct {
  vssem_t sem_list;          // semaphore for modify pool list
  int size;                // size of list
  vsthread_record_t *list; // pool list
  int priority; int sched; // POSIX threads attributes
} vsthread_pool_t;
#endif // VSTHREAD_POOL

//----------------------------------------------------------------------------
//
// Functions
//

#ifdef __cplusplus
extern "C" {
#endif // __plusplus

#ifdef VSTHREAD_POOL

// create treads pool (start all threads before)
int vsthread_pool_init(vsthread_pool_t *pool, int poolsize,
                       int priority, int sched);

// destroy pool of all startted thread
void vsthread_pool_destroy(vsthread_pool_t *pool);

// create thread (wakeup thread in pool)
int vsthread_create(
  vsthread_pool_t *pool,
  vsthread_t *thread, void *(*func)(void *), void *arg);

// cancel thread in pool
int vsthread_cancel(vsthread_pool_t *pool, vsthread_t thread);

// join to thread in pool
int vsthread_join(
  vsthread_pool_t *pool,
  vsthread_t thread, void **thread_return);

#else // !VSTHREAD_POOL

// create POSIX thread
int vsthread_create(
  int priority, int sched,
  vsthread_t *thread, void *(*func)(void *), void *arg);

// cancel POSIX thread
int vsthread_cancel(vsthread_t thread);

// join to POSIX thread
int vsthread_join(vsthread_t thread, void **thread_return);

#endif // VSTHREAD_POOL

//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif // __plusplus
//----------------------------------------------------------------------------
#endif // VSTHREAD_H

/*** end of "vsthread.h" file ***/


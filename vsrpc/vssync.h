/*
 * Very simple wrappers over syncronization primitives in POSIX way
 * Version: 0.9
 * File: "vssync.h"
 * (C) 2008 Anton Shmigirilov <shmigirilov@gmail.com>
 * (C) 2008-2015 Alex Grinkov <a.grinkov@gmail.com>
 */

//----------------------------------------------------------------------------
#ifndef VSSYNC_H
#define VSSYNC_H
//----------------------------------------------------------------------------
#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))
#  define VSWIN32
#endif
//----------------------------------------------------------------------------
#ifdef VSWIN32
#  include <windows.h>
#else
#  include <semaphore.h> // sem_t
#  include <pthread.h>   // pthread_mutex_t
#endif
//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
//----------------------------------------------------------------------------
// 'like POSIX' semaphores implementation
typedef struct
{
#ifdef VSWIN32
  HANDLE win_sem;
#else
  sem_t pth_sem;
#endif
}
vssem_t;
//----------------------------------------------------------------------------
// create semaphore object
int vssem_init(vssem_t * sem, int pshared, unsigned int value);
//----------------------------------------------------------------------------
// destroy semaphore object
int vssem_destroy(vssem_t * sem);
//----------------------------------------------------------------------------
// blocking decrement semaphore
int vssem_wait(vssem_t * sem);
//----------------------------------------------------------------------------
// non-blocking decrement semaphore
int vssem_trywait(vssem_t * sem);
//----------------------------------------------------------------------------
// increment semaphore
int vssem_post(vssem_t * sem);
//----------------------------------------------------------------------------
// get value of semaphore
int vssem_getvalue(vssem_t * sem, int *sval);
//----------------------------------------------------------------------------
// 'like POSIX' mutexes implementation
typedef struct {
#ifdef VSWIN32
  HANDLE win_mtx;
#else
  pthread_mutex_t pth_mtx;
#endif
} vsmutex_t;
//----------------------------------------------------------------------------
// create mutex object
int vsmutex_init(vsmutex_t *mtx);
//----------------------------------------------------------------------------
// destroy mutex object
int vsmutex_destroy(vsmutex_t *mtx);
//----------------------------------------------------------------------------
// lock mutex
int vsmutex_lock(vsmutex_t *mtx);
//----------------------------------------------------------------------------
// unlock mutex
int vsmutex_unlock(vsmutex_t *mtx);
//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif // __cplusplus
//----------------------------------------------------------------------------
#endif // VSSYNC_H
//----------------------------------------------------------------------------

/*** end of "vssync.h" file ***/


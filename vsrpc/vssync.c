/*
 * Very simple wrappers over syncronization primitives in POSIX way
 * Version: 0.9
 * File: "vssync.c"
 * (C) 2008 Anton Shmigirilov <shmigirilov@gmail.com>
 * (C) 2008-2015 Alex Grinkov <a.grinkov@gmail.com>
 */

//----------------------------------------------------------------------------
#include <stdlib.h>
#include "vssync.h"
//----------------------------------------------------------------------------
// create semaphore object
int vssem_init(vssem_t * sem, int pshared, unsigned int value)
{
#ifdef VSWIN32
  pshared = pshared; // unused

  sem->win_sem = CreateSemaphore(
                   NULL,     // default security attributes
                   value,    // initial value
                   1000000,  // maximum value
                   NULL);    // object name

  if (NULL == sem->win_sem)
    return -1;

  return 0;
#else
  return sem_init(&(sem->pth_sem), pshared, value);
#endif
}
//----------------------------------------------------------------------------
// destroy semaphore object
int vssem_destroy(vssem_t * sem)
{
#ifdef VSWIN32
  BOOL ret;

  ret = CloseHandle(sem->win_sem);

  if (!ret)
    return -1;
  else
    return 0;
#else
  return sem_destroy(&(sem->pth_sem));
#endif
}
//----------------------------------------------------------------------------
// blocking decrement semaphore
int vssem_wait(vssem_t * sem)
{
#ifdef VSWIN32
  int ret;

  // decrease count by 1
  ret = WaitForSingleObject(sem->win_sem, INFINITE);

  if (ret == WAIT_FAILED)
    return -1;

  return 0;
#else
  return sem_wait(&(sem->pth_sem));
#endif
}
//----------------------------------------------------------------------------
// non-blocking decrement semaphore
int vssem_trywait(vssem_t * sem)
{
#ifdef VSWIN32
  int ret;

  // decrease count by 1
  ret = WaitForSingleObject(sem->win_sem, 0L);

  if (ret == WAIT_TIMEOUT)
    return -3;  // EAGAIN
  else if (ret == WAIT_FAILED)
    return -1;

  return 0;
#else
  return sem_trywait(&(sem->pth_sem));
#endif
}
//----------------------------------------------------------------------------
// increment semaphore
int vssem_post(vssem_t * sem)
{
#ifdef VSWIN32
  BOOL ret;

  // increase count by 1
  ret = ReleaseSemaphore(sem->win_sem, 1, NULL);
  if (!ret)
    return -1;

  return 0;
#else
  return sem_post(&(sem->pth_sem));
#endif
}
//----------------------------------------------------------------------------
#ifdef VSWIN32
typedef struct _SEMAINFO {
  UINT    Count;    // current semaphore count
  UINT    Limit;    // max semaphore count
} SEMAINFO, *PSEMAINFO;
//----------------------------------------------------------------------------
int WINAPI NtQuerySemaphore(
  HANDLE Handle,
  int InfoClass,
  PSEMAINFO SemaInfo,
  int InfoSize,
  int *RetLen
);
#endif
//----------------------------------------------------------------------------
// get value of semaphore
int vssem_getvalue(vssem_t * sem, int *sval)
{
#ifdef VSWIN32
  int ret;
  SEMAINFO info;
  int len;

  ret = NtQuerySemaphore(
          sem->win_sem,
          0,
          &info,
          sizeof(info),
          &len);

  if ( ret < 0 )
  {
    *sval = -1;
    return ret;
  }
  else
  {
    *sval = info.Count;
    return 0;
  }
#else
  return sem_getvalue(&(sem->pth_sem), sval);
#endif
}
//----------------------------------------------------------------------------
// create mutex object
int vsmutex_init(vsmutex_t *mtx)
{
#ifdef VSWIN32
  mtx->win_mtx = CreateMutex(
                   NULL,     // default security attributes
                   FALSE,    // Initial value
                   NULL);    // object name

  if (NULL == mtx->win_mtx)
    return -1;

  return 0;
#else
  return pthread_mutex_init(&(mtx->pth_mtx), NULL);
#endif
}
//----------------------------------------------------------------------------
// destroy mutex object
int vsmutex_destroy(vsmutex_t *mtx)
{
#ifdef VSWIN32
  BOOL ret;

  ret = CloseHandle(mtx->win_mtx);

  if (!ret)
    return -1;
  else
    return 0;
#else
  return pthread_mutex_destroy(&(mtx->pth_mtx));
#endif
}
//----------------------------------------------------------------------------
// lock mutex
int vsmutex_lock(vsmutex_t *mtx)
{
#ifdef VSWIN32
  int ret;

  // wait until mutex released
  ret = WaitForSingleObject(mtx->win_mtx, INFINITE);
  if (ret == WAIT_FAILED)
    return -1;

  return 0;
#else
  return pthread_mutex_lock(&(mtx->pth_mtx));
#endif
}
//----------------------------------------------------------------------------
// unlock mutex
int vsmutex_unlock(vsmutex_t *mtx)
{
#ifdef VSWIN32
  BOOL ret;

  ret = ReleaseMutex(mtx->win_mtx);
  if (!ret)
    return -1;

  return 0;
#else
  return pthread_mutex_unlock(&(mtx->pth_mtx));
#endif
}
//----------------------------------------------------------------------------

/*** end of "vssync.c" file ***/

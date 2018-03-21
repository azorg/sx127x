/*
 * Very Simple Thread Implementation (VSRPC project)
 * Version: 0.9
 * File: "vsthread.c"
 * (C) 2007-2015 Alex Grinkov <a.grinkov@gmail.com>
 * (C) 2008 Anton Shmigirilov <shmigirilov@gmail.com>
 */

#include "vsthread.h"

#include <stdlib.h> // malloc(), free(), NULL

#ifndef VSWIN32
//----------------------------------------------------------------------------
// set POSIX thread attributes (OS depended function)
static int vsthread_set_attr(pthread_attr_t *attr, int priority, int sched)
{
#if defined(VSTHREAD_LINUX_RT) || \
    defined(VSTHREAD_OS2000) || defined(VSTHREAD_ECOS)
  struct sched_param schedparam;
#endif // VSTHREAD_LINUX_RT || VSTHREAD_OS2000 || VSTHREAD_ECOS

  if (pthread_attr_init(attr) != 0) goto err;

#ifdef VSTHREAD_LINUX
  if (pthread_attr_setschedpolicy(attr, sched) != 0) goto err;
#endif // VSTHREAD_LINUX

#ifdef VSTHREAD_LINUX_RT
  if (pthread_attr_setschedpolicy(attr, sched) != 0) goto err;
  schedparam.sched_priority = priority;
  if (pthread_attr_setschedparam(attr, &schedparam) != 0) goto err;
#endif // VSTHREAD_LINUX

#ifdef VSTHREAD_OS2000
  if (pthread_attr_setschedpolicy(attr, sched) != 0) goto err;
  schedparam.sched_priority = priority;
  if (pthread_attr_setschedparam(attr, &schedparam) != 0) goto err;
#endif // VSTHREAD_OS2000

#ifdef VSTHREAD_ECOS
  if (pthread_attr_setdetachstate(attr, PTHREAD_CREATE_DETACHED) != 0) goto err;
  if (pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED) != 0) goto err;
  if (pthread_attr_setschedpolicy(attr, sched) != 0) goto err;
  schedparam.sched_priority = priority;
  if (pthread_attr_setschedparam(attr, &schedparam) != 0) goto err;
#endif // VSTHREAD_ECOS

  return 0; // success

err:
  VSTHREAD_DBG("Ooops; can't set POSIX thread attributes");
  return -1; // error
}
#endif // VSWIN32
//----------------------------------------------------------------------------
#ifdef VSTHREAD_POOL
//----------------------------------------------------------------------------
// each thread of pool
static void *vsthread_pool_thread(void *arg)
{
  int finish;
  vsthread_record_t *p = (vsthread_record_t*) arg;
  while (1)
  {
    vssem_wait(&p->sem_start);
    if (vssem_trywait(&p->sem_kill) == 0) break;
    vssem_wait(&p->sem_free);
    p->ret = p->func(p->arg);
    vssem_post(&p->sem_free);
    vssem_getvalue(&p->sem_finish, &finish);
    if (finish == 0) vssem_post(&p->sem_finish);
  }
  return NULL;
}
//----------------------------------------------------------------------------
// create treads pool (start all threads before)
int vsthread_pool_init(vsthread_pool_t *pool, int poolsize,
                       int priority, int sched)
{
  int err, i;
  pthread_attr_t attr;
  vsthread_record_t *p;

  pool->list = (vsthread_record_t*)
    malloc(poolsize * sizeof(vsthread_record_t));
  if (pool->list == (vsthread_record_t*) NULL)
  {
    pool->size = 0;
    VSTHREAD_DBG("Ooops; maloc return NULL!");
    goto err;
  }
  pool->size = poolsize;

  // set thread attributes
  if (vsthread_set_attr(&attr,
                        pool->priority = priority,
                        pool->sched    = sched) != 0) goto err;

  if (vssem_init(&pool->sem_list, 0, 1) != 0) goto err;

  p = pool->list;
  for (i = poolsize; i > 0; i--)
  {
    if (vssem_init(&p->sem_start,  0, 0) != 0) goto err;
    if (vssem_init(&p->sem_free,   0, 1) != 0) goto err;
    if (vssem_init(&p->sem_finish, 0, 0) != 0) goto err;
    if (vssem_init(&p->sem_kill,   0, 0) != 0) goto err;

#ifdef VSTHREAD_STACKSIZE
    // set stack
#if defined(VSTHREAD_LINUX) || defined(VSTHREAD_LINUX_RT)
    pthread_attr_setstack(&attr,
                          (void*) &p->stack[0],
                          VSTHREAD_STACKSIZE);
#endif // VSTHREAD_LINUX || VSTHREAD_LINUX_RT

#ifdef VSTHREAD_ECOS
    pthread_attr_setstackaddr(&attr, (void*) &p->stack[VSTHREAD_STACKSIZE]);
    pthread_attr_setstacksize(&attr, VSTHREAD_STACKSIZE);
#endif // VSTHREAD_ECOS
#endif // VSTHREAD_STACKSIZE

    // create POSIX thread
    err = pthread_create(&p->pthread, &attr, vsthread_pool_thread, (void*) p);
    if (err != 0)
    {
      VSTHREAD_DBG("Ooops; error of pthread_create(): \"%s\"", strerror(err));
      goto err;
    }
    p++;
  } // for

  if (i != 0) goto err;

  VSTHREAD_DBG("Wow!; create pool of started thread [%i]; success...",
               poolsize);
  return 0; // success

err:
  VSTHREAD_DBG("Ooops; can't create pool of started thread");
  return -1; // error
}
//----------------------------------------------------------------------------
// destroy pool of all startted thread
void vsthread_pool_destroy(vsthread_pool_t *pool)
{
  int i;
  vsthread_record_t *p;

  vssem_wait(&pool->sem_list);

  p = pool->list;
  for (i = pool->size; i > 0; i--)
  {
    vssem_post(&p->sem_kill);
    vssem_post(&p->sem_start);
    p++;
  }

  p = pool->list;
  for (i = pool->size; i > 0; i--)
  {
#if 0 // FIXME
    pthread_join(p->pthread, NULL);
#endif
    vssem_destroy(&p->sem_start);
    vssem_destroy(&p->sem_free);
    vssem_destroy(&p->sem_finish);
    vssem_destroy(&p->sem_kill);
    p++;
  }

  vssem_destroy(&pool->sem_list);

  free((void*) pool->list);
  pool->size = 0;
}
//----------------------------------------------------------------------------
// create thread (wakeup thred in pool)
int vsthread_create(
  vsthread_pool_t *pool,
  vsthread_t *thread, void *(*func)(void *), void *arg)
{
  int i, free;
  vsthread_record_t *p;

  // find one of free pool thread
  vssem_wait(&pool->sem_list);
  p = pool->list;
  for (i = pool->size; i > 0; i--)
  {
    vssem_getvalue(&p->sem_free, &free);
    if (free != 0)
    { // thread is free
      thread->pthread = p->pthread;
      p->func = func;
      p->arg = arg;
      while (1)
      {
        vssem_getvalue(&p->sem_finish, &free);
        if (free == 0) break;
        vssem_wait(&p->sem_finish);
      }
      vssem_post(&p->sem_start); // wakeup thread
      vssem_post(&pool->sem_list);
      return 0; // success;
    }
    p++;
  }
  VSTHREAD_DBG("Ooops; can't create thread: \"all pool busy\"");
  vssem_post(&pool->sem_list);
  return -1; // try again later, all pool threads are busy
}
//----------------------------------------------------------------------------
// cancel thread in pool
int vsthread_cancel(vsthread_pool_t *pool, vsthread_t thread)
{
  int i, err;
  pthread_attr_t attr;
  vsthread_record_t *p;

  vssem_wait(&pool->sem_list);

  // find thread record in pool
  p = pool->list;
  for (i = pool->size; i > 0; i--)
  {
    if (p->pthread == thread.pthread)
    {
      // cancel POSIX thread
      if (pthread_cancel(p->pthread) != 0) goto err;

      vssem_destroy(&p->sem_start);
      vssem_destroy(&p->sem_free);
      vssem_destroy(&p->sem_finish);
      vssem_destroy(&p->sem_kill);

      // set thread attributes
      if (vsthread_set_attr(&attr,
                            pool->priority, pool->sched) != 0) goto err;

      if (vssem_init(&p->sem_start,  0, 0) != 0) goto err;
      if (vssem_init(&p->sem_free,   0, 1) != 0) goto err;
      if (vssem_init(&p->sem_finish, 0, 0) != 0) goto err;
      if (vssem_init(&p->sem_kill,   0, 0) != 0) goto err;

#ifdef VSTHREAD_STACKSIZE
      // set stack
#if defined(VSTHREAD_LINUX) || defined(VSTHREAD_LINUX_RT)
      pthread_attr_setstack(&attr,
                            (void*) &p->stack[0],
                            VSTHREAD_STACKSIZE);
#endif // VSTHREAD_LINUX || VSTHREAD_LINUX_RT

#ifdef VSTHREAD_ECOS
      pthread_attr_setstackaddr(&attr, (void*) &p->stack[VSTHREAD_STACKSIZE]);
      pthread_attr_setstacksize(&attr, VSTHREAD_STACKSIZE);
#endif // VSTHREAD_ECOS
#endif // VSTHREAD_STACKSIZE

      // create new POSIX thread
      err = pthread_create(&p->pthread, &attr, vsthread_pool_thread, (void*) p);
      if (err != 0)
      {
  VSTHREAD_DBG("Ooops; error of pthread_create(): \"%s\"", strerror(err));
  goto err;
      }
      vssem_post(&pool->sem_list);
      return 0; // success, thread restarted
    }
  }
  VSTHREAD_DBG("Ooops; cancel thread failure: \"thread not fount in pool\"");
  vssem_post(&pool->sem_list);
  return -1; // error

err:
  VSTHREAD_DBG("Ooops; cancel thread failure");
  vssem_post(&pool->sem_list);
  return -1; // error
}
//----------------------------------------------------------------------------
// join to thread in pool
int vsthread_join(
  vsthread_pool_t *pool,
  vsthread_t thread, void **thread_return)
{
  int i;
  vsthread_record_t *p;

  vssem_wait(&pool->sem_list);

  // find thread record in pool
  p = pool->list;
  for (i = pool->size; i > 0; i--)
  {
    if (p->pthread == thread.pthread)
    {
      vssem_post(&pool->sem_list);
      vssem_wait(&p->sem_finish);
      return 0; // success
    }
  }
  VSTHREAD_DBG("Ooops; join to thread failure: \"thread not fount in pool\"");
  vssem_post(&pool->sem_list);
  return -1; // error
}
//----------------------------------------------------------------------------
#else // !VSTHREAD_POOLSIZE
//----------------------------------------------------------------------------
// create POSIX thread
int vsthread_create(
  int priority, int sched,
  vsthread_t *thread, void *(*func)(void *), void *arg)
{
#ifdef VSWIN32
  BOOL ret;

  sched = sched;  // unused

  thread->handle = CreateThread(
    NULL,                          // default security descriptor
    0,                             // default stack size
    (LPTHREAD_START_ROUTINE) func, // entry point
    arg,                           // parameter
    0,                             // flags: run immediately
    &(thread->id)                  // returned ID of thread
  );

  if (thread->handle == NULL)
  {
    VSTHREAD_DBG("Ooops; error of CreateThread(): \"%d\"", (int) GetLastError());
    goto err;
  }

  ret = SetThreadPriority(thread->handle, priority);
  if (!ret)
  {
    VSTHREAD_DBG("Ooops; error of SetThreadPriority(): \"%d\"", (int) GetLastError());
    goto err;
  }

  return 0;

#else // VSWIN32
  int err;
  pthread_attr_t attr;

  // set thread attributes
  if (vsthread_set_attr(&attr, priority, sched) != 0) goto err;

  // create POSIX thread
  if ((err = pthread_create(&thread->pthread, &attr, func, arg)) != 0)
  {
    VSTHREAD_DBG("Ooops; error of pthread_create(): \"%s\"", strerror(err));
    goto err;
  }
  return 0; // success
#endif // VSWIN32

err:
  VSTHREAD_DBG("Ooops; can't create thread");
  return -1; // error
}
//----------------------------------------------------------------------------
// cancel POSIX thread
int vsthread_cancel(vsthread_t thread)
{
#ifdef VSWIN32
  return TerminateThread(thread.handle, 0);
#else // VSWIN32
  return pthread_cancel(thread.pthread);
#endif // VSWIN32
}
//----------------------------------------------------------------------------
// join to POSIX thread
int vsthread_join(
  vsthread_t thread, void **thread_return)
{
  int err;
#ifdef VSWIN32
  err = WaitForSingleObject(thread.handle, INFINITE);
  if (err == WAIT_FAILED)
  {
    VSTHREAD_DBG("Ooops; WaitForSingleObject failure: \"%d\"",
                 (int) GetLastError());
    return -1; // error
  }

  return 0;
#else
  if ((err = pthread_join(thread.pthread, thread_return)) == 0)
    return 0; // success

  VSTHREAD_DBG("Ooops; join to POSIX thread failure: \"%s\"", strerror(err));
  return -1; // error
#endif // VSWIN32
}
//----------------------------------------------------------------------------
#endif // !VSTHREAD_POOL
//----------------------------------------------------------------------------
/*** end of "vsthread.c" file ***/


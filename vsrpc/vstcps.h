/*
 * Very Simple TCP/IP Server based on threads (without VSRPC)
 * Version: 0.9
 * File: "vstcps.h"
 * (C) 2007-2015 Alex Grinkov <a.grinkov@gmail.com>
 * (C) 2008 Anton Shmigirilov <shmigirilov@gmail.com>
 */

#ifndef VSTCPS_H
#define VSTCPS_H
//----------------------------------------------------------------------------
#include "vssync.h"
#include "vsthread.h"
//----------------------------------------------------------------------------
// include debuging output
//#define VSTCPS_DEBUG
//----------------------------------------------------------------------------
#ifdef VSTCPS_DEBUG
#  include <stdio.h>  // fprintf(), vfprintf()
#  include <string.h> // strerror()
#  if defined(__GNUC__)
#    define VSTCPS_DBG(fmt, arg...) fprintf(stderr, "VSTCPS: " fmt "\n", ## arg)
#  elif defined(VSWIN32)
#    define VSTCPS_DBG(fmt, ...) fprintf(stderr, "VSTCPS: " fmt "\n", __VA_ARGS__)
#  elif defined(__BORLANDC__)
#    include <stdarg.h> // va_list, va_start(), va_end()
void VSTCPS_DBG(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "VSTCPS: ");
  vfprintf(stderr, fmt, ap);
  va_end(ap);
}
#  else
#    warning "unknown compiler"
#    define VSTCPS_DBG(fmt, arg...) fprintf(stderr, "VSTCPS: " fmt "\n", ## arg)
#  endif
#else
#  define VSTCPS_DBG(fmt, ...) // debug output off
#endif // VSTCPS_DEBUG
//----------------------------------------------------------------------------
// error codes of vstcps_start()
#define VSTCPS_ERR_NONE    0 // no error all success
#define VSTCPS_ERR_SOCKET -1 // can't create TCP server socket
#define VSTCPS_ERR_MUTEX  -2 // can't init mutex
#define VSTCPS_ERR_SEM    -3 // can't init semaphore
#define VSTCPS_ERR_POOL   -4 // can't create pool of threads
#define VSTCPS_ERR_THREAD -5 // can't create thread
//----------------------------------------------------------------------------
typedef struct vstcps_ vstcps_t;
typedef struct vstcps_client_ vstcps_client_t;
//----------------------------------------------------------------------------
// main server structure
struct vstcps_ {
  int max_clients;        // maximum clients may connected
  int sock;               // server socket ID
  int count;              // counter of connected clients
  vsthread_t thread;      // VSTCPD listen port thread
  vssem_t sem_zero;       // signal of zero clients counter
  vsmutex_t mtx_list;     // mutex for modify clients list
  vstcps_client_t *first; // structure of first connected client
  vstcps_client_t *last;  // structure of last connected client
  void *context;          // pointer to optional server context or NULL

  int (*on_accept)(       // on accept callback function
    int fd,                   // socket
    unsigned ipaddr,          // client IPv4 address
    void **client_context,    // client context
    void *server_context,     // server context
    int count);               // clients count

  void (*on_connect)(     // on connect callback function
    int fd,                   // socket
    unsigned ipaddr,          // client IPv4 address
    void *client_context,     // client context
    void *server_context);    // server context

  void (*on_disconnect)(  // on disconnect callback function
    void *client_context);    // client context

#ifdef VSTHREAD_POOL
  vsthread_pool_t pool;
#else
  int priority; int sched; // POSIX threads attributes
#endif // !VSTHREAD_POOL
};
//----------------------------------------------------------------------------
// structure for each connected client
struct vstcps_client_ {
  vstcps_t *server;      // pointer to VSTCPS (server) structure
  int fd;                // file descriptor
  void *context;         // client context
  unsigned ipaddr;       // client IPv4 address
  vsthread_t thread;     // connection thread
  vstcps_client_t *prev; // previous structure or NULL for first
  vstcps_client_t *next; // next structure or NULL for last
};
//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __plusplus
//----------------------------------------------------------------------------
// start server
// (return error code)
int vstcps_start(
  vstcps_t *server,       // pointer to VSTCPS object
  const char *host,       // server listen address
  int port,               // server listen TCP port
  int max_clients,        // max clients
  void *server_context,   // pointer to optional server context or NULL

  int (*on_accept)(       // on accept callback function
    int fd,                   // socket
    unsigned ipaddr,          // client IPv4 address
    void **client_context,    // client context
    void *server_context,     // server context
    int count),               // clients count

  void (*on_connect)(     // on connect callback function
    int fd,                   // socket
    unsigned ipaddr,          // client IPv4 address
    void *client_context,     // client context
    void *server_context),    // server context

  void (*on_disconnect)(  // on disconnect callback function
    void *client_context),    // client context

  int priority, int sched // POSIX threads attributes
);
//----------------------------------------------------------------------------
// stop server
void vstcps_stop(vstcps_t *server);
//----------------------------------------------------------------------------
// exchange for each client socket
// (return number of connected clients)
int vstcps_foreach(
  vstcps_t *server,       // pointer to VSTCPS object
  void *foreach_context,  // pointer to optional context or NULL

  void (*on_foreach)(     // on foreach callback function
    int fd,                  // socket
    unsigned ipaddr,         // client IPv4 address
    void *client_context,    // client context
    void *server_context,    // server context
    void *foreach_context,   // optional context
    int client_index,        // client index (< client_count)
    int client_count)        // client count
);
//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif // __plusplus
//----------------------------------------------------------------------------
#endif // VSTCPS_H

/*** end of "vstcps.h" file ***/


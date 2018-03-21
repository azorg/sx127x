/*
 * Very Simple TCP/IP Daemon based on VSRPC and VSTCPS
 * Version: 0.9
 * File: "vstcpd.h"
 * (C) 2007-2015 Alex Grinkov <a.grinkov@gmail.com>
 */

#ifndef VSTCPD_H
#define VSTCPD_H
//----------------------------------------------------------------------------
#include "vsrpc.h"
#include "vstcps.h"
//----------------------------------------------------------------------------
// select default timeout [ms] (-1 : infinite)
#ifndef VSTCPD_SELECT_TIMEOUT
//#  define VSTCPD_SELECT_TIMEOUT -1
#  define VSTCPD_SELECT_TIMEOUT 10000 // 10 sec
#endif
//----------------------------------------------------------------------------
// include debuging output
//#define VSTCPD_DEBUG
//----------------------------------------------------------------------------
#ifdef VSTCPD_DEBUG
#  include <stdio.h>  // fprintf(), vfprintf()
#  include <string.h> // strerror()
#  if defined(__GNUC__)
#    define VSTCPD_DBG(fmt, arg...) fprintf(stderr, "VSTCPD: " fmt "\n", ## arg)
#  elif defined(VSWIN32)
#    define VSTCPD_DBG(fmt, ...) fprintf(stderr, "VSTCPD: " fmt "\n", __VA_ARGS__)
#  elif defined(__BORLANDC__)
#    include <stdarg.h> // va_list, va_start(), va_end()
void VSTCPD_DBG(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "VSTCPD: ");
  vfprintf(stderr, fmt, ap);
  va_end(ap);
}
#  else
#    warning "unknown compiler"
#    define VSTCPD_DBG(fmt, arg...) fprintf(stderr, "VSTCPD: " fmt "\n", ## arg)
#  endif
#else
#  define VSTCPD_DBG(fmt, ...) // debug output off
#endif // VSTCPD_DEBUG
//----------------------------------------------------------------------------
typedef struct vstcpd_ vstcpd_t;
typedef struct vstcpd_client_ vstcpd_client_t;
//----------------------------------------------------------------------------
// main server structure
struct vstcpd_ {
  vstcps_t tcps;          // TCP server structure
  vsrpc_func_t *func;     // VSRPC user-defined table or NULL
  char** (*def_func)(     // pointer to default function or NULL
    vsrpc_t *rpc,             // VSRPC structure
    int argc,                 // number of arguments
    char * const argv[]);     // arguments (strings)
  int perm;               // VSRPC permissions
  void *context;          // pointer to optional server data or NULL

  int (*on_accept)(       // on accept callback function
    int fd,                   // socket
    unsigned ipaddr,          // client IPv4 address
    void **client_context,    // client context
    void *server_context,     // server context
    int count);               // clients count

  void (*on_disconnect)(  // on disconnect callback function
    void *client_context);    // client context
};
//----------------------------------------------------------------------------
// structure for each client
struct vstcpd_client_ {
  int fd;            // socket
  vsmutex_t mtx_fd;  // mutex for using fd
  unsigned ipaddr;   // client IPv4 address
  void *context;     // client context
  vstcpd_t *server;  // pointer to server structure
  vsrpc_t rpc;       // VSRPC structure
};
//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __plusplus
//----------------------------------------------------------------------------
// start server
// (return error code of vstcps_start())
int vstcpd_start(
  vstcpd_t *server,            // pointer to VSTCPD object
  vsrpc_func_t *rpc_functions, // table of VSRPC functions or NULL
  char** (*rpc_def_func)(      // pointer to default function or NULL
    vsrpc_t *rpc,                  // VSRPC structure
    int argc,                      // number of arguments
    char * const argv[]),          // arguments (strings)
  int rpc_permissions,         // VSRPC permissions
  const char* host,            // server listen address
  int port,                    // server listen TCP port
  int max_clients,             // max clients
  void *server_context,        // pointer to optional server data or NULL

  int (*on_accept)(            // on accept callback function or NULL
    int fd,                        // socket
    unsigned ipaddr,               // client IPv4 address
    void **client_context,         // client context
    void *server_context,          // server context
    int count),                    // clients count

  void (*on_disconnect)(       // terminate callback function or NULL
    void *client_context),         // client context

  int priority, int sched      // POSIX threads attributes
);
//----------------------------------------------------------------------------
// stop server
void vstcpd_stop(
  vstcpd_t *server // pointer to VSTCPD object
);
//----------------------------------------------------------------------------
// broadcast procedures call on all clients
int vstcpd_broadcast(
  vstcpd_t *server,   // pointer to VSTCPD object
  void *exclude,      // exclude this client (vstcpd_client_t*) or NULL
  char * const argv[] // function name and arguments
);
//----------------------------------------------------------------------------
// broadcast procedures call on all clients with "friendly" interface
// list: s-string, i-integer, f-float, d-double
int vstcpd_broadcast_ex(
  vstcpd_t *server, // pointer to VSTCPD object
  void *exclude,    // exclude this client (vstcpd_client_t*) or NULL
  const char *list, // list of argumets type (begin with 's' - function name)
  ...               // function name and arguments
);
//----------------------------------------------------------------------------
// exchange for each client
// (return number of connected clients)
int vstcpd_foreach(
  vstcpd_t *server,       // pointer to VSTCPS object
  void *foreach_context,  // pointer to optional context or NULL

  void (*on_foreach)(     // on foreach callback function
    vsrpc_t *rpc,            // pointer to VSRPC structure
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
#endif // VSTCPD_H

/*** end of "vstcpd.h" file ***/


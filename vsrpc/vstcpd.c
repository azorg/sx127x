/*
 * Very Simple TCP/IP Daemon based on VSRPC and VSTCPS
 * Version: 0.9
 * File: "vstcpd.c"
 * (C) 2007-2015 Alex Grinkov <a.grinkov@gmail.com>
 */

//----------------------------------------------------------------------------
#include "vstcpd.h"
#include "socklib.h"
#include <stdarg.h> // va_list, va_start(), va_end()
#include <string.h> // strlen()
#include <stdlib.h> // malloc(), free()
//----------------------------------------------------------------------------
// on accept callback function
static int vstcpd_on_accept(
  int fd,                // socket
  unsigned ipaddr,       // client IPv4 address
  void **client_context, // client context
  void *server_context,  // server context
  int count)             // clients count
{
  int retv = 0;
  vstcpd_t *ps = (vstcpd_t*) server_context;
  vstcpd_client_t *pc = (vstcpd_client_t*) malloc(sizeof(vstcpd_client_t));

  VSTCPD_DBG("vstcpd_on_accept(fd=%i, IP=%s, count=%i) start",
             fd, sl_inet_ntoa(ipaddr), count);

  if (pc == NULL)
  {
    VSTCPD_DBG("ooops; malloc() return NULL: reject connection");
    return -1;
  }

  vsmutex_init(&pc->mtx_fd);
  pc->server = ps;
  pc->context = NULL;
  *client_context = pc;

  if (ps->on_accept != NULL)
  {
    VSTCPD_DBG("ps->on_accept() start");
    retv = ps->on_accept(fd, ipaddr, &pc->context, ps->context, count);
    VSTCPD_DBG("ps->on_accept() finish and return %i", retv);
    if (retv != 0)
    { // reject
      VSTCPD_DBG("on_accept() return %i != 0, reject connection", retv);
      vsmutex_destroy(&pc->mtx_fd);
      free(pc);
    }
  }

  VSTCPD_DBG("vstcpd_on_accept() finish");
  return retv;
}
//----------------------------------------------------------------------------
// on connect callback function
static void vstcpd_on_connect(
  int fd,               // socket
  unsigned ipaddr,      // client IPv4 address
  void *client_context, // client context
  void *server_context) // server context
{
  int retv;
  vstcpd_client_t *pc = (vstcpd_client_t*) client_context;
  vstcpd_t *ps = pc->server;
  pc->ipaddr = ipaddr;

  VSTCPD_DBG("vstcpd_on_connect(fd=%i, IP=%s) start",
             fd, sl_inet_ntoa(pc->ipaddr));

  // new client connected
  // init VSRPC structure
  retv = vsrpc_init(
    &pc->rpc,              // VSRPC structure
    ps->func,              // user functions or NULL
    ps->def_func,          // default function or NULL
    ps->perm,              // permissions
    pc,                    // pointer to client struct (vstcpd_client_t)
    fd, fd,                // read and write file descriptor (socket)
    sl_read,               // read() function
    sl_write,              // write() function
    sl_select,             // select() function
    (void (*)(int)) NULL); // flush() function

  if (retv != VSRPC_ERR_NONE)
  {
    VSTCPD_DBG("ooops; can't init VSRPC: vsrpc_init() return %i)", retv);
    VSTCPD_DBG("vstcpd_on_connect() finish");
    return;
  }

  while (1)
  {
    // wait request from client side
    VSTCPD_DBG("run cycle: while ((sl_select(fd=%i, timeout=%i) == 0);",
               fd, VSTCPD_SELECT_TIMEOUT);

    while ((retv = sl_select(fd, VSTCPD_SELECT_TIMEOUT)) == 0)
    {
      VSTCPD_DBG("sl_select(fd=%i, timeout=%i) return 0 (false)",
                 fd, VSTCPD_SELECT_TIMEOUT);
    }

    VSTCPD_DBG("sl_select(fd=%i, timeout=%i) return %i: '%s'",
               fd, VSTCPD_SELECT_TIMEOUT, retv,
               retv < 0 ? sl_error_str(retv) : (retv ? "true" : "false"));

    vsmutex_lock(&pc->mtx_fd);

    if (retv < 0)
       break; // close connection

    // allow run 1 procedure on server
    retv = vsrpc_run(&pc->rpc);

    VSTCPD_DBG("vsrpc_run() return %i: '%s'", retv, vsrpc_error_str(retv));

    if (retv == VSRPC_ERR_EXIT)
    {
      if (pc->rpc.retc > 0)
        VSTCPD_DBG("client say exit(%i), disconnect",
                   vsrpc_exit_value(&pc->rpc));
      else
        VSTCPD_DBG("client say exit(), disconnect");
      break; // clode connection
    }

    // check VSRPC return value
    if (retv != VSRPC_ERR_EMPTY &&
        retv != VSRPC_ERR_NONE  &&
        retv != VSRPC_ERR_RET   &&
        retv != VSRPC_ERR_FNF) break; // close connection

    vsmutex_unlock(&pc->mtx_fd);
  } // while (1)

  vsmutex_unlock(&pc->mtx_fd);

  vsrpc_release(&pc->rpc); // stop VSRPC

  VSTCPD_DBG("vstcpd_on_connect() finish");
}
//----------------------------------------------------------------------------
// on disconnect callback function
static void vstcpd_on_disconnect(
  void *client_context) // client context
{
  vstcpd_client_t *pc = (vstcpd_client_t*) client_context;
  vstcpd_t *ps = pc->server;

  VSTCPD_DBG("vstcpd_on_disconnect() start");

  // disconnect
  if (ps->on_disconnect != NULL)
  {
    VSTCPS_DBG("pc->on_disconnect() start");
    ps->on_disconnect(pc->context);
    VSTCPS_DBG("pc->on_disconnect() finish");
  }

  vsmutex_destroy(&pc->mtx_fd);
  free(pc);

  VSTCPD_DBG("vstcpd_on_disconnect() finish");
}
//----------------------------------------------------------------------------
#ifdef VSTCPD_DEBUG
// print unknown functions if debug on
static char **vstcpd_def_func_debug(
  vsrpc_t *rpc, int argc, char * const argv[])
{
  int i;
  VSTCPD_DBG("unknown function '%s'", argv[0]);
  for (i = 1; i < argc; i++)
    VSTCPD_DBG("  argument #%i = '%s'", i, argv[i]);
  return (char**) NULL;
}
#endif
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

  int priority, int sched)     // POSIX threads attributes
{
  int retv;

#ifdef VSTCPD_DEBUG
  if (rpc_def_func == (char** (*)(vsrpc_t*, int, char *const[])) NULL)
     rpc_def_func = vstcpd_def_func_debug;
#endif

  VSTCPD_DBG("vstcpd_start(host=%s, port=%i, max_clients=%i) start",
             host, port, max_clients);

  // fill server structure
  server->func     = rpc_functions;
  server->def_func = rpc_def_func;
  server->perm     = rpc_permissions | VSRPC_PERM_EXIT;
  server->context  = server_context;
  server->on_accept     = on_accept;
  server->on_disconnect = on_disconnect;

  // start server (create VSTCPS object)
  retv = vstcps_start(
    &server->tcps,        // pointer to VSTCPS object
    host,                 // server listen address
    port,                 // server listen TCP port
    max_clients,          // max clients
    (void*) server,       // pointer to optional server context or NULL
    vstcpd_on_accept,     // on accept callback function
    vstcpd_on_connect,    // on connect callback function
    vstcpd_on_disconnect, // on disconnect callback function
    priority, sched);     // POSIX threads attributes

  if (retv != VSTCPS_ERR_NONE)
    VSTCPD_DBG("ooops; can't create server: vstcps_start() return %i", retv);

  VSTCPD_DBG("vstcpd_start() finish");

  return retv;
}
//----------------------------------------------------------------------------
// stop server
void vstcpd_stop(
  vstcpd_t *server) // pointer to VSTCPD object
{
  vstcps_stop(&server->tcps);
}
//----------------------------------------------------------------------------
typedef struct {
  int count;
  char * const * argv;
  void *exclude;
} vstcpd_broadcast_t;
//----------------------------------------------------------------------------
// on exchange callback function
static void vstcpd_on_broadcast(
  int fd,                // socket
  unsigned ipaddr,       // client IPv4 address
  void *client_context,  // client context (vstcpd_client_t*)
  void *server_context,  // server context (vstcpd_t*)
  void *foreach_context, // optional context (vstcpd_broadcast_t*)
  int client_index,      // client index (< client_count)
  int client_count)      // client count
{
  vstcpd_client_t *pc = (vstcpd_client_t*) client_context;
  vstcpd_broadcast_t *pb = (vstcpd_broadcast_t*) foreach_context;
  int err;

  if ((void*) pc == pb->exclude)
    return;

  vsmutex_lock(&pc->mtx_fd);
  err = vsrpc_call(&pc->rpc, pb->argv); // call procedure on remote machine
  vsmutex_unlock(&pc->mtx_fd);

  if (err == VSRPC_ERR_NONE) pb->count++; // count of sent messages
}
//----------------------------------------------------------------------------
// broadcast procedures call on all clients
int vstcpd_broadcast(
  vstcpd_t *server,    // pointer to VSTCPD object
  void *exclude,       // exclude this client (vstcpd_client_t*) or NULL
  char * const argv[]) // function name and arguments
{
  vstcpd_broadcast_t b;
  b.count = 0;
  b.argv = argv;
  b.exclude = exclude;
  vstcps_foreach(&server->tcps, (void*) &b, vstcpd_on_broadcast);
  return b.count;
}
//----------------------------------------------------------------------------
// broadcast procedures call on all clients with "friendly" interface
// list: s-string, i-integer, f-float, d-double
int vstcpd_broadcast_ex(
  vstcpd_t *server, // pointer to VSTCPD object
  void *exclude,    // exclude this client (vstcpd_client_t*) or NULL
  const char *list, // list of argumets type (begin with 's' - function name)
  ...)              // function name and arguments
{
  va_list ap;
  int argc, i;
  char **argv, c, *s;
  if (list == NULL) return -1; // some check
  argc = strlen(list); // very easy!
  if (argc == 0) return -1; // check argument

  // malloc argv array
  argv = (char**) vsrpc_malloc ((argc + 1) * (int)sizeof(char*));
  if (argv == NULL) return -1; // memory exeption

  va_start(ap, list);
  for (i = 0; i < argc; i++)
  {
    c = list[i];
    if      (c == 's') s = vsrpc_str2str(va_arg(ap, char*));
    else if (c == 'i') s = vsrpc_int2str(va_arg(ap, int));
#ifdef VSRPC_FLOAT
    else if (c == 'f') s = vsrpc_float2str((float)va_arg(ap, double));
    else if (c == 'd') s = vsrpc_double2str(va_arg(ap, double));
#endif // VSRPC_FLOAT
    else s = "";
    argv[i] = s;
  }
  argv[argc] = NULL; // place to end
  va_end(ap);

  // send procedure name and argument(s) to all remote machines (clients)
  i = vstcpd_broadcast(server, exclude, argv);

  // free memory from argument list
  vsrpc_free_argv(argv);
  return i;
}
//----------------------------------------------------------------------------
typedef struct {
  int count;
  void *context;
  void (*on_foreach)(     // on foreach callback function
    vsrpc_t *rpc,            // pointer to VSRPC structure
    unsigned ipaddr,         // client IPv4 address
    void *client_context,    // client context
    void *server_context,    // server context
    void *foreach_context,   // optional context
    int client_index,        // client index (< client_count)
    int client_count);       // client count
} vstcpd_foreach_t;
//----------------------------------------------------------------------------
static void vstcpd_on_foreach( // on foreach callback function
    int fd,                  // socket
    unsigned ipaddr,         // client IPv4 address
    void *client_context,    // client context
    void *server_context,    // server context
    void *foreach_context,   // optional context
    int client_index,        // client index (< client_count)
    int client_count)        // client count
{
  vstcpd_client_t *pc = (vstcpd_client_t*) client_context;
  vstcpd_t *ps = (vstcpd_t*) server_context;
  vstcpd_foreach_t *pf = (vstcpd_foreach_t*) foreach_context;

  vsmutex_lock(&pc->mtx_fd);

  pf->on_foreach(
    &pc->rpc,      // pointer to VSRPC structure
    ipaddr,        // client IPv4 address
    pc->context,   // client context
    ps->context,   // server context
    pf->context,   // optional context
    client_index,  // client index (< client_count)
    client_count); // client count

  pf->count++;

  vsmutex_unlock(&pc->mtx_fd);
}
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
    int client_count))       // client count
{
  vstcpd_foreach_t fe;
  fe.count = 0;;
  fe.on_foreach = on_foreach;
  fe.context = foreach_context;

  return vstcps_foreach(
           &server->tcps,      // pointer to VSTCPS object
           &fe,                // pointer to optional context or NULL
           vstcpd_on_foreach); // on foreach callback function
}
//----------------------------------------------------------------------------

/*** end of "vstcpd.c" file ***/


/*
 * Very Simple TCP/IP Server based on threads (without VSRPC)
 * Version: 0.9
 * File: "vstcps.c"
 * (C) 2007-2015 Alex Grinkov <a.grinkov@gmail.com>
 * (C) 2008 Anton Shmigirilov <shmigirilov@gmail.com>
 */

//----------------------------------------------------------------------------
#include "socklib.h"
#include "vstcps.h"
#include <stdarg.h>  // va_list, va_start(), va_end()
#include <stdlib.h>  // malloc(), free()
#include <string.h>  // strlen()
//----------------------------------------------------------------------------
// listen 1 client (1 thread per 1 client)
static void *vstcps_on_connect_thread(void *arg)
{
  vstcps_client_t *client = (vstcps_client_t*) arg;
  vstcps_t *server = client->server;

  VSTCPS_DBG("vstcps_on_connect_thread() start");

  if (server->on_connect != NULL)
  {
    VSTCPS_DBG("server->on_connect() start");
    server->on_connect(client->fd, client->ipaddr,
                       client->context, server->context);
    VSTCPS_DBG("server->on_connect() finish");
  }

  // disconnect
  vsmutex_lock(&server->mtx_list);
  if (--server->count == 0)
  {
    vssem_post(&server->sem_zero);
    server->first = server->last = NULL;
  }
  else
  {
    if (client->prev != NULL)
      client->prev->next = client->next;
    else
      server->first = client->next;

    if (client->next != NULL)
      client->next->prev = client->prev;
    else
      server->last = client->prev;
  }

  if (server->on_disconnect != NULL)
  {
    VSTCPS_DBG("server->on_disconnect() start");
    server->on_disconnect(client->context);
    VSTCPS_DBG("server->on_disconnect() finish");
  }

  sl_disconnect(client->fd);

  VSTCPS_DBG("client disconnected: IP=%s, count=%d",
             sl_inet_ntoa(client->ipaddr), server->count);

  free((void*) client); // free memory

  vsmutex_unlock(&server->mtx_list);

  VSTCPS_DBG("vstcps_on_connect_thread() finish");
  return NULL;
}
//----------------------------------------------------------------------------
// server listen TCP/IP port thread
static void *vstcps_listen_port_thread(void *arg)
{
  vstcps_t *server = (vstcps_t*) arg;
  vstcps_client_t *client;
  int fd, retv;
  unsigned ipaddr;

  VSTCPS_DBG("vstcps_listen_port_thread() start");

  while (1)
  {
    // wait for client connection
    fd = sl_accept(server->sock, &ipaddr);
    vsmutex_lock(&server->mtx_list);
    if (fd < 0)
    {
      VSTCPS_DBG("ooops; sl_accept(IP=%s) return %i: '%s'",
                 sl_inet_ntoa(ipaddr), fd, sl_error_str(fd));
      break;
    }
    VSTCPS_DBG("sl_accept(IP=%s) return %i", sl_inet_ntoa(ipaddr), fd);

    // check connections number
    if (server->count >= server->max_clients)
    { // too many connections - disconnect
      VSTCPS_DBG("connection rejected (count=%i >= max_clients=%i)",
                 server->count, server->max_clients);
      vsmutex_unlock(&server->mtx_list);
      sl_disconnect(fd);
      continue;
    }

    // new client connected
    // allocate memory for connected client
    client = (vstcps_client_t*) malloc(sizeof(vstcps_client_t));
    if (client == NULL)
    {
      VSTCPS_DBG("ooops; malloc() return NULL");
      sl_disconnect(fd);
      break;
    }

    // store in client structure
    client->server = server; // pointer to server structure
    client->ipaddr = ipaddr; // client IPv4 address
    client->fd = fd;         // file descriptor (socket)
    client->context = NULL;  // client context

    // add client structure to the end of list
    client->prev = server->last;
    client->next = NULL;
    if (server->count == 0)
    {
      vssem_wait(&server->sem_zero);
      server->first = client;
    }
    else
      server->last->next = client;
    server->last = client;
    server->count++;

    VSTCPS_DBG("client connected: IP=%s, count=%d",
               sl_inet_ntoa(ipaddr),
               server->count);

    if (server->on_accept != NULL)
    {
      VSTCPS_DBG("server->on_accept() start");
      retv = server->on_accept(fd, ipaddr, &client->context, server->context,
                               server->count);
      VSTCPS_DBG("server->on_accept() finish and return %i", retv);

      if (retv != 0)
      { // disconnect
        VSTCPS_DBG("connection rejected by on_accept()");

        if (--server->count == 0)
        {
          vssem_post(&server->sem_zero);
          server->first = server->last = NULL;
        }
        else
        {
          if (client->prev != NULL)
            client->prev->next = client->next;
          else
            server->first = client->next;

          if (client->next != NULL)
            client->next->prev = client->prev;
          else
            server->last = client->prev;
        }
        free((void*) client); // free memory
        vsmutex_unlock(&server->mtx_list);
        sl_disconnect(fd);
        continue;
      }
    }

    // create thread and start on_connect()
    retv = vsthread_create(
#ifdef VSTHREAD_POOL
      &server->pool,
#else
      server->priority, server->sched,
#endif // !VSTHREAD_POOL
      &client->thread, vstcps_on_connect_thread, (void*) client);
    if (retv != 0)
    {
      VSTCPS_DBG("ooops; can't create thread");
      break;
    }

    vsmutex_unlock(&server->mtx_list);
  } // while (1)

  vsmutex_unlock(&server->mtx_list);

  VSTCPS_DBG("vstcpd_listen_port_thread() finish");
  return NULL;
}
//----------------------------------------------------------------------------
// start server
// (return error code)
int vstcps_start(
  vstcps_t *server,        // pointer to VSTCPS object
  const char *host,        // server listen address
  int port,                // server listen TCP port
  int max_clients,         // max clients
  void *server_context,    // pointer to optional server context or NULL

  int (*on_accept)(        // on connect callback function
    int fd,                    // socket
    unsigned ipaddr,           // client IPv4 address
    void **client_context,     // client context
    void *server_context,      // server context
    int count),                // clients count

  void (*on_connect)(      // on connect callback function
    int fd,                    // socket
    unsigned ipaddr,           // client IPv4 address
    void *client_context,      // client context
    void *server_context),     // server context

  void (*on_disconnect)(   // on disconnect callback function
    void *client_context),     // client context

  int priority, int sched) // POSIX threads attributes
{
  int retv;

  VSTCPS_DBG("vstcps_start(host=%s, port=%i, max_clients=%i) start",
             host, port, max_clients);

  // set counter of clients to zero
  // and init server structure
  server->count = 0;
  server->first = server->last = NULL;
  server->max_clients = max_clients;
  server->context = server_context;
  server->on_accept     = on_accept;
  server->on_connect    = on_connect;
  server->on_disconnect = on_disconnect;

  // make server socket
  retv = sl_make_server_socket_ex(host, port, max_clients);
  if (retv < 0)
  {
    VSTCPS_DBG("ooops; can't create server socket %s:%i : '%s'",
               host, port, sl_error_str(retv));
    return VSTCPS_ERR_SOCKET;
  }
  server->sock = retv;

  // create mutex
  if (vsmutex_init(&server->mtx_list) < 0)
  {
    VSTCPS_DBG("ooops; can't init mutex");
    sl_disconnect(server->sock);
    return VSTCPS_ERR_MUTEX;
  }

  // create semaphore
  if (vssem_init(&server->sem_zero, 0, 1) < 0)
  {
    VSTCPS_DBG("ooops; can't init semaphore");
    vsmutex_destroy(&server->mtx_list);
    sl_disconnect(server->sock);
    return VSTCPS_ERR_SEM;
  }

#ifdef VSTHREAD_POOL
  if (vsthread_pool_init(&server->pool, max_clients + 1,
      priority, sched) != 0)
  {
    VSTCPS_DBG("ooops; can't create pool of threads");
    vssem_destroy(&server->sem_zero);
    vsmutex_destroy(&server->mtx_list);
    sl_disconnect(server->sock);
    return VSTCPS_ERR_POOL;
  }
#endif // VSTHREAD_POOL

  // create server listen port thread
  retv = vsthread_create(
#ifdef VSTHREAD_POOL
    &server->pool,
#else
    server->priority = priority, server->sched = sched,
#endif // !VSTHREAD_POOL
    &server->thread, vstcps_listen_port_thread, (void*) server);
  if (retv != 0)
  {
    VSTCPS_DBG("ooops; can't create thread");
#ifdef VSTHREAD_POOL
    vsthread_pool_destroy(&server->pool);
#endif // VSTHREAD_POOL
    vssem_destroy(&server->sem_zero);
    vsmutex_destroy(&server->mtx_list);
    sl_disconnect(server->sock);
    return VSTCPS_ERR_THREAD;
  }

  VSTCPS_DBG("vstcps_start() finish: TCP server %s:%i started",
             host, port);
  return 0; // success
}
//----------------------------------------------------------------------------
// stop server
void vstcps_stop(vstcps_t *server)
{
  vstcps_client_t *client;
  vsmutex_lock(&server->mtx_list);

  // close server socket and wait server thread
  sl_disconnect(server->sock);

  // close all clients sockets
  client = server->first;
  while (client != NULL)
  {
    sl_disconnect(client->fd);
    client = client->next;
  }

#if 0 // FIXME
  vsthread_join(
#ifdef VSTHREAD_POOL
    &server->pool,
#endif // VSTHREAD_POOL
    server->thread, NULL);
#endif

  vsmutex_unlock(&server->mtx_list);

  // wait last client thread
  vssem_wait(&server->sem_zero);

#ifdef VSTHREAD_POOL
  // destroy pool of threads
  vsthread_pool_destroy(&server->pool);
#endif // VSTHREAD_POOL

  // destroy semaphore and mutex
  vssem_destroy(&server->sem_zero);
  vsmutex_destroy(&server->mtx_list);
}
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
    int client_count))       // client count
{
  int i = 0;
  vstcps_client_t *client;
  vsmutex_lock(&server->mtx_list);
  client = server->first;
  while (client != NULL)
  {
    on_foreach(client->fd, client->ipaddr,
               client->context, server->context, foreach_context,
               i, server->count);
    i++;
    client = client->next;
  }
  vsmutex_unlock(&server->mtx_list);
  return i;
}
//----------------------------------------------------------------------------

/*** end of "vstcps.c" file ***/


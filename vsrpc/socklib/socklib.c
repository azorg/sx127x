/*
 * Simple wrappers to work with UNIX-like TCP/IP sockets
 * Version: 0.13
 * File: "socklib.c"
 * (C) 2008-2015 Alex  Grinkov     <a.grinkov@gmail.com>
 * (C) 2008-2009 Anton Shmigirilov <shmigirilov@gmail.com>
 * Last update: 2015.09.05
 */

//----------------------------------------------------------------------------
#include <string.h>  // memset(), memcpy()
#include "socklib.h"
//----------------------------------------------------------------------------
#ifdef SL_WIN32 // winsock
#  ifdef SL_USE_POLL
#    warning "use select() instead of poll() under Windows"
#    undef SL_USE_POLL
#  endif
#  include <windows.h>
#  include <winsock.h>
   typedef int socklen_t;
#else // SL_WIN32
#  include <errno.h>      // errno
#  include <sys/time.h>   // select()
#  include <sys/types.h>  // socket(), setsockopt(), bind()
#  include <sys/socket.h> // socket(), setsockopt(), bind(), listen(),
                          // inet_aton(), accept()
#  include <unistd.h>     // gethostname(), read(), write(), {select()}
#  include <netdb.h>      // gethostbyname()
#  include <arpa/inet.h>  // inet_aton()
#  include <netinet/in.h> // inet_aton()
#endif // SL_WIN32
//----------------------------------------------------------------------------
#ifdef ECOS
#  include <network.h> // select() under eCos
#endif // ECOS
//----------------------------------------------------------------------------
#ifndef SL_WIN32
#  ifdef SL_USE_POLL
#    include <sys/poll.h> // poll()
#  else
#    include <sys/select.h> // select() [POSIX way]
#  endif // SL_USE_POLL
#endif // SL_WIN32
//----------------------------------------------------------------------------
static int sl_initialized = 0;
//----------------------------------------------------------------------------
static const char *sl_errors[] = {
  "no error",
  "initialize error",
  "socket() error",
  "inet_aton() error",
  "bind() error",
  "listen() error",
  "gethostbyname() error",
  "connect() error",
  "accept() error",
  "pool() error",
  "select() error",
  "read error",
  "write error",
  "disconnect error",
  "socklib already initialized",
  "socklib not initialized",
  "timeout",
};
static const char *sl_error_unknown = "unknown error";
//----------------------------------------------------------------------------
#ifdef SL_WIN32
// emulate BSD inet_aton() for win32
static int inet_aton(const char *name, struct in_addr *addr)
{
  unsigned long a;

  a = inet_addr(name);
  addr->s_addr = a;

  return (a != (unsigned long) - 1);
}
#endif // SL_WIN32
//----------------------------------------------------------------------------
// initialize network subsystem
int sl_init(void)
{
#ifdef SL_WIN32
  WSADATA data;
  WORD ver;

  if (sl_initialized)
    return SL_ERROR_ALREADY_INIT;

  ver = 0x0202;

  if (WSAStartup(ver, &data))
    return SL_ERROR_INIT;
#endif // SL_WIN32

  sl_initialized = 1;

  return SL_SUCCESS;
}
//----------------------------------------------------------------------------
// finalize network subsystem
void sl_term(void)
{
#ifdef SL_WIN32
  WSACleanup();
  sl_initialized = 0;
#endif // SL_WIN32
}
//----------------------------------------------------------------------------
// return socklib error string
const char *sl_error_str(int err)
{
  err = -err;
  return (err >= 0 && err < SL_NUM_ERRORS) ?
         (char*) sl_errors[err] :
         (char*) sl_error_unknown;
}
//----------------------------------------------------------------------------
// get extra error code (useful for win32 WSA functions ONLY)
int sl_get_last_error()
{
  if (!sl_initialized)
    return 0;

#ifdef SL_WIN32
  return WSAGetLastError();
#else // SL_WIN32
  return errno;
#endif // SL_WIN32
}
//----------------------------------------------------------------------------
// make server TCP/IP socket
int sl_make_server_socket_ex(const char *listen_ip, int port, int backlog)
{
  int sock; // socket ID
  struct sockaddr_in saddr; // address of socket
  struct in_addr iaddr;

  if (!sl_initialized)
    return SL_ERROR_NOTINIT;

  // socket(...)
  sock = (int) socket(AF_INET, SOCK_STREAM, 0); // get socket
  if (sock < 0)
    return SL_ERROR_SOCKET;

  // bind(...)
  memset((void*) &saddr, (int) 0, (size_t) sizeof(saddr)); // clear address
  
  if (inet_aton(listen_ip, &iaddr) == 0)
    return SL_ERROR_ADDR;
  
  memcpy((void*) &saddr.sin_addr, (const void*) &iaddr, (size_t) sizeof(iaddr));
  
  saddr.sin_port = htons((unsigned short) port);
  saddr.sin_family = AF_INET;
  if (bind(sock, (struct sockaddr *) &saddr, sizeof(saddr)) != 0)
    return SL_ERROR_BIND;

  // listen(...)
  if (listen(sock, backlog) != 0)
    return SL_ERROR_LISTEN;

  return sock; // return vailid server socket for "accept(...)"
}
//----------------------------------------------------------------------------
// make server TCP/IP socket (simple)
int sl_make_server_socket(int port)
{
  return sl_make_server_socket_ex("0.0.0.0", port, 1);
}
//----------------------------------------------------------------------------
// make client TCP/IP socket
int sl_connect_to_server(const char *host, int port)
{
  int sock; // socket ID
  unsigned ip_addr;
  struct sockaddr_in saddr;
  struct hostent *hp;

  if (!sl_initialized)
    return SL_ERROR_NOTINIT;

  // socket(...)
  sock = (int) socket(AF_INET, SOCK_STREAM, 0); // get socket
  if (sock < 0)
    return SL_ERROR_SOCKET;

  // connect(...)
  memset((void*) &saddr, (int) 0, (size_t) sizeof(saddr)); // clear address
  hp = gethostbyname(host);
  if (hp == NULL)
  {
    ip_addr = inet_addr(host);

    if (ip_addr == INADDR_NONE)
      return SL_ERROR_RESOLVE;
    else
      saddr.sin_addr.s_addr = ip_addr;
  }
  else
  {
    memcpy((void*) &saddr.sin_addr,
           (const void*) hp->h_addr, (size_t) hp->h_length);
  }
  saddr.sin_port = htons((unsigned short) port);
  saddr.sin_family = AF_INET;
 
  if (connect(sock, (struct sockaddr *) &saddr, sizeof(saddr)) != 0)
    return SL_ERROR_CONNECT;

  return sock;
}
//----------------------------------------------------------------------------
// close socket
int sl_disconnect(int fd)
{
  int ret;

  if (!sl_initialized)
    return SL_ERROR_NOTINIT;

  ret = shutdown(fd, SHUT_RDWR);
  if (ret != 0)
    return SL_ERROR_DISCONNECT;

#ifdef SL_WIN32
  ret = closesocket(fd);
  if (ret != 0)
    return SL_ERROR_DISCONNECT;

#else // SL_WIN32

  ret = close(fd); // close file descriptor
  if (ret != 0)
    return SL_ERROR_DISCONNECT;
#endif  // SL_WIN32

  return SL_SUCCESS;
}
//----------------------------------------------------------------------------
// accept wrapper (return file descriptor or -1 on error)
int sl_accept(int server_socket, unsigned *ipaddr)
{
  int fd;
  struct sockaddr addr;
  socklen_t addrlen = sizeof(struct sockaddr);

  if (!sl_initialized)
    return SL_ERROR_NOTINIT;

  while (1)
  {
    fd = (int) accept(server_socket, &addr, &addrlen);
    if (fd < 0)
    {
#ifndef SL_WIN32
      if (errno == EINTR)
        continue; // interrupt by signal
#endif // SL_WIN32
      return SL_ERROR_ACCEPT; // error
    }

    if (ipaddr != (unsigned*) NULL)
      *ipaddr = (unsigned) (((struct sockaddr_in *)&addr)->sin_addr.s_addr);

    return fd; // success, client connected
  } // while(1)
}
//----------------------------------------------------------------------------
// select wraper for non block read (return 0:false, 1:true, <0:error code)
// if sigmask!=0 ignore interrupt by signals
int sl_select_ex(int fd, int msec, int sigmask)
{
#ifdef SL_USE_POLL
  // use poll()
  struct pollfd fds[1];
  int retv;

  while (1)
  {
    fds->fd      = fd;
    fds->events  = POLLIN;
    fds->revents = 0;

    retv = poll(fds, 1, msec);
    if (retv < 0)
    {
      if (errno == EINTR)
      { // interrupt by signal
        if (sigmask)
          continue;
        else
          return 0;
      }
      return SL_ERROR_POOL; // error
    }
    break;
  }

  if (retv > 0)
  {
    if (fds->revents & (POLLERR | POLLHUP | POLLNVAL))
      return SL_ERROR_POOL; // error

    if (fds->revents & (POLLIN | POLLPRI))
      return 1; // may non block read
  }

  return 0; // empty
#else // !SL_USE_POLL
  // use select()
  fd_set fds;
  struct timeval to, *pto;

  while (1)
  {
    FD_ZERO(&fds);
#ifdef SL_WIN32
    FD_SET((SOCKET) fd, &fds);
#else // SL_WIN32
    FD_SET(fd, &fds);
#endif // SL_WIN32
    if (msec >= 0)
    {
      to.tv_sec  = msec / 1000;
      to.tv_usec = (msec % 1000) * 1000;
      pto = &to;
    }
    else // if msec < 0 then wait forewer
      pto = (struct timeval *) NULL;

    if (select(fd + 1, &fds, NULL, NULL, pto) < 0)
    {
#ifndef SL_WIN32
      if (errno == EINTR)
      { // interrupt by signal
        if (sigmask)
          continue;
        else
          return 0;
      }
#endif // SL_WIN32
      return SL_ERROR_SELECT; // error
    }
    break;
  } // while(1)

  if (FD_ISSET(fd, &fds))
    return 1; // may non block read

  return 0; // empty
#endif // !SL_USE_SELECT
}
//----------------------------------------------------------------------------
// select wraper for non block read (return 0:false, 1:true, <0:error code)
// (sigmask = 0)
int sl_select(int fd, int msec)
{
  return sl_select_ex(fd, msec, 0);
}
//----------------------------------------------------------------------------
// fuse select wraper (always return 1)
int sl_select_fuse(int fd, int msec)
{
  fd = fd;
  msec = msec;
  return 1;
}
//----------------------------------------------------------------------------
// read wraper
int sl_read(int fd, void *buf, int size)
{
  if (!sl_initialized)
    return SL_ERROR_NOTINIT;

  if (size > 0)
    while (1)
    {
#ifdef SL_WIN32
      size = recv(fd, (char*) buf, size, 0);
#else // SL_WIN32
      size = read(fd, buf, (size_t) size);
#endif  // SL_WIN32
      if (size < 0)
      {
#ifndef SL_WIN32
        if (errno == EINTR)
          continue; // interrupt by signal
#endif // SL_WIN32
        return SL_ERROR_READ;
      }
      return size;
    } // while (1)
  return 0;
}
//----------------------------------------------------------------------------
// read `size` bytes from stream `fd` to `buf` at once
int sl_read_all(int fd, void *buf, int size)
{
  int retv, cnt = 0;
  char *ptr = (char*) buf;

  if (!sl_initialized)
    return SL_ERROR_NOTINIT;

  while (size > 0)
  {
#ifdef SL_WIN32
    retv = recv(fd, ptr, size, 0);
#else // SL_WIN32
    retv = read(fd, (void*) ptr, (size_t) size);
#endif // SL_WIN32
    if (retv < 0)
    {
#ifndef SL_WIN32
      if (errno == EINTR)
        continue; // interrupt by signal
#endif // SL_WIN32
      return SL_ERROR_READ;
    }
    else if (retv == 0)
      return cnt;
    ptr += retv;
    cnt += retv;
    size -= retv;
  }
  return cnt;
}
//----------------------------------------------------------------------------
// read `size` bytes from stream `fd` to `buf` at once with use timeout
int sl_read_all_to(int fd, void *buf, int size, int ms)
{
  int retv, cnt = 0;
  char *ptr = (char*) buf;

  if (!sl_initialized)
    return SL_ERROR_NOTINIT;

  while (size > 0)
  {
    retv = sl_select(fd, ms);
    if (retv < 0)
    {
#ifndef SL_WIN32
      if (errno == EINTR)
        continue; // interrupt by signal
#endif // SL_WIN32
      return SL_ERROR_SELECT;
    }
    if (retv == 0)
      return SL_TIMEOUT;
#ifdef SL_WIN32
    retv = recv(fd, ptr, size, 0);
#else // SL_WIN32
    retv = read(fd, (void*) ptr, (size_t) size);
#endif // SL_WIN32
    if (retv < 0)
    {
#ifndef SL_WIN32
      if (errno == EINTR)
        continue; // interrupt by signal
#endif // SL_WIN32
      return SL_ERROR_READ;
    }
    else if (retv == 0)
      return cnt;
    ptr += retv;
    cnt += retv;
    size -= retv;
  }
  return cnt;
}
//----------------------------------------------------------------------------
// write `size` bytes to stream `fd` from `buf` at once
int sl_write(int fd, const void *buf, int size)
{
  int retv, cnt = 0;
  char *ptr = (char*) buf;

  if (!sl_initialized)
    return SL_ERROR_NOTINIT;

  while (size > 0)
  {
#ifdef SL_WIN32
    retv = send(fd, (const char*) ptr, size, 0);
#else // SL_WIN32
    retv = write(fd, (void*) ptr, size);
#endif // SL_WIN32
    if (retv < 0)
    {
#ifndef SL_WIN32
      if (errno == EINTR)
        continue;
      else
#endif // SL_WIN32
        return SL_ERROR_WRITE;
    }
    ptr += retv;
    cnt += retv;
    size -= retv;
  }
  return cnt;
}
//----------------------------------------------------------------------------
// make server UDP socket
int sl_udp_make_server_socket_ex(const char *listen_ip, int port)
{
  int sock;
  struct in_addr iaddr;
  struct sockaddr_in saddr;

  if (!sl_initialized)
    return SL_ERROR_NOTINIT;

  // socket(...)
  sock = (int) socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock < 0)
    return SL_ERROR_SOCKET;

  // bind(...)
  memset(&saddr, 0, sizeof(saddr));
  
  if (inet_aton(listen_ip, &iaddr) == 0)
    return SL_ERROR_ADDR;
  
  memcpy((void*) &saddr.sin_addr, (const void*) &iaddr, (size_t) sizeof(iaddr));
  //saddr.sin_addr.s_addr = htonl(INADDR_ANY);
  
  saddr.sin_port = htons((unsigned short) port);
  saddr.sin_family = AF_INET;

  if (bind(sock, (struct sockaddr*) &saddr, sizeof(saddr)) < 0)
    return SL_ERROR_BIND;

  return sock;
}
//----------------------------------------------------------------------------
// make server UDP socket (simple)
int sl_udp_make_server_socket(int port)
{
  return sl_udp_make_server_socket_ex("0.0.0.0", port);
}
//----------------------------------------------------------------------------
// make client UDP socket
int sl_udp_make_client_socket()
{
  int ret;

  if (!sl_initialized)
    return SL_ERROR_NOTINIT;

  // socket(...)
  ret = (int) socket(AF_INET, SOCK_DGRAM, 0);
  if (ret < 0)
    return SL_ERROR_SOCKET;
  else
    return ret;
}
//----------------------------------------------------------------------------
// read datagram from UDP socket (blocked)
int sl_udp_read(int sock, void *buf, int size, unsigned *ipaddr, int *port)
{
  int ret, len;
  struct sockaddr_in client;

  if (!sl_initialized)
    return SL_ERROR_NOTINIT;

  len = sizeof(client);

  while (1)
  {
#ifdef SL_WIN32
    ret = recvfrom(sock, (char*) buf, size, 0,
                   (struct sockaddr*) &client, (socklen_t*) &len);
#else // SL_WIN32
    ret = recvfrom(sock, buf, size, 0,
                   (struct sockaddr*) &client, (socklen_t*) &len);
#endif // SL_WIN32

    if (ret < 0)
    {
#ifndef SL_WIN32
      if (errno == EINTR)
        continue; // interrupt by signal
#endif // SL_WIN32

      if (ipaddr != (unsigned*) NULL)
        *ipaddr = (unsigned) - 1;
 
      if (port != (int*) NULL)
        *port = -1;
 
      return SL_ERROR_READ;
    }
  
    if (ipaddr != (unsigned*) NULL)
      *ipaddr = (unsigned) (((struct sockaddr_in*) &client)->sin_addr.s_addr);
 
    if (port != (int*) NULL)
      *port = (int) ntohs(((struct sockaddr_in*) &client)->sin_port);
  
    return ret;
  } // while (1)
}
//----------------------------------------------------------------------------
// read datagram from UDP socket (timeout)
int sl_udp_read_to(int sock, void *buf, int size,
                   unsigned *ipaddr, int *port, int ms)
{
  int ret, len;
  struct sockaddr_in client;

  if (!sl_initialized)
    return SL_ERROR_NOTINIT;

  ret = sl_select(sock, ms);
  if (ret < 0)
    return SL_ERROR_SELECT;
  else if (ret == 0)
    return SL_TIMEOUT;

  len = sizeof(client);

  while (1)
  {
#ifdef SL_WIN32
    ret = recvfrom(sock, (char*) buf, size, 0,
                   (struct sockaddr*) &client, (socklen_t*) &len);
#else // SL_WIN32
    ret = recvfrom(sock, buf, size, 0,
                   (struct sockaddr*) &client, (socklen_t*) &len);
#endif // SL_WIN32

    if (ret < 0)
    {
#ifndef SL_WIN32
      if (errno == EINTR)
        continue; // interrupt by signal
#endif // SL_WIN32

      if (ipaddr != (unsigned*) NULL)
        *ipaddr = (unsigned) - 1;
    
      if (port != (int*) NULL)
        *port = -1;

      return SL_ERROR_READ;
    }  

    if (ipaddr != (unsigned*) NULL)
      *ipaddr = (unsigned) (((struct sockaddr_in*) &client)->sin_addr.s_addr);

    if (port != (int*) NULL)
      *port = (int) ntohs(((struct sockaddr_in*) &client)->sin_port);
 
    return ret;
  } // while (1)
}
//----------------------------------------------------------------------------
// send datagram to peer via UDP to ip
int sl_udp_sendto(int sock, unsigned ipaddr, int port,
                  const void *buf, int size, int connect_flag)
{
  struct sockaddr_in to_addr;
  int ret;

  if (!sl_initialized)
    return SL_ERROR_NOTINIT;

  memset(&to_addr, 0, sizeof(to_addr));

  to_addr.sin_family = AF_INET;
  to_addr.sin_port = htons((unsigned short) port);
  to_addr.sin_addr.s_addr = ipaddr;

#ifdef SL_WIN32
  ret = sendto(sock, (const char*) buf, size, 0,
               (struct sockaddr*) &to_addr, sizeof(to_addr));
#else // SL_WIN32
  ret = sendto(sock, buf, size, 0,
               (struct sockaddr*) &to_addr, sizeof(to_addr));
#endif // SL_WIN32

  if (ret < 0)
    return SL_ERROR_WRITE;

  if (connect_flag)
    if (connect(sock, (struct sockaddr *) &to_addr, sizeof(to_addr)) != 0)
      return SL_ERROR_CONNECT;

  return ret;
}
//----------------------------------------------------------------------------
// send datagram to peer via UDP to host
int sl_udp_sendto_addr(int sock, const char *host, int port,
                       const void *buf, int size, int connect_flag)
{
  int ret;
  struct sockaddr_in to_addr;
  unsigned ip_addr;

  if (!sl_initialized)
    return SL_ERROR_NOTINIT;

  ip_addr = inet_addr(host);
  if (ip_addr == INADDR_NONE)
    return SL_ERROR_RESOLVE;  

  memset(&to_addr, 0, sizeof(to_addr));

  to_addr.sin_family = AF_INET;
  to_addr.sin_port = htons((unsigned short)port);
  to_addr.sin_addr.s_addr = inet_addr(host);

#ifdef SL_WIN32
  ret = sendto(sock, (const char*) buf, size, 0,
               (struct sockaddr*) &to_addr, sizeof(to_addr));
#else // SL_WIN32
  ret = sendto(sock, buf, size, 0,
               (struct sockaddr*) &to_addr, sizeof(to_addr));
#endif // SL_WIN32

  if (ret < 0)
    return SL_ERROR_WRITE;

  if (connect_flag)
    if (connect(sock, (struct sockaddr *) &to_addr, sizeof(to_addr)) != 0)
      return SL_ERROR_CONNECT;

  return ret;
}
//----------------------------------------------------------------------------
// convert dotted ipaddr to numeric
unsigned sl_inet_aton(const char *s)
{
  struct in_addr iaddr;
  memset((void*) &iaddr, 0, sizeof(iaddr));
  inet_aton(s, &iaddr);
  return (unsigned) iaddr.s_addr;
}
//----------------------------------------------------------------------------
// convert numeric ipaddr to dotted
const char *sl_inet_ntoa(unsigned ipaddr)
{
  struct in_addr iaddr;
  iaddr.s_addr = ipaddr;
  return (const char*) inet_ntoa(iaddr);
}
//----------------------------------------------------------------------------
// get host by name
int sl_gethostbyname(const char *host, unsigned *ip_addr)
{
  struct hostent *hp;
  struct in_addr iaddr;

  memset((void*) &iaddr, 0, sizeof(iaddr)); // clear address

  hp = gethostbyname(host);
  if (hp == NULL)
  {
    *ip_addr = (unsigned) inet_addr(host);

    if (*ip_addr == INADDR_NONE)
      return SL_ERROR_RESOLVE;
    else
      return SL_SUCCESS;
  }
  else
  {
    memcpy((void*) &iaddr,
           (const void*) hp->h_addr, (size_t) hp->h_length);

    *ip_addr = (unsigned) iaddr.s_addr;
    return SL_SUCCESS;
  }
}
//----------------------------------------------------------------------------
unsigned sl_htonl(unsigned hostlong)
{
  return htonl(hostlong);
}
//----------------------------------------------------------------------------
unsigned short sl_htons(unsigned short hostshort)
{
  return htons(hostshort);
}
//----------------------------------------------------------------------------
unsigned sl_ntohl(unsigned netlong)
{
  return htonl(netlong);
}
//----------------------------------------------------------------------------
unsigned short sl_ntohs(unsigned short netshort)
{
  return htons(netshort);
}
//----------------------------------------------------------------------------
/*** end of "socklib.c" file ***/


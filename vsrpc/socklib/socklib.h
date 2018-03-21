/*
 * Simple wrappers to work with UNIX-like TCP/IP sockets
 * Version: 0.13
 * File: "socklib.h"
 * (C) 2008-2015 Alex  Grinkov     <a.grinkov@gmail.com>
 * (C) 2008-2009 Anton Shmigirilov <shmigirilov@gmail.com>
 * Last update: 2015.09.05
 */

#ifndef SOCKLIB_H
#define SOCKLIB_H
//----------------------------------------------------------------------------
#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))
#  ifndef SL_WIN32
#    define SL_WIN32
#  endif
#endif
//----------------------------------------------------------------------------
// use poll() nor select() in sl_select()
//#define SL_USE_POLL
//----------------------------------------------------------------------------
#ifdef SL_WIN32 // winsocks
#  include <windows.h>
#  include <winsock.h>
#  include <winsock2.h> //
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
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
//----------------------------------------------------------------------------
// missing const
#ifdef SL_WIN32
#  define SHUT_RDWR 2
#endif // SL_WIN32
//----------------------------------------------------------------------------
// error codes
#define SL_SUCCESS               0
#define SL_ERROR_INIT           -1
#define SL_ERROR_SOCKET         -2
#define SL_ERROR_ADDR           -3
#define SL_ERROR_BIND           -4
#define SL_ERROR_LISTEN         -5
#define SL_ERROR_RESOLVE        -6
#define SL_ERROR_CONNECT        -7
#define SL_ERROR_ACCEPT         -8
#define SL_ERROR_POOL           -9
#define SL_ERROR_SELECT         -10
#define SL_ERROR_READ           -11
#define SL_ERROR_WRITE          -12
#define SL_ERROR_DISCONNECT     -13
#define SL_ERROR_ALREADY_INIT   -14
#define SL_ERROR_NOTINIT        -15
#define SL_TIMEOUT              -16

#define SL_NUM_ERRORS           17
//----------------------------------------------------------------------------
// initialize network subsystem
int sl_init(void);
//----------------------------------------------------------------------------
// finalize network subsystem
void sl_term(void);
//----------------------------------------------------------------------------
// return socklib error string
const char *sl_error_str(int err);
//----------------------------------------------------------------------------
// get extra error code (useful for win32 WSA functions ONLY)
int sl_get_last_error();
//----------------------------------------------------------------------------
// make server TCP socket
int sl_make_server_socket_ex(const char *listen_ip, int port, int backlog);
//----------------------------------------------------------------------------
// make server TCP socket (simple)
int sl_make_server_socket(int port); // listen_ip="0.0.0.0" backlog=1
//----------------------------------------------------------------------------
// make client TCP socket
int sl_connect_to_server(const char *host, int port);
//----------------------------------------------------------------------------
// close socket
int sl_disconnect(int fd);
//----------------------------------------------------------------------------
// accept wrapper (return file descriptor or -1 on error)
int sl_accept(int server_socket, unsigned *ipaddr);
//----------------------------------------------------------------------------
// select wraper for non block read (return 0:false, 1:true, <0:error code)
// if sigmask!=0 ignore interrupt by signals
int sl_select_ex(int fd, int msec, int sigmask);
//----------------------------------------------------------------------------
// select wraper for non block read (return 0:false, 1:true, <0:error code)
// (sigmask = 0)
int sl_select(int fd, int msec);
//----------------------------------------------------------------------------
// fuse select wraper (always return 1)
int sl_select_fuse(int fd, int msec);
//----------------------------------------------------------------------------
// read wraper
int sl_read(int fd, void *buf, int size);
//----------------------------------------------------------------------------
// read `size` bytes from stream `fd` to `buf` at once
int sl_read_all(int fd, void *buf, int size);
//----------------------------------------------------------------------------
// read `size` bytes from stream `fd` to `buf` at once with use timeout
int sl_read_all_to(int fd, void *buf, int size, int ms);
//----------------------------------------------------------------------------
// write `size` bytes to stream `fd` from `buf` at once
int sl_write(int fd, const void *buf, int size);
//----------------------------------------------------------------------------
// make server UDP socket
int sl_udp_make_server_socket_ex(const char *listen_ip, int port);
//----------------------------------------------------------------------------
// make server UDP socket (simple)
int sl_udp_make_server_socket(int port); // listen_ip="0.0.0.0"
//----------------------------------------------------------------------------
// make client UDP socket
int sl_udp_make_client_socket();
//----------------------------------------------------------------------------
// read datagram from UDP socket (blocked)
int sl_udp_read(int sock, void *buf, int size, unsigned *ipaddr, int *port);
//----------------------------------------------------------------------------
// read datagram from UDP socket (timeout)
int sl_udp_read_to(int sock, void *buf, int size,
                   unsigned *ipaddr, int *port, int ms);
//----------------------------------------------------------------------------
// send datagram to peer via UDP to ip numeric
int sl_udp_sendto(int sock, unsigned ipaddr, int port,
                  const void *buf, int size, int connect_flag);
//----------------------------------------------------------------------------
// send datagram to peer via UDP to ip char*
int sl_udp_sendto_addr(int sock, const char *host, int port,
                       const void *buf, int size, int connect_flag);
//----------------------------------------------------------------------------
// convert dotted ipaddr to numeric
unsigned sl_inet_aton(const char *s);
//----------------------------------------------------------------------------
// convert numeric ipaddr to dotted
const char *sl_inet_ntoa(unsigned ipaddr);
//----------------------------------------------------------------------------
// get host by name
int sl_gethostbyname(const char *host, unsigned *ip_addr);
//----------------------------------------------------------------------------
// ntohl etc wrappers
unsigned sl_htonl(unsigned hostlong);
unsigned short sl_htons(unsigned short hostshort);
unsigned sl_ntohl(unsigned netlong);
unsigned short sl_ntohs(unsigned short netshort);
//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif // __cplusplus
//----------------------------------------------------------------------------
#endif // SOCKLIB_H

/*** end of "socklib.h" file ***/


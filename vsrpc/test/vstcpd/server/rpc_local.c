/*
 * File: "rpc_local.c".
 * This file is autogenerated from "rpc.vsidl" file
 * and can be append. You may (and must) edit this file.
 * Recomented backup copy this file to another too.
 */
//----------------------------------------------------------------------------
#include "vsrpc.h"
#include "vstcpd.h"
//----------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
//----------------------------------------------------------------------------
int rpc_atoi(vsrpc_t* rpc, char* str)
{
  int i = atoi(str);
  printf("print: atoi('%s')=%i\n", str, i);
  return i;
}
//----------------------------------------------------------------------------
static int buf[1024 * 1024 * 4]; // FIXME
void rpc_test(vsrpc_t *rpc, int size)
{
  int i;
  
  // read from pipe
  i = vsrpc_read(rpc, (char*) buf, size * sizeof(int));
  if (i != VSRPC_ERR_NONE)
    printf("\n>>> vsrpc_read() error %i: '%s'\n", i, vsrpc_error_str(i));
  
  // modify data
  for (i = 0; i < size; i++)
    buf[i] *= (i + 65536);
  
  // return data to pipe
  i = vsrpc_write(rpc, (const char*) buf, size * sizeof(int));
  if (i != VSRPC_ERR_NONE)
    printf("\n>>> vsrpc_write() error %i: '%s'\n", i, vsrpc_error_str(i));
}
//----------------------------------------------------------------------------
void rpc_cap(vsrpc_t* rpc)
{
  int chr = 0, retv;

  printf("press any keys and 'q' to exit\n");
  
  while (1)
  {
    retv = read(rpc->fd_rd, (void*) &chr, 1);
    if (retv != 1)
    {
      fprintf(stderr, "error in vstcpd_capture(): read(1) return %i\n", retv);
      break;
    }

    printf("chr: 0x%02X -> ", chr);
    if (chr >= 32 && chr <= 127 && chr != 'q')
      printf("'%c'\n", chr);
    else if (chr == 'q')
    {
      printf("'%c' => break\n", chr);
      break;
    }
    else if (chr == '\n')
      printf("'\\n'\n");
    else if (chr == '\r')
      printf("'\\r'\n");
    else
      printf("?\n");
  }
}
//----------------------------------------------------------------------------
void rpc_loop(vsrpc_t* rpc)
{
  // loop forever
  while (1);
}
//----------------------------------------------------------------------------
void rpc_toall(vsrpc_t* rpc, char* str)
{ // send message to all another clients
  vstcpd_client_t *pc = (vstcpd_client_t*) rpc->context;
  vstcpd_t *ps = pc->server;

  vstcpd_broadcast_ex(
    ps,            // pointer to VSTCPD object
    pc,            // exclude this client (vstcpd_client_t*) or NULL
    "ss",          // list of argumets type (begin with 's' - function name)
    "msg", "hi!"); // function name and arguments
}
//----------------------------------------------------------------------------
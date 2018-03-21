/*
 * Unit test of `vstcpc` component
 * File: "test_vstcpc.c"
 */

//----------------------------------------------------------------------------
#include <stdio.h>
//#include <time.h>
#include "socklib.h"    // sl_init()
#include "vsgettime.h"  // vsgettime()
#include "vstcpc.h"     // `vstcpc_t`
#include "rpc_remote.h"
#include "rpc_client.h"
#include "lrpc_server.h"
//----------------------------------------------------------------------------
#define HOST "127.0.0.1"
//#define HOST "192.168.7.177"
#define PORT 7777
//----------------------------------------------------------------------------
char **def_fn(vsrpc_t *rpc, int argc, char * const argv[])
{
  int i;
  printf("def_fn: %s", argv[0]);
  for (i = 1; i < argc; i++)
    printf(" %s", argv[i]);
  printf("\n");
  return NULL;
}
//----------------------------------------------------------------------------
char buf[1024 * 1024 * 16];
int buf2[1024 * 1024 * 4];
//----------------------------------------------------------------------------
int main()
{
  vstcpc_t obj;
  double t1, t2;
  int i, j, k, id, size, err;

#ifdef VSWIN32
  int priority = 16;
#else
  int priority = 10;
#endif

  sl_init();

  printf("\n\npress ENTER to start\n");
  fgetc(stdin);

  printf("vstcpc_start()\n");
  if ((i = vstcpc_start(&obj, lrpc_vsrpc_func, def_fn,
                        VSRPC_PERM_DEFAULT, HOST, PORT,
                        priority, SCHED_FIFO)) != 0)
  {
    fprintf(stderr, "vstcpc_start() return %i\n", i);
    return -1;
  }

  printf("press ENTER to ping remote host (3 times)\n");
  fgetc(stdin);

  // ping #1
  vsmutex_lock(&obj.mtx_rpc);
  t1 = vsgettime();
  err = vsrpc_remote_ping(&obj.rpc, &i);
  t2 = vsgettime();
  printf("dt = %f (vsrpc_remote_ping)\n", t2 - t1);
  vsmutex_unlock(&obj.mtx_rpc);
  if (err != VSRPC_ERR_NONE)
  {
    fprintf(stderr, "vsrpc_remote_ping() error %i\n", err);
    exit(1); // FIXME
  }
  // ping #2
  vsmutex_lock(&obj.mtx_rpc);
  t1 = vsgettime();
  err = vsrpc_remote_ping(&obj.rpc, &i);
  t2 = vsgettime();
  printf("dt = %f (vsrpc_remote_ping)\n", t2 - t1);
  vsmutex_unlock(&obj.mtx_rpc);
  if (err != VSRPC_ERR_NONE)
    fprintf(stderr, "vsrpc_remote_ping() error %i\n", err);

  // ping #3
  vsmutex_lock(&obj.mtx_rpc);
  t1 = vsgettime();
  err = vsrpc_remote_ping(&obj.rpc, &i);
  t2 = vsgettime();
  printf("dt = %f (vsrpc_remote_ping)\n", t2 - t1);
  vsmutex_unlock(&obj.mtx_rpc);
  if (err != VSRPC_ERR_NONE)
    fprintf(stderr, "vsrpc_remote_ping() error %i\n", err);

  printf("\npress ENTER to run remote procedure (3 times)\n");
  fgetc(stdin);

  // atoi #1
  vsmutex_lock(&obj.mtx_rpc);
  t1 = vsgettime();
  i  = rpc_atoi_remote(&obj.rpc, "Hello!");
  t2 = vsgettime();
  printf("dt = %f (rpc_atoi)\n", t2 - t1);
  vsmutex_unlock(&obj.mtx_rpc);
  printf(">>> rpc_atoi() return %i\n", i);

  // atoi #2
  vsmutex_lock(&obj.mtx_rpc);
  t1 = vsgettime();
  i  = rpc_atoi_remote(&obj.rpc, "12345");
  t2 = vsgettime();
  printf("dt = %f (rpc_atoi)\n", t2 - t1);
  vsmutex_unlock(&obj.mtx_rpc);
  printf(">>> rpc_atoi() return %i\n", i);

  // atoi #3
  vsmutex_lock(&obj.mtx_rpc);
  t1 = vsgettime();
  i  = rpc_atoi_remote(&obj.rpc, "-12345");
  t2 = vsgettime();
  printf("dt = %f (rpc_atoi)\n", t2 - t1);
  vsmutex_unlock(&obj.mtx_rpc);
  printf(">>> rpc_atoi() return %i\n", i);

  printf("\npress ENTER to run write/read test\n");
  fgetc(stdin);

  for (k = 0; k < 3; k++)
  {
    size = 200; //1024 * 1024;
    for (i = 0; i < size; i++)
      buf2[i] = i * i;

    vsmutex_lock(&obj.mtx_rpc);
    t1 = vsgettime();
    err = rpc_test_call(&obj.rpc, size);
    if (err != VSRPC_ERR_NONE)
      fprintf(stderr, "rpc_test_call() error %i\n", err);

    err = vsrpc_write(&obj.rpc, (const char*) buf2, size * sizeof(int));
    if (err != VSRPC_ERR_NONE)
      fprintf(stderr, "vsrpc_write() error %i\n", err);

    err = vsrpc_read(&obj.rpc, (char*) buf2, size * sizeof(int));
    if (err != VSRPC_ERR_NONE)
      fprintf(stderr, "vsrpc_read() error %i\n", err);

    err = rpc_test_wait(&obj.rpc);
    if (err != VSRPC_ERR_NONE)
      fprintf(stderr, "rpc_test_wait() error %i\n", err);
    t2 = vsgettime();
    vsmutex_unlock(&obj.mtx_rpc);

    j = 0;
    for (i = 0; i < size; i++)
      if (buf2[i] != i * i * (65536 + i))
        j++;

    if (j)
      printf("!!! errors = %i\n", j);

    printf("dt = %f (write/read %i bytes)\n",
            t2 - t1, (int) (size * sizeof(int)));
  } // for

  size = 10 * 1024 * 1024;
  printf("\npress ENTER to write to server %i bytes\n", size);
  fgetc(stdin);

  for (i = 0; i < size; i++)
    buf[i] = (i & 0xFF) ^ 0xAC;
  vsmutex_lock(&obj.mtx_rpc);
  t1 = vsgettime();
  err = vsrpc_remote_malloc(&obj.rpc, size, &id); // malloc on remote machine
  t2 = vsgettime();
  if (err != VSRPC_ERR_NONE)
    fprintf(stderr, "vsrpc_remote_malloc() error %i\n", err);
  if (err == -VSRPC_ERR_NONE)
    printf("dt = %f (vsrpc_remote_malloc)\n", t2 - t1);
  t1 = vsgettime();
  err = vsrpc_remote_write(&obj.rpc,
                           (void*) buf, // ptr - source address
                           id, 0, size, 0,
                           &i);
  t2 = vsgettime();
  vsmutex_unlock(&obj.mtx_rpc);
  if (err != VSRPC_ERR_NONE)
    fprintf(stderr, "vsrpc_remote_write() error %i\n", err);
  if (err == -VSRPC_ERR_NONE && i == size)
    printf("dt = %f (vsrpc_remote_write)\n", t2 - t1);

  printf("\npress ENTER to read from server %i bytes\n", size);
  fgetc(stdin);

  vsmutex_lock(&obj.mtx_rpc);
  t1 = vsgettime();
  err = vsrpc_remote_read(&obj.rpc,
                          (void*) buf, // ptr - source address
                          id, 0, size, 0,
                          &i);
  t2 = vsgettime();
  if (err != VSRPC_ERR_NONE)
    fprintf(stderr, "vsrpc_remote_read() error %i\n", err);
  vsmutex_unlock(&obj.mtx_rpc);
  if (j == -VSRPC_ERR_NONE && i == size)
    printf("dt = %f (vsrpc_remote_read)\n", t2 - t1);


  // check read data
  for (i = 0; i < size; i++)
  {
    char c = (i & 0xFF) ^ 0xAC;
    if (buf[i] != c)
      fprintf(stderr, "bad data; error! (%i != %i)\n", buf[i], c);
  }

  printf("\npress ENTER to stop\n");
  fgetc(stdin);

  printf("vstcpc_stop()\n");
  vstcpc_stop(&obj);

  printf("\npress ENTER to exit\n");
  fgetc(stdin);

  return 0;
}
//----------------------------------------------------------------------------

/*** end of "test_vstcpc.c" file ***/


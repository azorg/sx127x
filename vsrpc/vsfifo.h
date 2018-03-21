/*
 * Very Simple FIFO (Very Simple Remote Procedure Call (VSRPC) project)
 * Version: 0.9
 * File: "vsfifo.h"
 * (C) 2007-2015 Alex Grinkov <a.grinkov@gmail.com>
 */

#ifndef VSFIFO_H
#define VSFIFO_H
//----------------------------------------------------------------------------
#include "vssync.h" // vsset_t
//----------------------------------------------------------------------------
// add receive/send functions from/to pipe
#define VSFIFO_PIPE

// add functions to work with UNIX file descriptors
#define VSFIFO_UNIX

typedef struct vsfifo_ vsfifo_t;
struct vsfifo_ {
  char *data;       // pointer to FIFO buffer
  char *in;         // pointer for next input data
  char *out;        // pointer to next output data
  int count;        // data counter
  int size;         // FIFO size
  vssem_t read_sem; // block read wait semaphore
};
//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __plusplus
//----------------------------------------------------------------------------
// create new FIFO (constructor)
// on success return 0, else -1
int vsfifo_init(vsfifo_t *fifo, int size);

// print fifo status
void vsfifo_show(vsfifo_t *fifo);

// desrtroy FIFO (destructor)
void vsfifo_release(vsfifo_t *fifo);

// clear FIFO
void vsfifo_clear(vsfifo_t *fifo);

// return FIFO counter
int vsfifo_count(vsfifo_t *fifo);

// return FIFO free bytes
int vsfifo_free(vsfifo_t *fifo);

// write to FIFO from memory buffer
int vsfifo_write(vsfifo_t *fifo, const void *buf, int count);

// read from FIFO to memory buffer non-block
int vsfifo_read_nb(vsfifo_t *fifo, void *buf, int count);

// read (and not eject!) from FIFO to memory buffer non-block
int vsfifo_read_back(vsfifo_t *fifo, void *buf, int count);

// read from FIFO to memory buffer
int vsfifo_read(vsfifo_t *fifo, void *buf, int count);

// read from FIFO nowhere (to /dev/null)
int vsfifo_to_nowhere(vsfifo_t *fifo, int count);

#ifdef VSFIFO_PIPE
// write to FIFO from pipe
int vsfifo_from_pipe(
  vsfifo_t *fifo,
  int (*read_fn)(int fd, void *buf, int size), int fd,
  int count);

// read from FIFO to pipe
int vsfifo_to_pipe(
  vsfifo_t *fifo,
  int (*write_fn)(int fd, const void *buf, int size), int fd,
  int count);
#endif // VSFIFO_PIPE

#ifdef VSFIFO_UNIX
// read `size` bytes from stream `fd` to `buf` at once
int vsfifo_read_fd(int fd, void *buf, int size);

// write `size` bytes to stream `fd` from `buf` at once
int vsfifo_write_fd(int fd, const void *buf, int size);

// write to FIFO from UNIX pipe by fd
int vsfifo_from_unix_pipe(vsfifo_t *fifo, int fd, int count);

// read from FIFO to UNIX pipe by fd
int vsfifo_to_unix_pipe(vsfifo_t *fifo, int fd, int count);
#endif // VSFIFO_UNIX
//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif // __plusplus
//----------------------------------------------------------------------------
#endif // VSFIFO_H

/*** end of "vsfifo.h" file ***/


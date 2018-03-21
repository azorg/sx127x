/*
 * Very Simple FIFO (Very Simple Remote Procedure Call (VSRPC) project)
 * Version: 0.9
 * File: "vsfifo.c"
 * (C) 2007-2015 Alex Grinkov <a.grinkov@gmail.com>
 */

// ---------------------------------------------------------------------------
#include <stdlib.h> // malloc(), free(), NULL
#include <string.h> // memcpy()
#include <stdio.h>

#include "vsfifo.h"

#ifdef VSFIFO_UNIX
#  include <unistd.h> // read(), write()
#  include <errno.h>  // errno
#endif // VSFIFO_UNIX
//----------------------------------------------------------------------------
// print FIFO values
void vsfifo_show(vsfifo_t *fifo)
{
  int semval;

  printf("ptr: %p\n", fifo->data);
  printf("in : %p\n", fifo->in);
  printf("out: %p\n", fifo->out);
  printf("count: %d\n", fifo->count);
  printf("size: %d\n", fifo->size);

  vssem_getvalue(&(fifo->read_sem), &semval);

  printf("sem_value: %d\n", semval);
}
//----------------------------------------------------------------------------
// create new FIFO (constructor)
// on success return 0, else -1
int vsfifo_init(vsfifo_t *fifo, int size)
{
  int ret;

  fifo->in = fifo->out = fifo->data =
    (char*) malloc((size_t) (fifo->size = size));
  if (fifo->data == (char*) NULL) return -1; // error: out of memory

  ret = vssem_init(&(fifo->read_sem), 0, 0);
  if (ret == -1)
    return -2;

  return (fifo->count = 0); // success
}
//----------------------------------------------------------------------------
// desrtroy FIFO (destructor)
void vsfifo_release(vsfifo_t *fifo)
{
  free((void*) fifo->data);
}
//----------------------------------------------------------------------------
// clear FIFO
void vsfifo_clear(vsfifo_t *fifo)
{
  fifo->in = fifo->out = fifo->data;
  fifo->count = 0;
}
//----------------------------------------------------------------------------
// return FIFO counter
int vsfifo_count(vsfifo_t *fifo)
{
  return fifo->count;
}
//----------------------------------------------------------------------------
// return FIFO free bytes
int vsfifo_free(vsfifo_t *fifo)
{
  return fifo->size - fifo->count;
}
//----------------------------------------------------------------------------
// write to FIFO from memory buffer
int vsfifo_write(vsfifo_t *fifo, const void *buf, int count)
{
  int tail, head;
  if (count <= 0) return 0; // bad argument
  tail = fifo->size - fifo->count;
  if (count > tail) count = tail;
  tail = fifo->data + fifo->size - fifo->in;
  if (count <= tail)
  { // copy all at once
    memcpy((void*) fifo->in, buf, (size_t) count);
    if (count == tail) fifo->in = fifo->data;
    else               fifo->in += count;
  }
  else // count > tail
  { // copy by two pass
    memcpy((void*) fifo->in, buf, (size_t) tail);
    head = count - tail;
    memcpy((void*) fifo->data, (void*) (((char*) buf) + tail), (size_t) head);
    fifo->in = fifo->data + head;
  }
  fifo->count += count;

  vssem_post(&(fifo->read_sem));

  return count;
}
//----------------------------------------------------------------------------
// read from FIFO to memory buffer non-block
int vsfifo_read_nb(vsfifo_t *fifo, void *buf, int count)
{
  int tail, head;
  if (count <= 0) return 0; // bad argument

  if (count > fifo->count) count = fifo->count;

  tail = fifo->data + fifo->size - fifo->out;
  if (count <= tail)
  { // copy all at once
    memcpy(buf, (const void*) fifo->out, (size_t) count);
    if (count == tail) fifo->out = fifo->data;
    else               fifo->out += count;
  }
  else // count > tail
  { // copy by two pass
    memcpy(buf, (const void*) fifo->out, (size_t) tail);
    head = count - tail;
    memcpy((void*) (((char*) buf) + tail), (const void*) fifo->data,
           (size_t) head);
    fifo->out = fifo->data + head;
  }
  fifo->count -= count;
  return count;
}
//----------------------------------------------------------------------------
// read from FIFO to memory buffer
int vsfifo_read(vsfifo_t *fifo, void *buf, int count)
{
  int tail, head;
  if (count <= 0) return 0; // bad argument

  // wait until fifo size lesser then requested
  while (count > fifo->count)
    vssem_wait(&(fifo->read_sem));

  tail = fifo->data + fifo->size - fifo->out;
  if (count <= tail)
  { // copy all at once
    memcpy(buf, (const void*) fifo->out, (size_t) count);
    if (count == tail) fifo->out = fifo->data;
    else               fifo->out += count;
  }
  else // count > tail
  { // copy by two pass
    memcpy(buf, (const void*) fifo->out, (size_t) tail);
    head = count - tail;
    memcpy((void*) (((char*) buf) + tail), (const void*) fifo->data,
           (size_t) head);
    fifo->out = fifo->data + head;
  }
  fifo->count -= count;

  vssem_init(&(fifo->read_sem), 0, 0);

  return count;
}
//----------------------------------------------------------------------------
// read (and not eject!) from FIFO to memory buffer non-block
int vsfifo_read_back(vsfifo_t *fifo, void *buf, int count)
{
  int tail, head;
  if (count <= 0) return 0; // bad argument

  if (count > fifo->count) count = fifo->count;

  tail = fifo->data + fifo->size - fifo->out;
  if (count <= tail)
  { // copy all at once
    memcpy(buf, (const void*) fifo->out, (size_t) count);
  }
  else // count > tail
  { // copy by two pass
    memcpy(buf, (const void*) fifo->out, (size_t) tail);
    head = count - tail;
    memcpy((void*) (((char*) buf) + tail), (const void*) fifo->data,
            (size_t) head);
  }
  return count;
}
//----------------------------------------------------------------------------
// read from FIFO nowhere (to /dev/null)
int vsfifo_to_nowhere(vsfifo_t *fifo, int count)
{
  int tail;
  if (count <= 0) return 0; // bad argument
  if (count > fifo->count) count = fifo->count;
  tail = fifo->data + fifo->size - fifo->out;
  if (count < tail)
    fifo->out += count;
  else if (count > tail)
    fifo->out = fifo->data + count - tail;
  else // count == tail
    fifo->out = fifo->data;
  fifo->count -= count;
  return count;
}
//----------------------------------------------------------------------------
#ifdef VSFIFO_PIPE
// write to FIFO from pipe
int vsfifo_from_pipe(
  vsfifo_t *fifo,
  int (*read_fn)(int fd, void *buf, int size), int fd,
  int count)
{
  int tail, head;
  if (count <= 0) return 0; // bad argument
  tail = fifo->size - fifo->count;
  if (count > tail) count = tail;
  tail = fifo->data + fifo->size - fifo->in;
  if (count <= tail)
  { // read all from pipe at once
    count = read_fn(fd, (void*) fifo->in, count);
    if (count < 0) return -1; // broken pipe
    if (count == tail) fifo->in = fifo->data;
    else               fifo->in += count;
  }
  else // count > tail
  { // read from pipe by two pass
    head = count - tail;
    count = read_fn(fd, (void*) fifo->in, tail);
    if (count < 0) return -1; // broken pipe
    if (count != tail) // count < tail
    {
      fifo->in += count;
    }
    else // count == tail
    {
      count = read_fn(fd, (void*) fifo->data, head);
      if (count < 0) return -1; // broken pipe
      fifo->in = fifo->data + count;
      count += tail;
    }
  }
  fifo->count += count;
  return count;
}
//----------------------------------------------------------------------------
// read from FIFO to pipe
int vsfifo_to_pipe(
  vsfifo_t *fifo,
  int (*write_fn)(int fd, const void *buf, int size), int fd,
  int count)
{
  int tail, head;
  if (count <= 0) return 0; // bad argument
  if (count > fifo->count) count = fifo->count;
  tail = fifo->data + fifo->size - fifo->out;
  if (count <= tail)
  { // write to pipe at once
    count = write_fn(fd, (const void*) fifo->out, count);
    if (count < 0) return -1; // broken pipe
    if (count == tail) fifo->out = fifo->data;
    else               fifo->out += count;
  }
  else // count > tail
  { // write to pipe by two pass
    head = count - tail;
    count = write_fn(fd, (const void*) fifo->out, tail);
    if (count < 0) return -1; // broken pipe
    if (count != tail) // count < tail
    {
       fifo->out += count;
    }
    else // count == tail
    {
      count = write_fn(fd, (const void*) fifo->data, head);
      if (count < 0) return -1; // broken pipe
      fifo->out = fifo->data + count;
      count += tail;
    }
  }
  fifo->count -= count;
  return count;
}
#endif // VSFIFO_PIPE
//----------------------------------------------------------------------------
#ifdef VSFIFO_UNIX
// read `size` bytes from stream `fd` to `buf` at once
int vsfifo_read_fd(int fd, void *buf, int size)
{
  int retv, cnt = 0;
  char *ptr = (char*) buf;
  while (size > 0)
  {
    retv = read(fd, (void*) ptr, size);
    if (retv < 0)
    {
      if (errno == EINTR)
        continue;
      else
        return -1; // error
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
int vsfifo_write_fd(int fd, const void *buf, int size)
{
  int retv, cnt = 0;
  char *ptr = (char*) buf;
  while (size > 0)
  {
    retv = write(fd, (void*) ptr, size);
    if (retv < 0)
    {
      if (errno == EINTR)
        continue;
      else
        return -1; // error
    }
    ptr += retv;
    cnt += retv;
    size -= retv;
  }
  return cnt;
}
//----------------------------------------------------------------------------
// write to FIFO from UNIX pipe by fd
int vsfifo_from_unix_pipe(vsfifo_t *fifo, int fd, int count)
{
  return vsfifo_from_pipe(fifo, vsfifo_read_fd, fd, count);
}
//----------------------------------------------------------------------------
// read from FIFO to UNIX pipe by fd
int vsfifo_to_unix_pipe(vsfifo_t *fifo, int fd, int count)
{
  return vsfifo_to_pipe(fifo, vsfifo_write_fd, fd, count);
}
#endif // VSFIFO_UNIX
//----------------------------------------------------------------------------

/*** end of "vsfifo.c" file ***/


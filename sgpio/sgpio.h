/*
 * Simple GPIO Linux wrappers
 * File: "sgpio.h"
 */

#ifndef SGPIO_H
#define SGPIO_H
//----------------------------------------------------------------------------
#include <sys/types.h>    // open(), ...
#include <sys/stat.h>     //
#include <fcntl.h>        //
#include <unistd.h>       // close(), read(), write(), lseek(), ...
#include <linux/limits.h> // PATH_MAX
//----------------------------------------------------------------------------
#define SGPIO_MAIN_PATH "/sys/class/gpio/"
//#define SGPIO_MAIN_PATH "./gpio/" // FIXME test directory
//----------------------------------------------------------------------------
// max path size
//#define SGPIO_PATH_MAX 1024
#define SGPIO_PATH_MAX PATH_MAX
//----------------------------------------------------------------------------
// max string length of GPIO number or value
#define SGPIO_STR_MAX 80
//----------------------------------------------------------------------------
// write strings to "direction" file
#define SGPIO_STR_IN  "in"
#define SGPIO_STR_OUT "out"
//----------------------------------------------------------------------------
// write strings to "edge" file
#define SGPIO_STR_NONE    "none"
#define SGPIO_STR_RISING  "rising"
#define SGPIO_STR_FALLING "falling"
#define SGPIO_STR_BOTH    "both"
//----------------------------------------------------------------------------
// inline macro (platform depended)
#ifndef   SGPIO_INLINE
#  define SGPIO_INLINE static inline
#endif // SGPIO_INLINE
//----------------------------------------------------------------------------
//#define SGPIO_DEBUG // FIXME
#ifdef SGPIO_DEBUG
#  include <stdio.h>  // fprintf()
#    define SGPIO_DBG(fmt, arg...) fprintf(stderr, "SGPIO: " fmt "\n", ## arg)
#else
#  define SGPIO_DBG(fmt, ...) // debug output off
#endif // SGPIO_DEBUG
//----------------------------------------------------------------------------
// common error codes (return values)
#define SGPIO_ERR_NONE         0 // no error, success
#define SGPIO_ERR_WRITE       -1 // can't write fo file
#define SGPIO_ERR_EXPORT      -2 // can't export
#define SGPIO_ERR_UNEXPORT    -3 // can't unexport
#define SGPIO_ERR_OPEN_DIR    -4 // can't open "direction" file
#define SGPIO_ERR_OPEN_EDGE   -5 // can't open "edge" file
#define SGPIO_ERR_SET_DIR     -6 // can't write to "direction" file
#define SGPIO_ERR_SET_EDGE    -7 // can't write to "edge" file
#define SGPIO_ERR_OPEN_VAL    -8 // can't open "value" file
#define SGPIO_ERR_UNSET_MODE  -9 // mode is unset
#define SGPIO_ERR_LSEEK      -10 // lseek(0) return non zero 
#define SGPIO_ERR_GET        -11 // read(1) return not a one in sgpio_get() 
#define SGPIO_ERR_SET        -12 // write(1) return not a one in sgpio_set() 
#define SGPIO_ERR_POOL1      -13 // pool() return error #1
#define SGPIO_ERR_POOL2      -14 // pool() return error #2
#define SGPIO_ERR_EPOOL1     -15 // epool() return error #1
#define SGPIO_ERR_EPOOL2     -16 // epool() return error #2
#define SGPIO_ERR_EPOOL3     -17 // epool() return error #3

#define SGPIO_ERROR_NUM        18          // look sgpio_error_str() code
#define SGPIO_ERROR_INDEX(err) (0 - (err)) // ...
//----------------------------------------------------------------------------
// GPIO input/output direction mode
typedef enum {
  SGPIO_DIR_UNSET = 0, // initial set
  SGPIO_DIR_IN,        // input mode
  SGPIO_DIR_OUT        // output mode
} sgpio_dir_t;
//----------------------------------------------------------------------------
// GPIO edge mode
typedef enum {
  SGPIO_EDGE_NONE = 0,
  SGPIO_EDGE_RISING,
  SGPIO_EDGE_FALLING,
  SGPIO_EDGE_BOTH
} sgpio_edge_t;
//----------------------------------------------------------------------------
// `sgpio_t` type structure
typedef struct sgpio_ {
  int num;  // GPIO number /sys/class/gpio/gpioNUM
  int dir;  // GPIO input/output derection mode
  int edge; // GPIO edge mode
  int fd;   // file descriptor of /sys/class/gpio/gpioNUM/value
} sgpio_t;
//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
//----------------------------------------------------------------------------
// write `size` bytes to stream `fd` from `buf` at once
int sgpio_write(int fd, const char *buf, int size);
//----------------------------------------------------------------------------
// write num to /sys/class/gpio/export file
int sgpio_export(int num);
//----------------------------------------------------------------------------
// write num to /sys/class/gpio/unexport file
int sgpio_unexport(int num);
//----------------------------------------------------------------------------
// "constructor"
SGPIO_INLINE void sgpio_init(sgpio_t *self, int num)
{
  self->num  = num;
  self->dir  = SGPIO_DIR_UNSET;
  self->edge = SGPIO_EDGE_NONE;
  self->fd   = -1;
}
//----------------------------------------------------------------------------
// "destructor"
SGPIO_INLINE void sgpio_free(sgpio_t *self)
{
  if (self->fd > 0) close(self->fd);
  self->fd = -1;
}
//----------------------------------------------------------------------------
// set GPIO mode
int sgpio_mode(sgpio_t *self,
               int dir,   // sgpio_dir_t
               int edge); // sgpio_edge_t
//----------------------------------------------------------------------------
// set GPIO number
SGPIO_INLINE void sgpio_set_num(sgpio_t *self, int num)
{
  if (self->fd > 0) close(self->fd);
  sgpio_init(self, num);
}
//----------------------------------------------------------------------------
// get file descriptor of /sys/class/gpio/gpioNUM/value
SGPIO_INLINE int sgpio_fd(const sgpio_t *self) { return self->fd; }
//----------------------------------------------------------------------------
// get GPIO direction mode
SGPIO_INLINE int sgpio_dir(const sgpio_t *self) { return self->dir; }
//----------------------------------------------------------------------------
// get GPIO edge mode
SGPIO_INLINE int sgpio_edge(const sgpio_t *self) { return self->edge; }
//----------------------------------------------------------------------------
// get GPIO number
SGPIO_INLINE int sgpio_num(const sgpio_t *self) { return self->num; }
//----------------------------------------------------------------------------
// get value (return 0 or 1 or error code < 0)
int sgpio_get(sgpio_t *self);
//----------------------------------------------------------------------------
// set value (return 0 or 1 or error code < 0)
int sgpio_set(sgpio_t *self, int val);
//----------------------------------------------------------------------------
// pool wraper for non block read (return 0:false, 1:true, <0:error code)
// msec - timeout in ms
// if sigmask!=0 ignore interrupt by signals
int sgpio_poll_ex(const sgpio_t *self, int msec, int sigmask);
//----------------------------------------------------------------------------
// pool wraper for non block read (return 0:false, 1:true, <0:error code)
// msec - timeout in ms
SGPIO_INLINE int sgpio_poll(const sgpio_t *self, int msec)
{
  return sgpio_poll_ex(self, msec, 0);
}
//----------------------------------------------------------------------------
// epool wraper for non block read (return 0:false, 1:true, <0:error code)
// msec - timeout in ms
int sgpio_epoll(const sgpio_t *self, int msec);
//----------------------------------------------------------------------------
// return SGPIO error string
const char *sgpio_error_str(int err);
//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif // __cplusplus
//----------------------------------------------------------------------------
#endif // SGPIO_H

/*** end of "sgpio.h" file ***/


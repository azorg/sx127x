/*
 * Very Simple Remote Procedure Call (VSRPC) project
 * Version: 0.9
 * File: "vsrpc.h"
 * (C) 2007-2015 Alex Grinkov <a.grinkov@gmail.com>
 */

#ifndef VSRPC_H
#define VSRPC_H
//----------------------------------------------------------------------------
#include <stdlib.h> // NULL free() malloc() atoi() atof()
//----------------------------------------------------------------------------
// inline macro
#ifndef VSRPC_INLINE
#  if __GNUC__
#    define VSRPC_INLINE static inline
#  else
#    ifdef _WIN32
#      define VSRPC_INLINE __forceinline
#    else
#      define VSRPC_INLINE inline
#    endif
#  endif
#endif // VSRPC_INLINE
//----------------------------------------------------------------------------
// version of protocol
#define VSRPC_VERSION "0.9.0"

// if type float and double exist
// use atof(...), snprintf("%g"...), etc
#define VSRPC_FLOAT

// use some "help" (debug) functions
#define VSRPC_HELP

// include some extra function
#define VSRPC_EXTRA

// string delimeters for function name and arguments
#define VSRPC_DELIMETERS " \t" // input
#define VSRPC_SPACE      ' '   // output

// end of line string/character per one function call
#define VSRPC_EOL "\n\r" // input
#define VSRPC_CR  '\n'   // output

// size of sector for input buffer (64 bytes)
#define VSRPC_INBUF_SECTOR 64

// max size of input buffer (1MB)
#define VSRPC_INBUF_MAX_SIZE (1024*1024)

// return keywords which indicate that remote function is finish
// When remote function is finish server must return string
// like this: "ret ReturnValue(s)\n".
#define VSRPC_RETURN_KEYWORD "ret"

// return keywords which indicate that remote function can't execute
// When remote function can't execute server must return string
// like this: "err ErrorCode\n".
#define VSRPC_ERROR_KEYWORD "err"

// return keyword to indicate pong for builtin `ping` fuction
#define VSRPC_PONG_KEYWORD "pong"

// max number of common memory blocks
#define VSRPC_MB_NUM 64

// max size of one common memory block by default (64MB)
#define VSRPC_MB_MAX_SIZE (64*1024*1024)

// bufer for snprintf while convert integer/float to string etc...
#define VSRPC_SNPRINTF_BUF 512

// timeout for select function while sleep [msec]
// used in vsrpc_run_forever() and vsrpc_wait() functions
#define VSRPC_SELECT_TIMEOUT (-1)

// common error codes (return values)
#define VSRPC_ERR_RET     1 // function complete and return
#define VSRPC_ERR_NONE    0 // no error all success
#define VSRPC_ERR_FNF    -1 // function not found
#define VSRPC_ERR_PERM   -2 // permission denied
#define VSRPC_ERR_EOP    -3 // end of pipe (disconnect)
#define VSRPC_ERR_EMPTY  -4 // empty pipe (no data in pipe) if non block read
#define VSRPC_ERR_OVER   -5 // input buffer too big
#define VSRPC_ERR_MEM    -6 // memory error (malloc return NULL)
#define VSRPC_ERR_NFMB   -7 // no free memory block
#define VSRPC_ERR_TBMB   -8 // try to allocate too big memory block
#define VSRPC_ERR_BUSY   -9 // can't call more the one function
#define VSRPC_ERR_NRUN  -10 // procedure not run yet
#define VSRPC_ERR_PROT  -11 // error of protoco
#define VSRPC_ERR_BARG  -12 // bad argument
#define VSRPC_ERR_EXIT  -13 // client call "exit" => goodbye

#define VSRPC_ERROR_NUM        15          // look vsrpc_error_str() code
#define VSRPC_ERROR_INDEX(err) (1 - (err)) //

// permissions flags
#define VSRPC_PERM_READ   (1<<0) // read memory from server
#define VSRPC_PERM_WRITE  (1<<1) // write to memory on server
#define VSRPC_PERM_MALLOC (1<<2) // allocate memory at server
#define VSRPC_PERM_FREE   (1<<3) // free memory at server
#define VSRPC_PERM_EXIT   (1<<4) // abort server function
#define VSRPC_PERM_CALL   (1<<5) // call user-defined functions
#define VSRPC_PERM_CDEF   (1<<6) // call default function
#define VSRPC_PERM_CBACK  (1<<7) // callback procedure from server
//#define VSRPC_PERM_EXEC (1<<7) // execute another program

// "default" permissions (must be "safe")
#define VSRPC_PERM_DEFAULT \
  (VSRPC_PERM_READ | VSRPC_PERM_EXIT | VSRPC_PERM_CALL | \
   VSRPC_PERM_CDEF | VSRPC_PERM_CBACK)

// "all" permissions
#define VSRPC_PERM_ALL \
  (VSRPC_PERM_READ | VSRPC_PERM_WRITE | VSRPC_PERM_MALLOC | \
   VSRPC_PERM_FREE | VSRPC_PERM_EXIT  | VSRPC_PERM_CALL   | \
   VSRPC_PERM_CDEF | VSRPC_PERM_CBACK)
//----------------------------------------------------------------------------
typedef struct vsrpc_func_ vsrpc_func_t;
typedef struct vsrpc_mb_   vsrpc_mb_t;
typedef struct vsrpc_      vsrpc_t;
//----------------------------------------------------------------------------
// builtin and user-defined functions structure
struct vsrpc_func_ {
  char *fname;    // name of function or NULL
  char** (*fptr)( // pointer to function
    vsrpc_t *rpc,         // VSRPC structure
    int argc,             // number of arguments
    char * const argv[]); // arguments (strings)
};
//----------------------------------------------------------------------------
// memory blocks structure
struct vsrpc_mb_ {
  char *ptr; // pointer to memory block
  int stat;  // if 1 then ptr is pointer to static memory
  int size;  // size of memory block or 0 if block is free
};
//----------------------------------------------------------------------------
// state of RPC module
typedef enum {
  VSRPC_STOP = 0, // - VSRPC code not running (initial state)
  VSRPC_LISTEN,   // - VSRPC server listen client
  VSRPC_CALL,     // - VSRPC client call procedure on server
  VSRPC_CBACK     // - VSRPC server callback procedure on client
} vsrpc_state_t;
//----------------------------------------------------------------------------
// server object structure
struct vsrpc_ {
  vsrpc_state_t state; // current state
  vsrpc_func_t *bfunc; // list of builtin functions
  vsrpc_func_t *func;  // list of user-defined functions or NULL
  char** (*def_func)(  // pointer to default function or NULL
    vsrpc_t *rpc,         // VSRPC structure
    int argc,             // number of arguments
    char * const argv[]); // arguments (strings)
  int perm;            // current permissions for client
  void *context;       // pointer to optional data or NULL
  int retc;            // number of return value(s) of last procedure call
  char **retv;         // return value(s) of last procedure call
  vsrpc_mb_t *mb;      // pointer to array of memory block structrures
  int mb_max_size;     // max size of one memory block (VSRPC_MB_MAX_SIZE)
  char *inbuf_ptr;     // line input bufer for one procedure call
  int inbuf_size;      // current alocated size of input bufer
  int inbuf_max_size;  // max size of inbuf (else return VSRPC_ERR_OVER)
  int inbuf_cnt;       // inbuf received bytes (inbuf_cnt <= inbuf_size)
  int inbuf_len;       // inbuf length of message (inbuf_len <= inbuf_cnt)

  // file descriptors for "low layer"
  int fd_rd; // for read() and select() functions
  int fd_wr; // for write() and flush() functions

  // external function ("low layer")
  // This function like read and write (man read; man write),
  // but this function must return:
  //   RetVal > 0 - bytes read or written
  //   RetVal = 0 - End Of Pipe (close connection, EOF...)
  int (*read)  (int fd, void* buf, int size); // read from pipe function
  int (*write) (int fd, const void* buf, int size); // write to pipe func.
  int (*select)(int fd, int msec); // check input pipe for nonblock read (or NULL)
  void (*flush)(int fd); // flush output pipe (or NULL)

  // exit code if another side say `exit code`
  int exit_value;
};
//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __plusplus
//----------------------------------------------------------------------------
// allocate memory (return NULL on failure)
VSRPC_INLINE char *vsrpc_malloc(int size)
{ return (char*) malloc((size_t) size); }
//----------------------------------------------------------------------------
// free memory
VSRPC_INLINE void vsrpc_free(char *ptr)
{ free((void*) ptr); }
//----------------------------------------------------------------------------
// get last exit code
VSRPC_INLINE int vsrpc_exit_value(vsrpc_t *rpc)
{ return rpc->exit_value; }
//----------------------------------------------------------------------------
// return VSRPC error string
const char *vsrpc_error_str(int err);
//----------------------------------------------------------------------------
// reallocate memory (return NULL on failure)
//char *vsrpc_realloc(char *old, int old_size, int new_size);
//----------------------------------------------------------------------------
// init function
// return ErrorCode
int vsrpc_init(
  vsrpc_t *rpc,       // VSRPC object structure
  vsrpc_func_t *func, // list of user-defined functions or NULL
  char** (*def_func)( // pointer to default function or NULL
    vsrpc_t *rpc,         // VSRPC structure
    int argc,             // number of arguments
    char * const argv[]), // arguments (strings)
  int perm,           // permissions for client
  void *context,      // pointer to optional data (or NULL)
  int fd_rd,          // file descriptor for fn_read() and fn_select()
  int fd_wr,          // file descriptor for fn_write() and fn_flush()
  int (*fn_read)(     // read from pipe function
    int fd,               // file descriptor (socket)
    void* buf,            // input buffer
    int size),            // bufer size
  int (*fn_write)(    // write to pipe function
    int fd,               // file descriptor (socket)
    const void* buf,      // output bufer
    int size),            // bufer size
  int (*fn_select)(   // check input pipe function for nonblock read (or NULL)
    int fd,               // file descriptor (socket)
    int msec),            // timeout [ms], (-1 <=> forever)
  void (*fn_flush)(   // flush output pipe function (or NULL)
    int fd));             // file descriptor (socket)
//----------------------------------------------------------------------------
// release function
void vsrpc_release(vsrpc_t *rpc);
//----------------------------------------------------------------------------
// allow to call one procedure (or wait previous results)
int vsrpc_run(vsrpc_t *rpc); // return ErrorCode
//----------------------------------------------------------------------------
// allow to call procedures until disconnect or exit or error
int vsrpc_run_forever(vsrpc_t *rpc); // return ErrorCode
//----------------------------------------------------------------------------
// call procedure
int vsrpc_call(vsrpc_t *rpc, char * const argv[]); // return ErrorCode
//----------------------------------------------------------------------------
// call procedure with "friendly" interface
// list: s-string, i-integer, f-float, d-double
int vsrpc_call_ex(vsrpc_t *rpc, const char *list, ...); // return ErrorCode
//----------------------------------------------------------------------------
// check is procedure finished or run yet
int vsrpc_check(vsrpc_t *rpc); // return ErrorCode
//----------------------------------------------------------------------------
// wait until procedure run
int vsrpc_wait(vsrpc_t *rpc); // return ErrorCode
//----------------------------------------------------------------------------
// check for nonblock call of vsrpc_read_one()
// return {1:true, 0:false, -1:error}
int vsrpc_select(vsrpc_t *rpc, int msec);
//----------------------------------------------------------------------------
// read from pipe (may be read from input bufer)
// return ErrorCode: VSRPC_ERR_NONE | VSRPC_ERR_EOP
// nubmer of read bytes: count = size or count < size
int vsrpc_read_one(vsrpc_t *rpc, char *ptr, int size, int *count);
//----------------------------------------------------------------------------
// read block from pipe at once (may be read from input bufer)
// return ErrorCode: VSRPC_ERR_NONE | VSRPC_ERR_EOP
int vsrpc_read(vsrpc_t *rpc, char *ptr, int size);
//----------------------------------------------------------------------------
// write block to pipe at once
// return ErrorCode: VSRPC_ERR_NONE | VSRPC_ERR_EOP
int vsrpc_write(vsrpc_t *rpc, const char *ptr, int size);
//----------------------------------------------------------------------------
// get string from input pipe to input bufer
// may return empty string, check rpc->inbuf_len!
// return ErrorCode: VSRPC_ERR_NONE | VSRPC_ERR_EOP | VSRPC_ERR_EMPTY
//                   VSRPC_ERR_MEM | VSRPC_ERR_OVER
int vsrpc_gets_in(vsrpc_t *rpc); // return ErrorCode
//----------------------------------------------------------------------------
// get string from input pipe to input bufer
// skip empty string!
// return ErrorCode: VSRPC_ERR_NONE | VSRPC_ERR_EOP | VSRPC_ERR_EMPTY
//                   VSRPC_ERR_MEM | VSRPC_ERR_OVER
int vsrpc_gets(vsrpc_t *rpc); // return ErrorCode
//----------------------------------------------------------------------------
// put string to output pipe
// return ErrorCode: VSRPC_ERR_NONE | VSRPC_ERR_EOP
//int vsrpc_puts(vsrpc_t *rpc, const char *str); // return ErrorCode
//----------------------------------------------------------------------------
// put string to output pipe with VSRPC_CR code on the end
// return ErrorCode: VSRPC_ERR_NONE | VSRPC_ERR_EOP | VSRPC_ERR_MEM
int vsrpc_putsn(vsrpc_t *rpc, const char *str); // return ErrorCode
//----------------------------------------------------------------------------
// return value(s)
// put string like this: "ret [RetVal1] [RetVal2] ...\n"
// return ErrorCode: VSRPC_ERR_NONE | VSRPC_ERR_EOP | VSRPC_ERR_MEM
int vsrpc_return_value(vsrpc_t *rpc, char * const retv[]);
//----------------------------------------------------------------------------
// return error code
// put string like this: "error ErrorCode\n"
// return ErrorCode: VSRPC_ERR_NONE | VSRPC_ERR_EOP | VSRPC_ERR_MEM
int vsrpc_return_error(vsrpc_t *rpc, int error);
//----------------------------------------------------------------------------
// receive array from pipe
// format: 0-char(binary), 1-int(text), 2-float(text), 3-double(text)
// return ErrorCode: VSRPC_ERR_NONE | VSRPC_ERR_EOP | VSRPC_ERR_MEM or
//                   VSRPC_ERR_OVER
int vsrpc_receive(vsrpc_t *rpc, const void *ptr, int size,
                  int format, int *count);
//----------------------------------------------------------------------------
// transmit array to pipe
// format: 0-char(binary), 1-int(text), 2-float(text), 3-double(text)
// return ErrorCode: VSRPC_ERR_NONE | VSRPC_ERR_EOP | VSRPC_ERR_MEM
int vsrpc_transmit(vsrpc_t *rpc, const void *ptr, int size,
                   int format, int *count);
//----------------------------------------------------------------------------
// pack string ('\ ', '\t', '\\', '\n', '\r')
// return pointer to new string or NULL on failure
char *vsrpc_pack_str(const char *str);
//----------------------------------------------------------------------------
// unpack string ('\ ', '\t', '\\', '\n', '\r', '\0')
// return pointer to new string or NULL on failure
char *vsrpc_unpack_str(const char *str);
//----------------------------------------------------------------------------
// pack one string from argv argument list
// return pointer to new string or NULL on failure
char *vsrpc_argv2str(char * const argv[]);
//----------------------------------------------------------------------------
// unpack argv argument list from one string
// return argv or NULL on failure
char **vsrpc_str2argv(const char *str);
//----------------------------------------------------------------------------
// create argv argument list from variable list
// list: s-string, i-integer, f-float, d-double
// return argv or NULL on failure
char **vsrpc_list2argv(const char *list, ...);
//----------------------------------------------------------------------------
// free memory from argv strings
void vsrpc_free_argv(char *argv[]);
//----------------------------------------------------------------------------
// get argc (number of values) of argv
int vsrpc_argv2argc(char * const argv[]);
//----------------------------------------------------------------------------
// shift argv
// return pointer to new shifted argv or NULL on failure
char **vsrpc_shift_argv(char * const argv[]);
//----------------------------------------------------------------------------
// convert integer value to string
// return pointer to new string or NULL on failure
char *vsrpc_int2str(int val);
//----------------------------------------------------------------------------
// convert string to integer value
VSRPC_INLINE int vsrpc_str2int(const char *str)
{
  if (str == (char*) NULL) return 0; // argument error
  return atoi(str);
}
//----------------------------------------------------------------------------
#ifdef VSRPC_FLOAT
// convert float value to string
// return pointer to new string or NULL on failure
char *vsrpc_float2str(float val);
//----------------------------------------------------------------------------
// convert double value to string
// return pointer to new string or NULL on failure
char *vsrpc_double2str(double val);
//----------------------------------------------------------------------------
// convert string to double value
VSRPC_INLINE double vsrpc_str2double(const char *str)
{
  if (str == (char*) NULL) return 0.; // argument error
  return atof(str);
}
#endif // VSRPC_FLOAT
//----------------------------------------------------------------------------
// create dynamic new copy of string
// return pointer to new string or NULL on failure
char *vsrpc_str2str(char *str);
//----------------------------------------------------------------------------
#ifdef VSRPC_EXTRA
// add some string(s) to one
// return pointer to new string or NULL on failure
char *vsrpc_stradd(int n, ...);
//----------------------------------------------------------------------------
// convert binary array to hex string (alya "01:23:AB:CD")
// return pointer to new string or NULL on failure
char *vsrpc_bin2hex(const char *bin, int len);
//----------------------------------------------------------------------------
// convert hex string (alya "01:23:AB:CD") to binary array
char *vsrpc_hex2bin(const char *str);
// return pointer to new string or NULL on failure
#endif // VSRPC_EXTRA
//----------------------------------------------------------------------------
// create new common memory block
//    input: Size
//   return: ErrorCode [MemoryBlockID]
//   errors: VSRPC_ERR_NONE | VSRPC_ERR_PERM | VSRPC_ERR_MEM |
//           VSRPC_ERR_BARG | VSRPC_ERR_NFMB | VSRPC_ERR_TBMB
char **vsrpc_builtin_malloc(vsrpc_t *rpc, int argc, char * const argv[]);
int vsrpc_local_malloc(vsrpc_t *rpc, int size, int *id); // return ErrorCode
int vsrpc_remote_malloc(vsrpc_t *rpc, int size, int *id); // return ErrorCode
//----------------------------------------------------------------------------
// free used common memory block
//    input: MemoryBlockID
//   return: ErrorCode
//   errors: VSRPC_ERR_NONE | VSRPC_ERR_PERM | VSRPC_ERR_BARG
char **vsrpc_builtin_free(vsrpc_t *rpc, int argc, char * const argv[]);
int vsrpc_local_free(vsrpc_t *rpc, int id); // return ErrorCode
int vsrpc_remote_free(vsrpc_t *rpc, int id); // return ErrorCode
//----------------------------------------------------------------------------
// stat common memory block
//    input: MemoryBlockID
//   return: ErrorCode [Size(0 - if unused)] [Stat(0-dynamic, 1-static)]
//   errors: VSRPC_ERR_NONE | VSRPC_ERR_BARG
char **vsrpc_builtin_stat(vsrpc_t *rpc, int argc, char * const argv[]);
int vsrpc_local_stat(vsrpc_t *rpc, int id, int *size, int *stat); // return ErrorCode
int vsrpc_remote_stat(vsrpc_t *rpc, int id, int *size, int *stat); // return ErrorCode
//----------------------------------------------------------------------------
// read from common memory block
//    input: MemoryBlockID Offset Size Format
//   return: ErrorCode [Count]
//   errors: VSRPC_ERR_NONE | VSRPC_ERR_PERM | VSRPC_ERR_BARG |
//           VSRPC_ERR_MEM
//   format: 0-char(bibary), 1-int(text), 2-float(text), 3-double(text)
char **vsrpc_builtin_read(vsrpc_t *rpc, int argc, char * const argv[]);
int vsrpc_local_read(vsrpc_t *rpc,
                     void *ptr, // ptr - destination address
                     int id, int off, int size,
                     int *count); // return ErrorCode
int vsrpc_remote_read(vsrpc_t *rpc,
                      void *ptr, // ptr - destination address
                      int id, int off, int size, int format,
                      int *count); // return ErrorCode
//----------------------------------------------------------------------------
// write to common memory block
//    input: MemoryBlockID Offset Size Format
//   return: ErrorCode [Count]
//   errors: VSRPC_ERR_NONE | VSRPC_ERR_PERM | VSRPC_ERR_BARG |
//           VSRPC_ERR_MEM
//   format: 0-char(binary), 1-int(text), 2-float(text), 3-double(text)
char **vsrpc_builtin_write(vsrpc_t *rpc, int argc, char * const argv[]);
int vsrpc_local_write(vsrpc_t *rpc,
                      const void *ptr, // ptr - source address
                      int id, int off, int size,
                      int *count); // return ErrorCode
int vsrpc_remote_write(vsrpc_t *rpc,
                       const void *ptr, // ptr - source address
                       int id, int off, int size, int fmt,
                       int *count); // return ErrorCode
//----------------------------------------------------------------------------
// simple "ping"
//    input: Nothing
//   return: ErrorCode "pong"
//   errors: VSRPC_ERR_NONE
char **vsrpc_builtin_ping(vsrpc_t *rpc, int argc, char * const argv[]);
int vsrpc_remote_ping(vsrpc_t *rpc, int *ack); // return ErrorCode
//----------------------------------------------------------------------------
// terminate connection
//    input: [ExitValue(s)]
//   return: ErrorCode
//   errors: VSRPC_ERR_PERM | VSRPC_ERR_EXIT
char **vsrpc_builtin_exit(vsrpc_t *rpc, int argc, char * const argv[]);
int vsrpc_remote_exit(vsrpc_t *rpc, int retv); // return ErrorCode
//----------------------------------------------------------------------------
#ifdef VSRPC_HELP
// get list of all builtin functions
//    input: Nothing
//   return: "func_name1" "func_name2" ...
//   errors: VSRPC_ERR_NONE
char **vsrpc_builtin_help(vsrpc_t *rpc, int argc, char * const argv[]);
int vsrpc_local_help(vsrpc_t *rpc, char ***list); // return VSRPC_ERR_NONE
int vsrpc_remote_help(vsrpc_t *rpc, char ***list); // return ErrorCode
//----------------------------------------------------------------------------
// get list of all user-defined functions
//    input: Nothing
//   return: "func_name1" "func_name2" ...
//   errors: VSRPC_ERR_NONE
char **vsrpc_builtin_list(vsrpc_t *rpc, int argc, char * const argv[]);
int vsrpc_local_list(vsrpc_t *rpc, char ***list); // return VSRPC_ERR_NONE
int vsrpc_remote_list(vsrpc_t *rpc, char ***list); // return ErrorCode
#endif // VSRPC_HELP
//----------------------------------------------------------------------------
// get version of protocol
//    input: Nothing
//   return: ErrorCode "version"
//   errors: VSRPC_ERR_NONE
char **vsrpc_builtin_version(vsrpc_t *rpc, int argc, char * const argv[]);
int vsrpc_local_version(vsrpc_t *rpc, char **ver); // return VSRPC_ERR_NONE
int vsrpc_remote_version(vsrpc_t *rpc, char **ver); // return ErrorCode
//----------------------------------------------------------------------------
// compare remote and local protocol version
//    input: Nothing
//   return: ErrorCode, *ok=1 - verion eqval, *ok=0 - bad version
//   errors: VSRPC_ERR_NONE | -???
int vsrpc_check_version(vsrpc_t *rpc, int *ok);
//----------------------------------------------------------------------------
// get permissions
//    input: Nothing
//   return: ErrorCode permissins
//   errors: VSRPC_ERR_NONE
char **vsrpc_builtin_perm(vsrpc_t *rpc, int argc, char * const argv[]);
int vsrpc_local_perm(vsrpc_t *rpc, int *perm); // return VSRPC_ERR_NONE
int vsrpc_remote_perm(vsrpc_t *rpc, int *perm); // return ErrorCode
//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif // __plusplus
//----------------------------------------------------------------------------
#endif // VSRPC_H

/*** end of "vsrpc.h" file ***/


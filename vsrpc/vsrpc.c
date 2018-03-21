/*
 * Very Simple Remote Procedure Call (VSRPC) project
 * Version: 0.9
 * File: "vsrpc.c"
 * (C) 2007-2015 Alex Grinkov <a.grinkov@gmail.com>
 */

//----------------------------------------------------------------------------
#include <string.h> // strchr() memcpy() strcmp() strlen()
#include <stdio.h>  // snprintf()
#include <stdarg.h> // va_list, va_start, va_end, va_arg
#include "vsrpc.h"
//============================================================================
// builtin functions structure
static vsrpc_func_t vsrpc_builtin_functions[] = {
  { "malloc",  vsrpc_builtin_malloc  }, // allocate memory block
  { "free",    vsrpc_builtin_free    }, // free memory block
  { "stat",    vsrpc_builtin_stat    }, // get status memory block
  { "read",    vsrpc_builtin_read    }, // read from memory block
  { "write",   vsrpc_builtin_write   }, // write to memory block
  { "ping",    vsrpc_builtin_ping    }, // "ping" another part
  { "exit",    vsrpc_builtin_exit    }, // terminate another part
  { "help",    vsrpc_builtin_help    }, // list of builtin functions
  { "list",    vsrpc_builtin_list    }, // list of user-defined functions
  { "version", vsrpc_builtin_version }, // get version of protocol
  { "perm",    vsrpc_builtin_perm    }, // get permissions
  { NULL, NULL }
};
//----------------------------------------------------------------------------
const char *vsrpc_errors[] = {
  "function complete and return",
  "no error all success",
  "function not found",
  "permission denied",
  "end of pipe (disconnect)",
  "empty pipe (no data in pipe)",
  "input buffer too big",
  "memory error (malloc return NULL)",
  "no free memory block",
  "try to allocate too big memory block",
  "can't call more the one function",
  "procedure not run yet",
  "error of protocol",
  "bad argument",
  "client call 'exit'",
};
static const char *vsrpc_error_unknown = "unknown error";
//============================================================================
/*
// reallocate memory (return NULL on failure)
char *vsrpc_realloc(char *old, int old_size, int new_size)
{
  int min_size;
  char *new;

  if (old_size == new_size)
    return old; // do nothing
  min_size = new_size > old_size ? old_size : new_size;
  new = vsrpc_malloc(new_size);
  if (new != NULL)
    memcpy((void*) new, (const void*) old, (size_t) min_size);
  vsrpc_free(old);

  return new;
}
*/
//----------------------------------------------------------------------------
// return VSRPC error string
const char *vsrpc_error_str(int err)
{
  err = VSRPC_ERROR_INDEX(err);
  return (err >= 0 && err < VSRPC_ERROR_NUM) ?
         (char*) vsrpc_errors[err] :
         (char*) vsrpc_error_unknown;
}
//----------------------------------------------------------------------------
// init function
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
    int fd))              // file descriptor (socket)
{
  int i;

  // fill VSRPC structure by arguments
  rpc->func     = func;
  rpc->def_func = def_func;
  rpc->perm     = perm;
  rpc->context  = context;
  rpc->fd_rd    = fd_rd;
  rpc->fd_wr    = fd_wr;
  rpc->read     = fn_read;
  rpc->write    = fn_write;
  rpc->select   = fn_select;
  rpc->flush    = fn_flush;

  // init input bufer
  rpc->inbuf_size = rpc->inbuf_cnt = rpc->inbuf_len = 0;
  rpc->inbuf_max_size = VSRPC_INBUF_MAX_SIZE;

  // reset return values by default
  rpc->retc = 0;

  // set pointer to build-in functions
  rpc->bfunc = vsrpc_builtin_functions;

  // set initial VSRPC state
  rpc->state = VSRPC_STOP;

  // set max size of memory block by default
  rpc->mb_max_size = VSRPC_MB_MAX_SIZE;

  // allocate array of memory block structrures
  rpc->mb = (vsrpc_mb_t*)
    vsrpc_malloc((int) VSRPC_MB_NUM * sizeof(vsrpc_mb_t));
  if (rpc->mb == (vsrpc_mb_t*) NULL)
    return VSRPC_ERR_MEM; // memory exeption
  for (i = 0; i < VSRPC_MB_NUM; i++)
  {
    rpc->mb[i].stat = 0; // set by default
    rpc->mb[i].size = 0; // all descriptors are free
  }

  // set exit value to default
  rpc->exit_value = 0;

  return VSRPC_ERR_NONE;
}
//----------------------------------------------------------------------------
// release function
void vsrpc_release(vsrpc_t* rpc)
{
  int i;
  // free bufer
  if (rpc->inbuf_size != 0)
  {
    vsrpc_free(rpc->inbuf_ptr);
    rpc->inbuf_size = rpc->inbuf_cnt = rpc->inbuf_len = 0;
  }

  // free memory from return values
  if (rpc->retc != 0) { vsrpc_free_argv(rpc->retv); rpc->retc = 0; }

  // free array of memory block structrures
  if (rpc->mb != (vsrpc_mb_t*) NULL)
  {
    for (i = 0; i < VSRPC_MB_NUM; i++)
    {
      if (rpc->mb[i].size == 0 || rpc->mb[i].stat != 0)
        continue;
      vsrpc_free((char*) rpc->mb[i].ptr);
      rpc->mb[i].size = 0; // free all sctructures
    }
    vsrpc_free((char*) rpc->mb);
    rpc->mb = (vsrpc_mb_t*) NULL;
  }
}
//----------------------------------------------------------------------------
// allow to call one procedure (or wait previous results)
int vsrpc_run(vsrpc_t* rpc)
{
  char **argv, **rv, *fname;
  int retv, argc;
  vsrpc_func_t *fn;
  vsrpc_func_t *bfn;

  // set VSRPC state
  if (rpc->state == VSRPC_STOP)
    rpc->state = VSRPC_LISTEN; // run server (and listen client)

  // free memory from previous return values
  if (rpc->retc != 0) { vsrpc_free_argv(rpc->retv); rpc->retc = 0; }

  while (1)
  {
    // read string from input pipe
    retv = vsrpc_gets(rpc);
    if (retv != VSRPC_ERR_NONE) break;

    // unpack arguments list
    if ((argv = vsrpc_str2argv(rpc->inbuf_ptr)) == (char**) NULL)
    {
      retv = VSRPC_ERR_EMPTY; // memory exeption or empty string?
      break;
    }

    // calculate number of arguments
    argc = vsrpc_argv2argc(argv);

    // store pointer to first argument (function name)
    fname = *argv;

    // check if procedure return
    if (strcmp(VSRPC_RETURN_KEYWORD, fname) == 0)
    {
      if (rpc->state != VSRPC_LISTEN) // VSRPC_CALL || VSRPC_CBACK
      {
        if (argc > 1)
        { // return argument list exist
          rpc->retv = vsrpc_shift_argv(argv); // return values
          rpc->retc = argc - 1;
        }
        vsrpc_free_argv(argv);
        retv = VSRPC_ERR_NONE; // procedure is finish
        break;
      }
      else // rpc->state == VSRPC_LISTEN
      { // protocol error
        vsrpc_free_argv(argv); // free memory from argv
        retv = vsrpc_return_error(rpc, VSRPC_ERR_PROT);
        if (retv != VSRPC_ERR_NONE) break; // return error
        continue;
      }
    }

    // check if procedure can't finish success
    if (strcmp(VSRPC_ERROR_KEYWORD, fname) == 0 )
    {
      if (rpc->state != VSRPC_LISTEN) // VSRPC_CALL || VSRPC_CBACK
      {
        if (argc == 2)
          // number of argument is correct
          retv = vsrpc_str2int(argv[1]); // integer value of error
        else
          retv = VSRPC_ERR_PROT; // protocol error
        vsrpc_free_argv(argv);
        break;
      }
      else // rpc->state == VSRPC_LISTEN
      { // protocol error
        vsrpc_free_argv(argv); // free memory from argv
        retv = vsrpc_return_error(rpc, VSRPC_ERR_PROT);
        if (retv != VSRPC_ERR_NONE) break; // return error
        continue;
      }
    }

    // check recursive callback
    if (rpc->state == VSRPC_CBACK)
    {
      // recursive call detected
      vsrpc_free_argv(argv); // free memory from argv
      retv = vsrpc_return_error(rpc, VSRPC_ERR_PROT);
      if (retv != VSRPC_ERR_NONE) break; // return error
      continue;
    }

    // check permission for callback
    if ( rpc->state == VSRPC_CALL &&
         (rpc->perm & VSRPC_PERM_CBACK) == 0 )
    {
      // permission denied for callback
      vsrpc_free_argv(argv); // free memory from argv
      retv = vsrpc_return_error(rpc, VSRPC_ERR_PERM);
      if (retv != VSRPC_ERR_NONE) break; // return error
      continue;
    }

    // try to find "user-defined" function
    if (rpc->func != (vsrpc_func_t*) NULL)
    { // user-defined functions exists
      for (fn = rpc->func; fn->fname != (char*) NULL; fn++)
        if (strcmp(fn->fname, fname) == 0) break;
      if (fn->fname != (char*) NULL)
      { // try to call user-defined function
        if ((rpc->perm & VSRPC_PERM_CALL) != 0)
        { // permissions is valid
          rv = fn->fptr(rpc, argc, argv); // call user-defined function
          vsrpc_free_argv(argv);
          retv = vsrpc_return_value(rpc, rv); // return value(s) to client
          vsrpc_free_argv(rv);
          if (retv != VSRPC_ERR_NONE) break; // return error
        }
        else
        { // permission denied
          vsrpc_free_argv(argv);
          retv = vsrpc_return_error(rpc, VSRPC_ERR_PERM);
          if (retv != VSRPC_ERR_NONE) break; // return error
        }
        if (rpc->state == VSRPC_LISTEN)
        {
          rpc->state = VSRPC_STOP; // restore VSRPC state
          return VSRPC_ERR_RET; // user-defined funcion finish
        }
        continue;
      }
    }

    // try to find "buildin" function
    for (bfn = rpc->bfunc; bfn->fname != (char*) NULL; bfn++)
      if (strcmp(bfn->fname, fname) == 0) break;
    if (bfn->fname != (char*) NULL)
    { // call builtin function
      rv = bfn->fptr(rpc, argc, argv); // call builtin function
      retv = vsrpc_return_value(rpc, rv); // return value(s) to client
      if (retv != VSRPC_ERR_NONE)
      {
        vsrpc_free_argv(rv);
        vsrpc_free_argv(argv);
        break; // return error
      }
      if (vsrpc_argv2argc(rv) != 0)
        retv = vsrpc_str2int(*rv); // store error code of builin function
      vsrpc_free_argv(rv);
      // check if exit was called or end of pipe
      if (retv == VSRPC_ERR_EXIT || retv == VSRPC_ERR_EOP)
      { // exit by remote machine
        if (argc > 1 && retv == VSRPC_ERR_EXIT)
        { // exit argument list exist
          rpc->retv = vsrpc_shift_argv(argv); // return values
          rpc->retc = argc - 1;
        }
        vsrpc_free_argv(argv);
        break; // exit or end of pipe
      }
      vsrpc_free_argv(argv);
      if (rpc->state == VSRPC_LISTEN)
      {
        rpc->state = VSRPC_STOP; // restore VSRPC state
        return VSRPC_ERR_RET; // user-defined funcion finish
      }
      continue;
    }

    // try to run "default" function
    if (rpc->def_func != (char** (*)(vsrpc_t*, int, char *const[])) NULL)
    {
      if ((rpc->perm & VSRPC_PERM_CDEF) != 0)
      { // permissions is valid
        rv = rpc->def_func(rpc, argc, argv); // call default function
        vsrpc_free_argv(argv);
        retv = vsrpc_return_value(rpc, rv); // return value(s) to client
        vsrpc_free_argv(rv);
        if (retv != VSRPC_ERR_NONE) break; // return error
      }
      else
      { // permission denied
        vsrpc_free_argv(argv);
        retv = vsrpc_return_error(rpc, VSRPC_ERR_PERM);
        if (retv != VSRPC_ERR_NONE) break; // return error
      }
      if (rpc->state == VSRPC_LISTEN)
      {
        rpc->state = VSRPC_STOP; // restore VSRPC state
        return VSRPC_ERR_RET; // user-defined funcion finish
      }
      continue;
    }

    // function not found
    vsrpc_free_argv(argv); // free memory from argv
    retv = vsrpc_return_error(rpc, VSRPC_ERR_FNF);
    if (retv != VSRPC_ERR_NONE) break; // return error
  } // while (1)

  // restore VSRPC state
  if (retv != VSRPC_ERR_EMPTY)
  {
    if (rpc->state == VSRPC_CBACK)
      rpc->state = VSRPC_LISTEN;
    else // rpc->state == VSRPC_LISTEN || rpc->state == VSRPC_CALL
      rpc->state = VSRPC_STOP;
  }

  return retv;
}
//----------------------------------------------------------------------------
// allow to call procedures until disconnect or exit or error
int vsrpc_run_forever(vsrpc_t *rpc)
{
  int retv;
  while (1)
  {
    if (vsrpc_select(rpc, VSRPC_SELECT_TIMEOUT) == 0) continue;
    retv = vsrpc_run(rpc);
    if (retv != VSRPC_ERR_EMPTY &&
    retv != VSRPC_ERR_RET &&
    retv != VSRPC_ERR_NONE) break;
  }
  return retv;
}
//----------------------------------------------------------------------------
// call procedure
int vsrpc_call(vsrpc_t *rpc, char * const argv[])
{
  int argc;
  char *str;

  // check state
  if (rpc->state == VSRPC_CALL || rpc->state == VSRPC_CBACK)
    // sory can't run again
    // must wait result of previous call
    return VSRPC_ERR_BUSY;

  // check argument
  if ((argc = vsrpc_argv2argc(argv)) == 0)
    return VSRPC_ERR_BARG;

  // free memory from previous return values if need
  if (rpc->retc != 0) { vsrpc_free_argv(rpc->retv); rpc->retc = 0; }

  // prepare string to call
  str = vsrpc_argv2str(argv);
  if (str == (char*) NULL) return VSRPC_ERR_MEM; // abnormal return

  // call procedure on another part
  argc = vsrpc_putsn(rpc, str);
  vsrpc_free(str); // free memory
  if (argc != VSRPC_ERR_NONE) return argc; // error (may be end of pipe?)

  // flush output pipe
  if (rpc->flush != (void (*)(int)) NULL) rpc->flush(rpc->fd_wr);

  // switch state to "ANOTHER PART IS BUSY"
  if (rpc->state == VSRPC_STOP)
    rpc->state = VSRPC_CALL; // client listen and wait server
  else // rpc->state == VSRPC_LISTEN
    rpc->state = VSRPC_CBACK; // server listen and wait client

  return VSRPC_ERR_NONE; // ok, but remote function not finished yet
}
//----------------------------------------------------------------------------
// call remote function with "friendly" interface
// list: s-string, i-integer, f-float, d-double
int vsrpc_call_ex(vsrpc_t *rpc, const char *list, ...)
{
  va_list ap;
  int argc, i;
  char **argv, c, *s;
  if (list == (char*) NULL) return VSRPC_ERR_BARG; // some check
  argc = strlen(list); // very easy!
  if (argc == 0) return VSRPC_ERR_BARG; // check argument

  // malloc argv array
  argv = (char**) vsrpc_malloc ((argc + 1) * (int)sizeof(char*));
  if (argv == (char**) NULL) return VSRPC_ERR_MEM; // memory exeption

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
    else s = vsrpc_str2str(""); // ?!
    argv[i] = s;
  }
  argv[argc] = (char*) NULL; // place to end
  va_end(ap);

  // send procedure and argument(s) to remote machine
  i = vsrpc_call(rpc, argv);

  // free memory from argument list
  vsrpc_free_argv(argv);

  return i;
}
//----------------------------------------------------------------------------
// check is procedure finished or run
int vsrpc_check(vsrpc_t *rpc)
{
  // check state
  if (rpc->state == VSRPC_STOP || rpc->state == VSRPC_LISTEN)
    // sory procedure not run yet
    return VSRPC_ERR_NRUN; // abnormal return

  // listen another part, wait return value(s) or error message
  return vsrpc_run(rpc);
}
//----------------------------------------------------------------------------
// wait until procedure run
// while procedure runing on another part may callback procedure
int vsrpc_wait(vsrpc_t *rpc)
{
  int retv;
  while (1)
  {
    if (vsrpc_select(rpc, VSRPC_SELECT_TIMEOUT) == 0) continue;
    if ((retv = vsrpc_check(rpc)) != VSRPC_ERR_EMPTY) return retv;
  }
}
//----------------------------------------------------------------------------
// check for nonblock call of vsrpc_read_one()
// return {1:true, 0:false, -1:error}
int vsrpc_select(vsrpc_t *rpc, int msec)
{
  int i;
  if (rpc->inbuf_cnt > rpc->inbuf_len) return 1;
  if (rpc->select == (int (*)(int, int)) NULL) return 1;
  i = rpc->select(rpc->fd_rd, msec); // check for nonblock read
  if (i < 0) return -1; // error
  if (i > 0) return 1;  // true
  return 0;             // false
}
//----------------------------------------------------------------------------
// read from pipe (may be read from input bufer)
// return ErrorCode: VSRPC_ERR_NONE | VSRPC_ERR_EOP
// nubmer of read bytes: count = size or count < size
int vsrpc_read_one(vsrpc_t *rpc, char *ptr, int size, int *count)
{
  int relict;
  char *src, *dst;

  // check argument
  if (size <= 0)
  { // bad argument
    *count = 0;
    return VSRPC_ERR_NONE;
  }

  // check text input bufer
  if ((relict = rpc->inbuf_cnt - rpc->inbuf_len) > 0)
  { // some bytes already in bufer
    if (size > relict) size = relict;
    memcpy((void*) ptr,
           (const void*) (rpc->inbuf_ptr + rpc->inbuf_len), (size_t) size);
    relict -= size;
    dst = rpc->inbuf_ptr + rpc->inbuf_len;
    src = dst + size;
    while (relict-- != 0) *dst++ = *src++;
    rpc->inbuf_cnt -= size;
    *count = size;
    return VSRPC_ERR_NONE;
  }

  // try read from pipe
  size = rpc->read(rpc->fd_rd, (void*) ptr, size);
  if (size <= 0)
  { // end of pipe or error
    *count = 0;
    return VSRPC_ERR_EOP;
  }
  *count = size;
  return VSRPC_ERR_NONE;
}
//----------------------------------------------------------------------------
// read block from pipe at once (may be read from input bufer)
// return ErrorCode: VSRPC_ERR_NONE | VSRPC_ERR_EOP
int vsrpc_read(vsrpc_t *rpc, char *ptr, int size)
{
  int len, relict;
  char *src, *dst;

  // check argument
  if (size <= 0)
    return VSRPC_ERR_NONE; // bad argument

  // check text input buffer
  if ((relict = rpc->inbuf_cnt - rpc->inbuf_len) > 0)
  { // some bytes already in buffer
    len = relict;
    if (len > size) len = size;
    memcpy((void*) ptr,
           (const void*) (rpc->inbuf_ptr + rpc->inbuf_len), (size_t) len);
    relict -= len;
    dst = rpc->inbuf_ptr + rpc->inbuf_len;
    src = dst + len;
    while (relict-- != 0) *dst++ = *src++;
    rpc->inbuf_cnt -= len;
    size -= len;
    ptr += len;
  }

  while (size > 0)
  { // read from pipe
    len = rpc->read(rpc->fd_rd, (void*) ptr, size);
    if (len <= 0) return VSRPC_ERR_EOP; // end of pipe or error
    size -= len;
    ptr += len;
  }
  return VSRPC_ERR_NONE;
}
//----------------------------------------------------------------------------
// write block to pipe at once
// return ErrorCode: VSRPC_ERR_NONE | VSRPC_ERR_EOP
int vsrpc_write(vsrpc_t *rpc, const char *ptr, int size)
{
  while (size > 0)
  {
    int len = rpc->write(rpc->fd_wr, (void*) ptr, size);
    if (len <= 0) return VSRPC_ERR_EOP;
    size -= len;
    ptr += len;
  }
  return VSRPC_ERR_NONE;
}
//----------------------------------------------------------------------------
// get string from input pipe to input bufer
// may return empty string, check rpc->inbuf_len!
// return ErrorCode: VSRPC_ERR_NONE | VSRPC_ERR_EOP | VSRPC_ERR_EMPTY |
//                   VSRPC_ERR_MEM | VSRPC_ERR_OVER
int vsrpc_gets_in(vsrpc_t* rpc)
{
  int retv, i;

  // Note:
  // 0 <= inbuf_len <= inbuf_cnt <= inbuf_size < inbuf_max_size

  // rotate inbuf (remove previous message)
  if (rpc->inbuf_len != 0)
  {
    rpc->inbuf_cnt -= rpc->inbuf_len;
    i = rpc->inbuf_cnt;
    if (i > 0)
    {
      char *src = rpc->inbuf_ptr + rpc->inbuf_len;
      char *dst = rpc->inbuf_ptr;
      do
        *dst++ = *src++;
      while (--i != 0);
    }
    rpc->inbuf_len = 0;
  }
  // Note:
  // 0 = inbuf_len <= inbuf_cnt <= inbuf_size < inbuf_max_size

  while (1)
  {
    // find '\n' or '\r' in input bufer
    for (i = 0; i < rpc->inbuf_cnt; i++)
    {
      if (strchr(VSRPC_EOL, rpc->inbuf_ptr[i]) != (char*) NULL)
      { // end of line symbol receive
        rpc->inbuf_ptr[i] = '\0'; // replace '\n' to '\0'
        rpc->inbuf_len = ++i;
        return VSRPC_ERR_NONE; // line in bufer ok
      }
    }

    // (re)allocate memory from bufer
    i = (rpc->inbuf_cnt + (VSRPC_INBUF_SECTOR*3/2)) / VSRPC_INBUF_SECTOR;
    i *= VSRPC_INBUF_SECTOR; // i = "recomented" size
    if (i != rpc->inbuf_size)
    { // reallocate more or less
      char *dst;
      if (i > rpc->inbuf_max_size)
        return VSRPC_ERR_OVER; // input bufer too big
      dst = vsrpc_malloc(i);
      if (dst == (char*) NULL)
        return VSRPC_ERR_MEM; // memory exeption
      if (rpc->inbuf_size != 0)
      {
        if (rpc->inbuf_cnt != 0)
          memcpy((void*) dst, (const void*) rpc->inbuf_ptr,
                 (size_t) rpc->inbuf_cnt);
        vsrpc_free(rpc->inbuf_ptr);
      }
      rpc->inbuf_ptr = dst;
      rpc->inbuf_size = i; // store new size of input bufer
    }

    // read block from input pipe
    if (rpc->select != (int (*)(int, int)) NULL)
      if (rpc->select(rpc->fd_rd, 0) == 0) // check for nonblock read
        return VSRPC_ERR_EMPTY; // empty pipe (no data)
    i = rpc->inbuf_size - rpc->inbuf_cnt; // maximum bytes may read
    retv = rpc->read(rpc->fd_rd, (void*) (rpc->inbuf_ptr + rpc->inbuf_cnt), i);
    if (retv <= 0)
      return VSRPC_ERR_EOP; // end of pipe (disconnect)
    rpc->inbuf_cnt += retv; // some bytes are read
  } // while (1)

}
//----------------------------------------------------------------------------
// get string from input pipe to input bufer
// skip empty string!
// return ErrorCode: VSRPC_ERR_NONE | VSRPC_ERR_EOP | VSRPC_ERR_EMPTY |
//                   VSRPC_ERR_MEM | VSRPC_ERR_OVER
int vsrpc_gets(vsrpc_t* rpc)
{
  int retv;
  do
    if ((retv = vsrpc_gets_in(rpc)) != VSRPC_ERR_NONE)
      return retv;
  while (rpc->inbuf_len <= 1); // repeat while len=0
  return retv;
}
//----------------------------------------------------------------------------
//// put string to output pipe
//// return ErrorCode: VSRPC_ERR_NONE | VSRPC_ERR_EOP
//int vsrpc_puts(vsrpc_t *rpc, const char *str)
//{
//  int size = strlen(str);
//  return vsrpc_write(rpc, str, size);
//}
//----------------------------------------------------------------------------
// put string to output pipe with VSRPC_CR code on the end
// return ErrorCode: VSRPC_ERR_NONE | VSRPC_ERR_EOP | VSRPC_ERR_MEM
int vsrpc_putsn(vsrpc_t *rpc, const char *str)
{
  int retv, size = strlen(str);
  char *buf = vsrpc_malloc(size + 1);
  if (buf == (char*) NULL) return VSRPC_ERR_MEM;
  memcpy((void*) buf, (const void*) str, (size_t) size);
  buf[size++] = VSRPC_CR;
  retv = vsrpc_write(rpc, buf, size); // write all at once
  vsrpc_free(buf);
  return retv;
}
//----------------------------------------------------------------------------
// return value(s)
// put string like this: "ret [RetVal1] [RetVal2] ...\n"
// return ErrorCode: VSRPC_ERR_NONE | VSRPC_ERR_EOP | VSRPC_ERR_MEM
int vsrpc_return_value(vsrpc_t *rpc, char * const retv[])
{
  char *str, *buf;
  int cnt, bsize, size = sizeof(VSRPC_RETURN_KEYWORD) - 1;

  cnt = ((str = vsrpc_argv2str(retv)) != (char*) NULL) ? strlen(str) + 1 : 0;
  buf = vsrpc_malloc(bsize = size + cnt + 1);
  if (buf == (char*) NULL) return VSRPC_ERR_MEM;
  memcpy((void*) buf, (const void*) VSRPC_RETURN_KEYWORD, (size_t) size);
  if (cnt != 0)
  {
    buf[size] = VSRPC_SPACE;
    memcpy((void*) &buf[size + 1], (const void*) str, (size_t) (cnt - 1));
    vsrpc_free(str);
  }
  buf[bsize - 1] = VSRPC_CR;

  cnt = vsrpc_write(rpc, buf, bsize);
  vsrpc_free(buf);

  if (cnt == VSRPC_ERR_NONE) // flush output pipe
    if (rpc->flush != (void (*)(int)) NULL) rpc->flush(rpc->fd_wr);

  return cnt;
}
//----------------------------------------------------------------------------
// return error code
// put string like this: "error ErrorCode\n"
// return ErrorCode: VSRPC_ERR_NONE | VSRPC_ERR_EOP | VSRPC_ERR_MEM
int vsrpc_return_error(vsrpc_t *rpc, int error)
{
  char *str, *buf;
  int cnt, bsize, size = sizeof(VSRPC_ERROR_KEYWORD) - 1;

  if ((str = vsrpc_int2str(error)) == (char*) NULL) return VSRPC_ERR_MEM;
  cnt = strlen(str) + 1;
  buf = vsrpc_malloc(bsize = size + cnt + 1);
  if (buf == (char*) NULL) return VSRPC_ERR_MEM;
  memcpy((void*) buf, (const void*) VSRPC_ERROR_KEYWORD, (size_t) size);
  buf[size] = VSRPC_SPACE;
  memcpy((void*) &buf[size + 1], (const void*) str, (size_t) (cnt - 1));
  vsrpc_free(str);
  buf[bsize - 1] = VSRPC_CR;

  cnt = vsrpc_write(rpc, buf, bsize);
  vsrpc_free(buf);

  if (cnt == VSRPC_ERR_NONE) // flush output pipe
    if (rpc->flush != (void (*)(int)) NULL) rpc->flush(rpc->fd_wr);

  return cnt;
}
//----------------------------------------------------------------------------
// receive array from pipe
// format: 0-char(binary), 1-int(text), 2-float(text), 3-double(text)
// return ErrorCode: VSRPC_ERR_NONE | VSRPC_ERR_EOP | VSRPC_ERR_MEM |
//                   VSRPC_ERR_OVER
int vsrpc_receive(vsrpc_t *rpc, const void *ptr, int size,
                  int format, int *count)
{
  int retv = VSRPC_ERR_BARG, cnt = 0;
  int *iptr;
#ifdef VSRPC_FLOAT
  float *fptr; double *dptr;
#endif // VSRPC_FLOAT

  switch (format)
  {
    case 0: // binary (as-is)
      retv = vsrpc_read(rpc, (char*) ptr, cnt = size);
      break;
    case 1: // integer (text)
      iptr = (int*) ptr;
      while (size-- != 0)
      {
        do
          retv = vsrpc_gets(rpc);
        while (retv == VSRPC_ERR_EMPTY);
        if (retv != VSRPC_ERR_NONE) break;
        *iptr++ = vsrpc_str2int(rpc->inbuf_ptr);
        cnt++;
      }
      break;
#ifdef VSRPC_FLOAT
    case 2: // float (text)
      fptr = (float*) ptr;
      while (size-- != 0)
      {
        do
          retv = vsrpc_gets(rpc);
        while (retv == VSRPC_ERR_EMPTY);
        if (retv != VSRPC_ERR_NONE) break;
        *fptr++ = (float) vsrpc_str2double(rpc->inbuf_ptr);
        cnt++;
      }
      break;
    case 3: // double (text)
      dptr = (double*) ptr;
      while (size-- != 0)
      {
        do
          retv = vsrpc_gets(rpc);
        while (retv == VSRPC_ERR_EMPTY);
        if (retv != VSRPC_ERR_NONE) break;
        *dptr++ = vsrpc_str2double(rpc->inbuf_ptr);
        cnt++;
      }
      break;
#endif // VSRPC_FLOAT
  } // switch
  *count = cnt;

  return retv;
}
//----------------------------------------------------------------------------
// transmit array to pipe
// return ErrorCode: VSRPC_ERR_NONE | VSRPC_ERR_EOP | VSRPC_ERR_MEM
// format: 0-char(binary), 1-int(text), 2-float(text), 3-double(text)
int vsrpc_transmit(vsrpc_t *rpc, const void *ptr, int size,
                   int format, int *count)
{
  int retv = VSRPC_ERR_BARG, cnt = 0;
  char *str;
  int *iptr;
#ifdef VSRPC_FLOAT
  float *fptr; double *dptr;
#endif // VSRPC_FLOAT

  switch (format)
  {
    case 0: // binary (as-is)
      retv = vsrpc_write(rpc, (char*) ptr, cnt = size);
      break;
    case 1: // integer (text)
      iptr = (int*) ptr;
      while (size-- != 0)
      {
        str = vsrpc_int2str(*iptr++);
        retv = vsrpc_putsn(rpc, str);
        vsrpc_free(str);
        if (retv != VSRPC_ERR_NONE) break;
        cnt++;
      }
      break;
#ifdef VSRPC_FLOAT
    case 2: // float (text)
      fptr = (float*) ptr;
      while (size-- != 0)
      {
        str = vsrpc_float2str(*fptr++);
        retv = vsrpc_putsn(rpc, str);
        vsrpc_free(str);
        if (retv != VSRPC_ERR_NONE) break;
        cnt++;
      }
      break;
    case 3: // double (text)
      dptr = (double*) ptr;
      while (size-- != 0)
      {
        str = vsrpc_double2str(*dptr++);
        retv = vsrpc_putsn(rpc, str);
        vsrpc_free(str);
        if (retv != VSRPC_ERR_NONE) break;
        cnt++;
      }
      break;
#endif // VSRPC_FLOAT
  } // switch
  *count = cnt;

  return retv;
}
//----------------------------------------------------------------------------
// pack string ('\ ', '\t', '\\', '\n', '\r')
char *vsrpc_pack_str(const char *str)
{
  char c, *src, *dst, *out;
  int cnt, empty = 0;

  if (str == (char*) NULL) return (char*) NULL; // bad argument

  // calculate size of output string
  cnt = 0;
  src = (char*)str;
  do {
    c = *src++;
    if (strchr(" \t\\\n\r", c) != (char*) NULL && c != '\0')
      cnt++;
    cnt++;
  } while (c != '\0');

  // check if string is empty and cnt = 1
  if (cnt == 1)
    // empty string is pack specially as "\0"
    empty = cnt = 3;

  // allocate memory for output string
  if ((dst = out = vsrpc_malloc(cnt)) == (char*) NULL)
    return (char*) NULL; // memory exeption

  if (empty != 0)
  { // pack empty string
    *dst++ = '\\';  *dst++= '0'; *dst = '\0';
    return out;
  }

  // pack input string to output
  src = (char*)str;
  do {
    c = *src++;
    if (c == ' ' || c == '\\') *dst++ = '\\';
    else if (c == '\t') { *dst++ = '\\'; c = 't'; }
    else if (c == '\n') { *dst++ = '\\'; c = 'n'; }
    else if (c == '\r') { *dst++ = '\\'; c = 'r'; }
    *dst++ = c;
  } while (c != '\0');

  return out;
}
//----------------------------------------------------------------------------
// unpack string ('\ ', '\t', '\\', '\n', '\r', '\0')
char *vsrpc_unpack_str(const char *str)
{
  char c, p, *src, *dst, *out;
  int cnt;

  if (str == (char*) NULL) return (char*) NULL; // bad argument

  // check if string was empty before packing
  if (strcmp(str, "\\0") == 0)
  { // output string must be empty
    if ((out = vsrpc_malloc(1)) == (char*) NULL)
      return (char*) NULL; // abnormal return
    *out = '\0';
    return out; // return empty string with '\0'
  }

  // calculate size of output string
  cnt = 0;
  src = (char*)str;
  p = '\0';
  do {
    c = *src++;
    if (c != '\\' || p == '\\')
      cnt++;
    p = c;
  } while (c != '\0');

  // allocate memory for output string
  if ((dst = out = vsrpc_malloc(cnt)) == (char*) NULL)
    return (char*) NULL; // abnormal return

  // unpack input string to output
  src = (char*)str;
  do {
    c = *src++;
    if (c == '\\')
    {
      c = *src++;
      if      (c == 't') c = '\t';
      else if (c == 'n') c = '\n';
      else if (c == 'r') c = '\r';
      else if (c == '0') c = '\0';
      // else "as-is" "\ " -> " ", "\b" -> "b"
    }
    *dst++ = c;
  } while (c != '\0');

  return out;
}
//----------------------------------------------------------------------------
// pack one string from argv argument list
// return pointer to string or NULL on failure
char *vsrpc_argv2str(char * const argv[])
{
  int i, len, argc;
  char **argv_local, *out, *dst;

  if ((argc = vsrpc_argv2argc(argv)) == 0)
    return (char*) NULL; // bad argument

  // allocate memory for local argv list
  argv_local = (char**) vsrpc_malloc ((argc + 1) * (int)sizeof(char*));
  if (argv_local == (char**) NULL)
    return (char*) NULL; // memory exeption

  // pack each string
  for (i = 0; i < argc; i++)
    if ((argv_local[i] = vsrpc_pack_str(argv[i])) == (char*) NULL)
    { // memory exeption
      vsrpc_free_argv(argv_local);
      return (char*) NULL; // abnormal return
    }
  argv_local[argc] = (char*) NULL; // place to end

  // caclulate size of output string
  len = argc; // ' ' for each argument and '\0' at the end
  for (i = 0; i < argc; i++)
    len += (int) strlen(argv_local[i]);

  // allocate memory for output string
  if ((out = vsrpc_malloc(len)) == (char*) NULL)
  { // memory exeption
    vsrpc_free_argv(argv_local);
    return (char*) NULL; // abnormal return
  }

  // copy all argument to output string
  dst = out;
  for (i = 0; i < argc; i++)
  {
    len = strlen(argv_local[i]);
    memcpy((void*) dst, (const void*) argv_local[i], (size_t) len);
    dst += len;
    *dst++ = ' '; // separator
  }
  *--dst = '\0'; // zero terminate

  vsrpc_free_argv(argv_local);
  return out;
}
//----------------------------------------------------------------------------
// unpack argv argument list from one string
// return argv or NULL on failure
char **vsrpc_str2argv(const char *str)
{
  int argc; // argument index/counter
  char **argv; // return value
  int state; // 0-find_first, 1-find_last
  char c, p, *src, *dst, *ptr;

  // count argc
  argc = state = 0;
  src = (char*) str;
  p = '\0'; // init previous char
  do {
    c = *src++;
    if ( (strchr(VSRPC_DELIMETERS, (int) c) == (char*) NULL || p == '\\')
          && c != '\0' )
    { // current symbol is not delimeter and is not end of string
      if (state == 0) state = 1;
    }
    else
    { // current symbol is delimeter or end of string
      if (state != 0) { state = 0; argc++; }
    }
    p = c; // store previous char
  } while (c != '\0');

  if (argc == 0)
    return (char**) NULL; // bad argument

  // malloc argv array
  argv = (char**) vsrpc_malloc ((argc + 1) * (int)sizeof(char*));
  if (argv == (char**) NULL) return (char**) NULL; // memory exeption

  // fill argv
  argc = state = 0;
  src = ptr = (char*) str;
  do {
    c = *src;
    if ( (strchr(VSRPC_DELIMETERS, (int)c) == (char*) NULL || p == '\\')
          && c != '\0' )
    { // current symbol is not delimeter and is not end of string
      if (state == 0) { state = 1; ptr = src; }
    }
    else
    { // current symbol is delimeter or end of string
      if (state != 0)
      {
        state = (int) (src - ptr);
        dst = vsrpc_malloc(state + 1);
        if (dst != (char*) NULL)
        {
          memcpy((void*) dst, (const void*) ptr, (size_t) state);
          dst[state] = '\0';
          ptr = vsrpc_unpack_str(dst); // unpack argument
          vsrpc_free(dst);
          dst = ptr;
        }
        if ((argv[argc++] = dst) == (char*) NULL)
        { // memory exeption
          vsrpc_free_argv(argv);
          return (char**) NULL; // abnormal return
        }
        state = 0;
      }
    }
    src++;
    p = c; // store previous char
  } while (c != '\0');

  argv[argc] = (char*) NULL; // place to end
  return argv;
}
//----------------------------------------------------------------------------
// create argv argument list from variable list
// list: s-string, i-integer, f-float, d-double
// return argv or NULL on failure
char **vsrpc_list2argv(const char *list, ...)
{
  va_list ap;
  int argc, i;
  char **argv, c, *s;

  if (list == (char*) NULL) return (char**) NULL; // some check
  argc = strlen(list); // very easy!
  if (argc == 0) return (char**) NULL; // check argument

  // malloc argv array
  argv = (char**) vsrpc_malloc ((argc + 1) * (int)sizeof(char*));
  if (argv == (char**) NULL) return (char**) NULL; // memory exeption

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
    else s = vsrpc_str2str(""); // ?!
    argv[i] = s;
  }
  argv[argc] = (char*) NULL; // place to end
  va_end(ap);

  return argv;
}
//----------------------------------------------------------------------------
// free memory from argv strings
void vsrpc_free_argv(char *argv[])
{
  char **p = (char**) argv, *s;
  if (argv == (char**) NULL) return;
  while ((s = *p++) != (char*) NULL)
    vsrpc_free(s);
  vsrpc_free((char*) argv);
}
//----------------------------------------------------------------------------
// get argc of argv
int vsrpc_argv2argc(char * const argv[])
{
  char **p = (char**)argv;
  int argc = 0;
  if (argv == (char**) NULL) return 0; // bad argument
  while ((*p++) != (char*) NULL) argc++;
  return argc;
}
//----------------------------------------------------------------------------
// shift argv
// return pointer to shifted argv or NULL on failure
char **vsrpc_shift_argv(char * const argv[])
{
  int argc, i, len;
  char **retv, *str;

  // count input arguments
  if ((argc = vsrpc_argv2argc(argv)) < 2)
    return (char**) NULL; // bad argument

  // allocate memory
  retv = (char**) vsrpc_malloc(argc * (int)sizeof(char*));
  if (retv == (char**) NULL)
    return (char**) NULL; // memory exeption

  // copy arguments begin from second
  for (i = 1; i < argc; i++)
  {
    len = strlen(argv[i]) + 1;
    if ((str = vsrpc_malloc(len)) == (char*) NULL)
    { // memory exeption
      vsrpc_free_argv(retv);
      return (char**) NULL;
    }
    retv[i - 1] = (char*) memcpy((void*) str, (const void*) argv[i],
                                 (size_t) len);
  }
  retv[i - 1] = (char*) NULL; // place to end

  return retv;
}
//----------------------------------------------------------------------------
// convert integer value to string
char *vsrpc_int2str(int val)
{
  char str[VSRPC_SNPRINTF_BUF], *out;
#ifdef VSWIN32
  int len = _snprintf_s(str, VSRPC_SNPRINTF_BUF, VSRPC_SNPRINTF_BUF, "%i", val);
#else // VSWIN32
  int len = snprintf(str, VSRPC_SNPRINTF_BUF, "%i", val);
#endif // VSWIN32
  if (len < 0 || len > VSRPC_SNPRINTF_BUF)
    return (char*) NULL; // litle bufer, sorry
  if ((out = vsrpc_malloc(++len)) == (char*) NULL)
    return (char*) NULL; // memory exeption
  return (char*) memcpy((void*) out, (const void*) str, (size_t) len);
}
//----------------------------------------------------------------------------
#ifdef VSRPC_FLOAT
// convert float value to string
char *vsrpc_float2str(float val)
{
  char str[VSRPC_SNPRINTF_BUF], *out;
#ifdef VSWIN32
  int len = _snprintf_s(str, VSRPC_SNPRINTF_BUF, VSRPC_SNPRINTF_BUF, "%.8g", val);
#else // VSWIN32
  int len = snprintf(str, VSRPC_SNPRINTF_BUF, "%.8g", val);
#endif // VSWIN32
  if (len < 0 || len > VSRPC_SNPRINTF_BUF)
    return (char*) NULL; // litle bufer, sorry
  if ((out = vsrpc_malloc(++len)) == (char*) NULL)
    return (char*) NULL; // memory exeption
  return (char*) memcpy((void*) out, (const void*) str, (size_t) len);
}
//----------------------------------------------------------------------------
// convert double value to string
char *vsrpc_double2str(double val)
{
  char str[VSRPC_SNPRINTF_BUF], *out;
#ifdef VSWIN32
  int len = _snprintf_s(str, VSRPC_SNPRINTF_BUF, VSRPC_SNPRINTF_BUF, "%.16g", val);
#else // VSWIN32
  int len = snprintf(str, VSRPC_SNPRINTF_BUF, "%.16g", val);
#endif // VSWIN32
  if (len < 0 || len > VSRPC_SNPRINTF_BUF)
    return (char*) NULL; // litle bufer, sorry
  if ((out = vsrpc_malloc(++len)) == (char*) NULL)
    return (char*) NULL; // memory exeption
  return (char*) memcpy((void*) out, (const void*) str, (size_t) len);
}
#endif // VSRPC_FLOAT
//----------------------------------------------------------------------------
// create dynamic new copy of string
char *vsrpc_str2str(char *str)
{
  char *out;
  int len;
  if (str == (char*) NULL) return (char*) NULL; // argument error
  len = strlen(str) + 1;
  if ((out = vsrpc_malloc(len)) == (char*) NULL)
    return (char*) NULL; // memory exeption
  return (char*) memcpy((void*) out, (const void*) str, (size_t) len);
}
//----------------------------------------------------------------------------
#ifdef VSRPC_EXTRA
// add some string(s) to one
char *vsrpc_stradd(int n, ...)
{
  va_list ap;
  char **strs, *str;
  int i, *lens, len;

  if (n <= 0) return (char*) NULL;
  if ((strs = (char**) vsrpc_malloc(n)) == (char**) NULL)
    return (char*) NULL;
  if ((lens = (int*) vsrpc_malloc(n)) == (int*) NULL)
  { vsrpc_free((char*) strs); return (char*) NULL; }

  va_start(ap, n);
  for (len = i = 0; i < n; i++)
  {
    if ((str = va_arg(ap, char*)) != (char*) NULL)
    {
      strs[i] = str;
      len += (lens[i] = strlen(str)); // count sum length
    }
    else lens[i] = 0;
  }
  va_end(ap);

  if ((str = vsrpc_malloc(len)) == (char*) NULL)
  {
    vsrpc_free((char*) strs);
    vsrpc_free((char*) lens);
    return (char*) NULL;
  }

  for (len = i = 0; i < n; i++)
  {
    if (lens[i] == 0) continue;
    memcpy((void*) (str + len), (const void*) strs[i], (size_t) lens[i]);
    len += lens[i];
  }

  vsrpc_free((char*) strs);
  vsrpc_free((char*) lens);
  str[len] = '\0';
  return str;
}
//----------------------------------------------------------------------------
// convert binary array to hex string (alya "01:23:AB:CD")
char *vsrpc_bin2hex(const char *bin, int len)
{
  char c, *src, *dst, *out;
  unsigned char val;

  src = (char*)bin;
  if ((dst = out = vsrpc_malloc(len * 3)) == (char*) NULL)
    return (char*) NULL; // memory exeption

  while (len-- != 0)
  {
    val = *src++;
    c = (char)(val >> 4) + '0';
    if (c > '9') c += 'A' - '9' - 1;
    *dst++ = c;
    c = (char)(val & 0x0F) + '0';
    if (c > '9') c += 'A' - '9' - 1;
    *dst++ = c;
    *dst++ = ':';
  }
  *--dst = '\0'; // terminate output string

  return out;
}
//----------------------------------------------------------------------------
// convert hex string (alya "01:23:AB:CD") to binary array
char *vsrpc_hex2bin(const char *str)
{
  char h, l, *src, *dst, *out;
  int len;

  len = strlen(src = (char*)str) + 1;
  if ((len % 3) != 0)
    return (char*) NULL; // syntax error ???
  len /= 3;

  if ((dst = out = vsrpc_malloc(len)) == (char*) NULL)
    return (char*) NULL; // memory exeption

  while (len-- != 0)
  {
    h = *src++;
    if (     h >= 'A' && h <= 'Z') h -= 'A' - '9' - 1;
    else if (h >= 'a' && h <= 'z') h -= 'a' - '9' - 1;
    h -= '0';
    h <<= 4;
    l = *src++;
    if (     l >= 'A' && l <= 'Z') l -= 'A' - '9' - 1;
    else if (l >= 'a' && l <= 'z') l -= 'a' - '9' - 1;
    l -= '0';
    *dst++ = h | l;
    h = *src++;
    if (h != ':' && h != '\0')
    {
      vsrpc_free(out);
      return (char*) NULL; // syntax error
    }
  }

  return out;
}
#endif // VSRPC_EXTRA
//============================================================================
// create new common memory block
//    input: Size
//   return: ErrorCode [MemoryBlockID]
//   errors: VSRPC_ERR_NONE | VSRPC_ERR_PERM | VSRPC_ERR_MEM |
//           VSRPC_ERR_BARG | VSRPC_ERR_NFMB | VSRPC_ERR_TBMB
char **vsrpc_builtin_malloc(vsrpc_t *rpc, int argc, char * const argv[])
{
  int size, id;

  // check permission
  if ((rpc->perm & VSRPC_PERM_MALLOC) == 0)
    return vsrpc_list2argv("i", VSRPC_ERR_PERM);

  // check argument
  if (argc != 2) return vsrpc_list2argv("i", VSRPC_ERR_BARG);
  size = vsrpc_str2int(argv[1]);

  size = vsrpc_local_malloc(rpc, size, &id);
  if (size != VSRPC_ERR_NONE) return vsrpc_list2argv("i", size);

  // all right -> return MemoryBlockID
  return vsrpc_list2argv("ii", VSRPC_ERR_NONE, id);
}
//----------------------------------------------------------------------------
int vsrpc_local_malloc(vsrpc_t *rpc, int size, int *id) // return ErrorCode
{
  int i;

  // check argument
  if (size <= 0) return VSRPC_ERR_BARG; // bad argument
  if (size > rpc->mb_max_size) return VSRPC_ERR_TBMB; // size too big

  // find free descriptor
  for (i = 0; i < VSRPC_MB_NUM; i++)
    if (rpc->mb[i].size == 0) break;
  if (i == VSRPC_MB_NUM)
    return VSRPC_ERR_NFMB; // no free descriptor

  // try to allocate memory
  if ((rpc->mb[i].ptr = vsrpc_malloc(size)) == (char*) NULL)
    return VSRPC_ERR_MEM; // memory exeption

  // mark descriptor of common memory block as busy (store size)
  rpc->mb[i].size = size;
  *id = i; // all right -> return MemoryBlockID

  return VSRPC_ERR_NONE;
}
//----------------------------------------------------------------------------
int vsrpc_remote_malloc(vsrpc_t *rpc, int size, int *id) // return ErrorCode
{
  int retv;

  // call procedure on remote machine
  retv = vsrpc_call_ex(rpc, "si", "malloc", size);
  if (retv != VSRPC_ERR_NONE) return retv;

  // wait when procedure return result
  retv = vsrpc_wait(rpc);
  if (retv != VSRPC_ERR_NONE) return retv;

  // check and return result
  if (rpc->retc == 1)
    retv = vsrpc_str2int(*rpc->retv);
  else if (rpc->retc == 2)
  {
    retv = vsrpc_str2int(*rpc->retv);
    //printf("<%s>\n", rpc->retv[1]);
    *id = vsrpc_str2int(rpc->retv[1]); // MemoryBlockID
  }
  else
    retv = VSRPC_ERR_PROT; // protocol error

  if (rpc->retc != 0){ vsrpc_free_argv(rpc->retv); rpc->retc = 0; }
  return retv;
}
//----------------------------------------------------------------------------
// free used common memory block
//    input: MemoryBlockID
//   return: ErrorCode
//   errors: VSRPC_ERR_NONE | VSRPC_ERR_PERM | VSRPC_ERR_BARG
char **vsrpc_builtin_free(vsrpc_t *rpc, int argc, char * const argv[])
{
  int id;

  // check permission
  if ((rpc->perm & VSRPC_PERM_FREE) == 0)
    return vsrpc_list2argv("i", VSRPC_ERR_PERM);

  // check argument
  if (argc != 2) return vsrpc_list2argv("i", VSRPC_ERR_BARG);
  id = vsrpc_str2int(argv[1]);

  return vsrpc_list2argv("i", vsrpc_local_free(rpc, id));
}
//----------------------------------------------------------------------------
int vsrpc_local_free(vsrpc_t *rpc, int id) // return ErrorCode
{
  // check argument
  if (id < 0 || id >= VSRPC_MB_NUM) return VSRPC_ERR_BARG;
  if (rpc->mb[id].size == 0)        return VSRPC_ERR_BARG;
  if (rpc->mb[id].stat != 0)        return VSRPC_ERR_BARG;

  // free memory block
  vsrpc_free(rpc->mb[id].ptr);
  rpc->mb[id].size = 0; // mark as free

  return VSRPC_ERR_NONE;
}
//----------------------------------------------------------------------------
int vsrpc_remote_free(vsrpc_t *rpc, int id) // return ErrorCode
{
  int retv;

  // call procedure on remote machine
  retv = vsrpc_call_ex(rpc, "si", "free", id);
  if (retv != VSRPC_ERR_NONE) return retv;

  // wait when procedure return result
  retv = vsrpc_wait(rpc);
  if (retv != VSRPC_ERR_NONE) return retv;

  // check result
  if (rpc->retc == 1)
    retv = vsrpc_str2int(*rpc->retv);
  else
    retv = VSRPC_ERR_PROT; // protocol error

  if (rpc->retc != 0){ vsrpc_free_argv(rpc->retv); rpc->retc = 0; }
  return retv;
}
//----------------------------------------------------------------------------
// stat common memory block
//    input: MemoryBlockID
//   return: ErrorCode [Size(0 - if unused)] [Stat(0-dynamic, 1-static)]
//   errors: VSRPC_ERR_NONE | VSRPC_ERR_BARG
char **vsrpc_builtin_stat(vsrpc_t *rpc, int argc, char * const argv[])
{
  int id, size, stat;

  // check argument
  if (argc != 2) return vsrpc_list2argv("i", VSRPC_ERR_BARG);
  id = vsrpc_str2int(argv[1]);

  id = vsrpc_local_stat(rpc, id, &size, &stat);
  if (id != VSRPC_ERR_NONE) return vsrpc_list2argv("i", id);

  // all right -> return size of MemoryBlock
  return vsrpc_list2argv("iii", VSRPC_ERR_NONE, size, stat);
}
//----------------------------------------------------------------------------
int vsrpc_local_stat(vsrpc_t *rpc, int id, int *size, int *stat)
{
  if (id < 0 || id >= VSRPC_MB_NUM) return VSRPC_ERR_BARG;
  *size = rpc->mb[id].size;
  *stat = rpc->mb[id].stat;
  return VSRPC_ERR_NONE;
}
//----------------------------------------------------------------------------
int vsrpc_remote_stat(vsrpc_t *rpc, int id, int *size, int *stat)
{
  int retv;

  // call procedure on remote machine
  retv = vsrpc_call_ex(rpc, "si", "stat", id);
  if (retv != VSRPC_ERR_NONE) return retv;

  // wait when procedure return result
  retv = vsrpc_wait(rpc);
  if (retv != VSRPC_ERR_NONE) return retv;

  // check and return result
  if (rpc->retc == 1)
    retv = vsrpc_str2int(*rpc->retv);
  else if (rpc->retc == 3)
  {
    retv = vsrpc_str2int(*rpc->retv);
    *size = vsrpc_str2int(rpc->retv[1]); // size of MemoryBlock
    *stat = vsrpc_str2int(rpc->retv[2]); // memory stat of MemoryBlock
  }
  else
    retv = VSRPC_ERR_PROT; // protocol error

  if (rpc->retc != 0){ vsrpc_free_argv(rpc->retv); rpc->retc = 0; }
  return retv;
}
//----------------------------------------------------------------------------
// read from common memory block
//    input: MemoryBlockID Offset Size Format
//   return: ErrorCode [Count]
//   errors: VSRPC_ERR_NONE | VSRPC_ERR_PERM | VSRPC_ERR_BARG |
//           VSRPC_ERR_MEM
//   format: 0-char(binary), 1-int(text), 2-float(text), 3-double(text)
char **vsrpc_builtin_read(vsrpc_t *rpc, int argc, char * const argv[])
{
  int id, off, size, fmt, max, retv, step;
  char **rv;

  // check permission
  if ((rpc->perm & VSRPC_PERM_READ) == 0)
    return vsrpc_list2argv("i", VSRPC_ERR_PERM);

  // check and prepare argument
  if (argc != 5) return vsrpc_list2argv("i", VSRPC_ERR_BARG);
  id = vsrpc_str2int(argv[1]);
  if (id < 0 || id >= VSRPC_MB_NUM)
    return vsrpc_list2argv("i", VSRPC_ERR_BARG);
  max = rpc->mb[id].size;
  if (max == 0) return vsrpc_list2argv("i", VSRPC_ERR_BARG);
  fmt = vsrpc_str2int(argv[4]);
  if (fmt < 0 || fmt > 3) return vsrpc_list2argv("i", VSRPC_ERR_BARG);
  off = vsrpc_str2int(argv[2]);

  step = sizeof(char);
  if      (fmt == 1) step = sizeof(int);
#ifdef VSRPC_FLOAT
  else if (fmt == 2) step = sizeof(float);
  else if (fmt == 3) step = sizeof(double);
#endif // VSRPC_FLOAT
  if (off < 0 ||
      off >= max) return vsrpc_list2argv("i", VSRPC_ERR_BARG);

  // prepare valid max size
  size = step * vsrpc_str2int(argv[3]);
  max -= off; // max size may read
  if (size > max) size = max; // real size
  size /= step;
  if (size <= 0) return vsrpc_list2argv("i", VSRPC_ERR_BARG);

  // tell to caller vailid transfer size (Count)
  if ((rv = vsrpc_list2argv("ii", VSRPC_ERR_NONE, size)) == (char**) NULL)
    return vsrpc_list2argv("i", VSRPC_ERR_MEM);
  if((retv = vsrpc_return_value(rpc, rv)) != VSRPC_ERR_NONE)
    return vsrpc_list2argv("i", retv);
  vsrpc_free_argv(rv);

  // now all preparing is finish => send requested array to client
  retv = vsrpc_transmit(rpc, (void*) (rpc->mb[id].ptr + off), size, fmt, &size);

  return vsrpc_list2argv("i", retv);
}
//----------------------------------------------------------------------------
int vsrpc_local_read(vsrpc_t *rpc,
                     void *ptr, // ptr - destination address
                     int id, int off, int size,
                     int *count) // return ErrorCode
{
  int max;

  // check arguments
  if (id < 0 || id >= VSRPC_MB_NUM) goto err_barg;
  max = rpc->mb[id].size;
  if (max == 0) goto err_barg;
  if (off < 0 || off >= max) goto err_barg;
  max -= off; // max size may read
  if (size > max) size = max; // real size

  // copy memory
  memcpy(ptr, (const void*) (rpc->mb[id].ptr + off), (size_t) size);
  *count = size;
  return VSRPC_ERR_NONE;

err_barg:
  *count = 0;
  return VSRPC_ERR_BARG;
}
//----------------------------------------------------------------------------
int vsrpc_remote_read(vsrpc_t *rpc,
                      void *ptr, // ptr - destination address
                      int id, int off, int size, int format,
                      int *count) // return ErrorCode
{
  int retv;
  vsrpc_state_t save_state;
  *count = 0;

  // call procedure on remote machine
  retv = vsrpc_call_ex(rpc, "siiii", "read", id, off, size, format);
  if (retv != VSRPC_ERR_NONE) return retv;

  // save state
  save_state = rpc->state; // VSRPC_CALL || VSRPC_CBACK

  // wait when procedure return first result
  retv = vsrpc_wait(rpc);
  if (retv != VSRPC_ERR_NONE) return retv;

  // first result must be vailid Cout
  if (rpc->retc == 2)
  {
    retv = vsrpc_str2int(*rpc->retv);
    size = vsrpc_str2int(rpc->retv[1]); // Count
  }
  else
    retv = VSRPC_ERR_PROT; // protocol error

  if (rpc->retc != 0){ vsrpc_free_argv(rpc->retv); rpc->retc = 0; }
  if (retv != VSRPC_ERR_NONE) return retv;

  // now all preparing is finish => receive requested array from server
  retv = vsrpc_receive(rpc, ptr, size, format, count);
  if (retv != VSRPC_ERR_NONE) return retv;

  // restore state
  rpc->state = save_state;

  // wait second result - finish mark
  retv = vsrpc_wait(rpc);
  if (retv != VSRPC_ERR_NONE) return retv;

  if (rpc->retc != 0)
  {
    retv = vsrpc_str2int(*rpc->retv);
    vsrpc_free_argv(rpc->retv); rpc->retc = 0;
  }

  return retv;
}
//----------------------------------------------------------------------------
// write to common memory block
//    input: MemoryBlockID Offset Size Format
//   return: ErrorCode [Count]
//   errors: VSRPC_ERR_NONE | VSRPC_ERR_PERM | VSRPC_ERR_BARG |
//           VSRPC_ERR_MEM
//   format: 0-char(binary), 1-int(text), 2-float(text), 3-double(text)
char **vsrpc_builtin_write(vsrpc_t *rpc, int argc, char * const argv[])
{
  int id, off, size, fmt, max, retv, step;
  char **rv;

  // check permission
  if ((rpc->perm & VSRPC_PERM_WRITE) == 0)
    return vsrpc_list2argv("i", VSRPC_ERR_PERM);

  // check argument
  if (argc != 5) return vsrpc_list2argv("i", VSRPC_ERR_BARG);
  id = vsrpc_str2int(argv[1]);
  if (id < 0 || id >= VSRPC_MB_NUM)
    return vsrpc_list2argv("i", VSRPC_ERR_BARG);
  max = rpc->mb[id].size;
  if (max == 0) return vsrpc_list2argv("i", VSRPC_ERR_BARG);
  fmt = vsrpc_str2int(argv[4]);
  if (fmt < 0 || fmt > 3) return vsrpc_list2argv("i", VSRPC_ERR_BARG);
  off = vsrpc_str2int(argv[2]);

  step = sizeof(char);
  if      (fmt == 1) step = (sizeof(int));
#ifdef VSRPC_FLOAT
  else if (fmt == 2) step = (sizeof(float));
  else if (fmt == 3) step = (sizeof(double));
#endif // VSRPC_FLOAT
  if (off < 0 ||
      off >= max) return vsrpc_list2argv("i", VSRPC_ERR_BARG);

  // prepare valid max size
  size = step * vsrpc_str2int(argv[3]);
  max -= off; // max size may read
  if (size > max) size = max;
  size /= step;
  if (size <= 0) return vsrpc_list2argv("i", VSRPC_ERR_BARG);

  // tell to caller vailid transfer size (Count)
  if ((rv = vsrpc_list2argv("ii", VSRPC_ERR_NONE, size)) == (char**) NULL)
    return vsrpc_list2argv("i", VSRPC_ERR_MEM);
  if((retv = vsrpc_return_value(rpc, rv)) != VSRPC_ERR_NONE)
    return vsrpc_list2argv("i", retv);
  vsrpc_free_argv(rv);

  // now all preparing is finish => receive requested array from client
  retv = vsrpc_receive(rpc, (void*) (rpc->mb[id].ptr + off), size, fmt, &size);

  return vsrpc_list2argv("i", retv);
}
//----------------------------------------------------------------------------
int vsrpc_local_write(vsrpc_t *rpc,
                      const void *ptr, // ptr - source address
                      int id, int off, int size,
                      int *count) // return ErrorCode
{
  int max;

  // check arguments
  if (id < 0 || id >= VSRPC_MB_NUM) goto err_barg;
  max = rpc->mb[id].size;
  if (max == 0) goto err_barg;
  if (off < 0 || off >= max) goto err_barg;
  max -= off; // max size may write
  if (size > max) size = max; // real size

  // copy memory
  memcpy((void*) (rpc->mb[id].ptr + off), ptr, (size_t) size);
  *count = size;
  return VSRPC_ERR_NONE;

err_barg:
  *count = 0;
  return VSRPC_ERR_BARG;
}
//----------------------------------------------------------------------------
int vsrpc_remote_write(vsrpc_t *rpc,
                       const void *ptr, // ptr - source address
                       int id, int off, int size, int fmt,
                       int *count) // return ErrorCode
{
  int retv;
  vsrpc_state_t save_state;
  *count = 0;

  // call procedure on remote machine
  retv = vsrpc_call_ex(rpc, "siiii", "write", id, off, size, fmt);
  if (retv != VSRPC_ERR_NONE) return retv;

  // save state
  save_state = rpc->state; // VSRPC_CALL || VSRPC_CBACK

  // wait when procedure return first result
  retv = vsrpc_wait(rpc);
  if (retv != VSRPC_ERR_NONE) return retv;

  // first result must be vailid Cout
  if (rpc->retc == 2)
  {
    retv = vsrpc_str2int(*rpc->retv);
    size = vsrpc_str2int(rpc->retv[1]); // Count
  }
  else
    retv = VSRPC_ERR_PROT; // protocol error

  if (rpc->retc != 0){ vsrpc_free_argv(rpc->retv); rpc->retc = 0; }
  if (retv != VSRPC_ERR_NONE) return retv;

  // now all preparing is finish => send array to server
  retv = vsrpc_transmit(rpc, (void*) ptr, size, fmt, count);
  if (retv != VSRPC_ERR_NONE) return retv;

  // flush output pipe
  if (rpc->flush != (void (*)(int)) NULL) rpc->flush(rpc->fd_wr);

  // restore state
  rpc->state = save_state;

  // wait second result - finish mark
  retv = vsrpc_wait(rpc);
  if (retv != VSRPC_ERR_NONE) return retv;

  if (rpc->retc != 0)
  {
    retv = vsrpc_str2int(*rpc->retv);
    vsrpc_free_argv(rpc->retv); rpc->retc = 0;
  }

  return retv;
}
//----------------------------------------------------------------------------
// simple "ping"
//    input: Nothing
//   return: ErrorCode "pong"
//   errors: VSRPC_ERR_NONE
char **vsrpc_builtin_ping(vsrpc_t *rpc, int argc, char * const argv[])
{
  // It's so funy, isn't it?
  return vsrpc_list2argv("is", VSRPC_ERR_NONE, VSRPC_PONG_KEYWORD);
}
//----------------------------------------------------------------------------
int vsrpc_remote_ping(vsrpc_t *rpc, int *ack) // return ErrorCode
{
  int retv;
  *ack = 0; // set if error

  // call procedure on remote machine
  retv = vsrpc_call_ex(rpc, "s", "ping");
  if (retv != VSRPC_ERR_NONE) return retv;

  // wait when procedure return result
  retv = vsrpc_wait(rpc);
  if (retv != VSRPC_ERR_NONE) return retv;

  // check result
  if (rpc->retc != 2)
  { // error of protocol
    if (rpc->retc != 0){ vsrpc_free_argv(rpc->retv); rpc->retc = 0; }
    return VSRPC_ERR_PROT;
  }

  retv = vsrpc_str2int(*rpc->retv);
  // check if "pong" returned
  if (strcmp(rpc->retv[1], VSRPC_PONG_KEYWORD) == 0) *ack = 1;

  vsrpc_free_argv(rpc->retv); rpc->retc = 0;
  return retv;
}
//----------------------------------------------------------------------------
// terminate connection
//    input: [ExitValue(s)]
//   return: ErrorCode
//   errors: VSRPC_ERR_PERM | VSRPC_ERR_EXIT
char **vsrpc_builtin_exit(vsrpc_t* rpc, int argc, char * const argv[])
{
  int retv;

  // check permission
  if ( (rpc->perm & VSRPC_PERM_EXIT) != 0 )
    retv = VSRPC_ERR_EXIT; // permission ok => exit from run cycle
  else
    retv = VSRPC_ERR_PERM; // permission denied

  if (argc > 1)
    rpc->exit_value = vsrpc_str2int(argv[1]);

  return vsrpc_list2argv("i", retv);
}
//----------------------------------------------------------------------------
int vsrpc_remote_exit(vsrpc_t *rpc, int retv) // return ErrorCode
{
  // call procedure on remote machine
  retv =  vsrpc_call_ex(rpc, "si", "exit", retv);
  if (retv != VSRPC_ERR_NONE) return retv;

  // wait when procedure return
  retv =  vsrpc_wait(rpc);
  if (retv != VSRPC_ERR_NONE) return retv;

  // check result
  if (rpc->retc == 1)
    retv = vsrpc_str2int(*rpc->retv);
  else
    retv = VSRPC_ERR_PROT; // protocol error

  if (rpc->retc != 0){ vsrpc_free_argv(rpc->retv); rpc->retc = 0; }
  return retv;
}
//----------------------------------------------------------------------------
#ifdef VSRPC_HELP
// get list of all builtin functions
//    input: Nothing
//   return: "func_name1" "func_name2" ...
//   errors: VSRPC_ERR_NONE
char **vsrpc_builtin_help(vsrpc_t *rpc, int argc, char * const argv[])
{
  char **list;
  vsrpc_local_help(rpc, &list);
  return list;
}
//----------------------------------------------------------------------------
int vsrpc_local_help(vsrpc_t *rpc, char ***list) // return VSRPC_ERR_NONE
{
  vsrpc_func_t *fn;
  int argc = 0; // number of builtin functions
  char **argv; // return list

  // count builtin functions
  if (rpc->bfunc != NULL)
    for (fn = rpc->bfunc, argc = 0; fn->fname != (char*) NULL; fn++)
      argc++;

  // allocate memory for output list
  argv = (char**) vsrpc_malloc ((argc + 1) * (int)sizeof(char*));

  // store pointer to output list
  *list = argv;

  // fill list
  if (rpc->bfunc != NULL)
    for (fn = rpc->bfunc; fn->fname != (char*) NULL; fn++)
      *argv++ = vsrpc_str2str(fn->fname);
  *argv = (char*) NULL;

  return VSRPC_ERR_NONE;
}
//----------------------------------------------------------------------------
int vsrpc_remote_help(vsrpc_t *rpc, char ***list) // return ErrorCode
{
  int retv;
  char **argv, **p;
  *list = (char**) NULL;

  // call procedure on remote machine
  retv = vsrpc_call_ex(rpc, "s", "help");
  if (retv != VSRPC_ERR_NONE) return retv;

  // wait when procedure return result
  retv = vsrpc_wait(rpc);
  if (retv != VSRPC_ERR_NONE) return retv;

  if (rpc->retc == 0) return VSRPC_ERR_NONE; // return NULL

  // allocate memory for output list
  argv = (char**) vsrpc_malloc ((rpc->retc + 1) * (int)sizeof(char*));

  // store pointer to output list
  *list = argv;

  // copy list to user
  for (p = rpc->retv; *p != (char*) NULL; p++)
    *argv++ = vsrpc_str2str(*p);
  *argv = (char*) NULL;

  vsrpc_free_argv(rpc->retv);
  rpc->retc = 0;
  return VSRPC_ERR_NONE;
}
//----------------------------------------------------------------------------
// get list of all user-defined functions
//    input: Nothing
//   return: "func_name1" "func_name2" ...
//   errors: VSRPC_ERR_NONE
char **vsrpc_builtin_list(vsrpc_t *rpc, int argc, char * const argv[])
{
  char **list;
  vsrpc_local_list(rpc, &list);
  return list;
}
//----------------------------------------------------------------------------
int vsrpc_local_list(vsrpc_t *rpc, char ***list) // return VSRPC_ERR_NONE
{
  vsrpc_func_t *fn;
  int argc = 0; // number of user-defined functions
  char **argv; // return list

  // count user-defined functions
  if (rpc->func != NULL)
    for (fn = rpc->func, argc = 0; fn->fname != (char*) NULL; fn++)
      argc++;

  // allocate memory for output list
  argv = (char**) vsrpc_malloc((argc + 1) * (int)sizeof(char*));

  // store pointer to output list
  *list = argv;

  // fill list
  if (rpc->func != NULL)
    for (fn = rpc->func; fn->fname != (char*) NULL; fn++)
      *argv++ = vsrpc_str2str(fn->fname);
  *argv = (char*) NULL;

  return VSRPC_ERR_NONE;
}
//----------------------------------------------------------------------------
int vsrpc_remote_list(vsrpc_t *rpc, char ***list) // return ErrorCode
{
  int retv;
  char **argv, **p;
  *list = (char**) NULL;

  // call procedure on remote machine
  retv = vsrpc_call_ex(rpc, "s", "list");
  if (retv != VSRPC_ERR_NONE) return retv;

  // wait when procedure return result
  retv = vsrpc_wait(rpc);
  if (retv != VSRPC_ERR_NONE) return retv;

  if (rpc->retc == 0) return VSRPC_ERR_NONE; // return NULL

  // allocate memory for output list
  argv = (char**) vsrpc_malloc ((rpc->retc + 1) * (int)sizeof(char*));

  // store pointer to output list
  *list = argv;

  // copy list to user
  for (p = rpc->retv; *p != (char*) NULL; p++)
    *argv++ = vsrpc_str2str(*p);
  *argv = (char*) NULL;

  vsrpc_free_argv(rpc->retv);
  rpc->retc = 0;
  return VSRPC_ERR_NONE;
}
#endif // VSRPC_HELP
//----------------------------------------------------------------------------
// get version of protocol
//    input: Nothing
//   return: ErrorCode "version"
//   errors: VSRPC_ERR_NONE
char **vsrpc_builtin_version(vsrpc_t *rpc, int argc, char * const argv[])
{
  return vsrpc_list2argv("is", VSRPC_ERR_NONE, VSRPC_VERSION);
}
//----------------------------------------------------------------------------
int vsrpc_local_version(vsrpc_t *rpc, char **ver) // VSRPC_ERR_NONE
{
  *ver = vsrpc_str2str(VSRPC_VERSION);
  return VSRPC_ERR_NONE;
}
//----------------------------------------------------------------------------
int vsrpc_remote_version(vsrpc_t *rpc, char **ver) // return ErrorCode
{
  int retv;

  // call procedure on remote machine
  retv = vsrpc_call_ex(rpc, "s", "version");
  if (retv != VSRPC_ERR_NONE) return retv;

  // wait when procedure return result
  retv = vsrpc_wait(rpc);
  if (retv != VSRPC_ERR_NONE) return retv;

  // check result
  if (rpc->retc == 2)
  {
    *ver = vsrpc_str2str(rpc->retv[1]);
    retv = vsrpc_str2int(*rpc->retv);
  }
  else
  {
    *ver = vsrpc_str2str("unknown");
    retv = VSRPC_ERR_PROT; // protocol error
  }

  if (rpc->retc != 0) { vsrpc_free_argv(rpc->retv); rpc->retc = 0; }
  return retv;
}
//----------------------------------------------------------------------------
// compare remote and local protocol version
//    input: Nothing
//   return: ErrorCode, *ok=1 - verion eqval, *ok=0 - bad version
//   errors: VSRPC_ERR_NONE | -???
int vsrpc_check_version(vsrpc_t *rpc, int *ok)
{
  char *local_ver, *remote_ver;
  int retv;
  *ok = 0;

  retv = vsrpc_local_version(rpc, &local_ver);
  if (retv != VSRPC_ERR_NONE) return retv;

  retv = vsrpc_remote_version(rpc, &remote_ver);
  if (retv != VSRPC_ERR_NONE)
  {
    vsrpc_free(local_ver);
    return retv;
  }

  if (strcmp(local_ver, remote_ver) == 0)
    *ok = 1; // good version
  else
    *ok = 0; // bad version

  vsrpc_free(local_ver);
  vsrpc_free(remote_ver);
  return VSRPC_ERR_NONE;
}
//----------------------------------------------------------------------------
// get permissions
//    input: Nothing
//   return: ErrorCode permissins
//   errors: VSRPC_ERR_NONE
char **vsrpc_builtin_perm(vsrpc_t *rpc, int argc, char * const argv[])
{
  return vsrpc_list2argv("ii", VSRPC_ERR_NONE, rpc->perm);
}
//----------------------------------------------------------------------------
int vsrpc_local_perm(vsrpc_t *rpc, int *perm) // return VSRPC_ERR_NONE
{
  *perm = rpc->perm;
  return VSRPC_ERR_NONE;
}
//----------------------------------------------------------------------------
int vsrpc_remote_perm(vsrpc_t *rpc, int *perm) // return ErrorCode
{
  int retv;

  // call procedure on remote machine
  retv = vsrpc_call_ex(rpc, "s", "perm");
  if (retv != VSRPC_ERR_NONE) return retv;

  // wait when procedure return result
  retv = vsrpc_wait(rpc);
  if (retv != VSRPC_ERR_NONE) return retv;

  // check result
  if (rpc->retc == 2)
  {
    *perm = vsrpc_str2int(rpc->retv[1]);
    retv = vsrpc_str2int(*rpc->retv);
  }
  else
  {
    *perm = 0;
    retv = VSRPC_ERR_PROT; // protocol error
  }

  if (rpc->retc != 0) { vsrpc_free_argv(rpc->retv); rpc->retc = 0; }
  return retv;
}
//============================================================================

/*** end of "vsrpc.c" file ***/


#! /bin/sh

if [ `uname` != Linux ]
then
  echo "This VSRPC test may run under Linux only; exit"
  exit
fi
  
if true
then
  bash ../../vsrpc_idl.sh \
    --input-file prj.vsidl \
    --server-out-dir   server \
    --client-out-dir   client \
    --examples-out-dir examples \
    --fn-prefix     rpc_ \
    --remote-suffix _remote \
    --local-suffix  _local
fi

CFLAGS="-O0 -g -fomit-frame-pointer -I../../ -I../../socklib -Iserver -Iclient -Wall -pipe"
LDFLAGS="-lrt"
GCC=gcc
LD=gcc

${GCC} ${CFLAGS} -c ../../vsrpc.c           || exit
${GCC} ${CFLAGS} -c ../../socklib/socklib.c || exit

${GCC} ${CFLAGS} -c   server/prj_local.c          || exit
${GCC} ${CFLAGS} -c   server/prj_server.c         || exit
${GCC} ${CFLAGS} -c examples/prj_server_example.c || exit

${LD} vsrpc.o socklib.o prj_local.o prj_server.o \
         prj_server_example.o \
      -o prj_server_example ${LDFLAGS} || exit


${GCC} ${CFLAGS} -c   client/prj_remote.c         || exit
${GCC} ${CFLAGS} -c   client/prj_client.c         || exit
${GCC} ${CFLAGS} -c examples/prj_client_example.c || exit

${LD} vsrpc.o socklib.o prj_remote.o prj_client.o \
         prj_client_example.o \
      -o prj_client_example ${LDFLAGS} || exit

[ ! -e fifo ] && mkfifo fifo

cat fifo | ./prj_server_example | tee log2 | ./prj_client_example | tee log1 > fifo

#gprof prj_server_example > gprof_server.rpt
#gprof prj_client_example > gprof_client.rpt

# check results
cmp log1 log1.pat && cmp log2 log2.pat && echo && echo ">>> test finished [OK]"


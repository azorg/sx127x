#! /bin/sh

rm -f *.o

../../vsrpc_idl.sh \
  --input-file prj.vsidl \
  --server-out-dir   server \
  --client-out-dir   client \
  --examples-out-dir examples \
  --clean

rm -rf server
rm -rf client
rm -rf examples

rm -f prj_server_example prj_client_example

rm -f fifo
rm -f log1 log2

rm -f gprof_*.rpt
rm -f gmon.out


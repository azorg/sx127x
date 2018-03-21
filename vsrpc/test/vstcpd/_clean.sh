#! /bin/sh

test `uname` = Linux && OPT='-j4' || OPT='WIN32=1'
make clean $OPT

../../vsrpc_idl.sh \
  --input-file rpc.vsidl \
  --server-out-dir server \
  --clean


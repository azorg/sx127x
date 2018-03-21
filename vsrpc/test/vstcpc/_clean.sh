#! /bin/sh

test `uname` = Linux && OPT='-j4' || OPT='WIN32=1'
make clean $OPT

../../vsrpc_idl.sh \
  --input-file ../vstcpd/rpc.vsidl \
  --client-out-dir client \
  --clean

../../vsrpc_idl.sh \
  --input-file lrpc.vsidl \
  --server-out-dir server \
  --clean


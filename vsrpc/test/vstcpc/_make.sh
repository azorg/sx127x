#!/bin/sh

if [ `uname` = "Linux" ]
then
  PROC_NUM=`grep processor /proc/cpuinfo | wc -l`
	OPT="-j $PROC_NUM"
else
	OPT="WIN32=1"
fi

../../vsrpc_idl.sh \
  --input-file ../vstcpd/rpc.vsidl \
  --client-out-dir client \
  --fn-prefix      rpc_ \
  --remote-suffix  _remote

../../vsrpc_idl.sh \
  --input-file lrpc.vsidl \
  --server-out-dir server \
  --fn-prefix      lrpc_ \
  --remote-suffix  _remote \
  --remote-suffix  _local

make $OPT


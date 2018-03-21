#! /bin/sh

if [ `uname` = "Linux" ]
then
  PROC_NUM=`grep processor /proc/cpuinfo | wc -l`
	OPT="-j $PROC_NUM"
else
	OPT="WIN32=1"
fi

../../vsrpc_idl.sh \
  --input-file rpc.vsidl \
  --server-out-dir server \
  --fn-prefix rpc_

make ${OPT}


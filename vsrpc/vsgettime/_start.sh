#! /bin/sh

if [ `uname` = Linux ]
then
  echo "Start test under Linux"
  LDFLAGS="-lrt"
else
  echo "Start test under Windows"
  LDFLAGS=""
fi

OPTIM="-O0 -g"
DEFS="-DVSGETTIME_TEST $DEFS"
CFLAGS="$DEFS $OPTIM -Wall -pipe"
LDFLAGS="-lrt"
GCC="gcc"
#LD="gcc"

${GCC} ${CFLAGS} vsgettime.c -o test_vsgettime ${LDFLAGS} || exit

./test_vsgettime



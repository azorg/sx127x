#! /bin/sh
test `uname` = Linux && OPT='-j4' || OPT='WIN32=1'
make clean $OPT


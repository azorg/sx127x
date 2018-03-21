#----------------------------------------------------------------------------
OUT_NAME    := test_vstcpd
OUT_DIR     := .
#CLEAN_FILES := "$(OUT_DIR)/$(OUT_NAME).exe"
#----------------------------------------------------------------------------
# 1-st way to select source files
SRCS := \
	../../socklib/socklib.c \
	../../vsrpc.c \
	../../vssync.c \
	../../vsthread.c \
	../../vstcps.c \
	../../vstcpd.c \
	server/rpc_local.c \
	server/rpc_server.c \
	test_vstcpd.c

HDRS := \
	../../socklib/socklib.h \
	../../vsrpc.h \
	../../vssync.h \
	../../vsthread.h \
	../../vstcps.h \
	../../vstcpd.h \
	server/rpc_local.h \
	server/rpc_server.h

# 2-nd way to select source files
#SRC_DIRS := . ../.. server ../../socklib
#HDR_DIRS := . ../.. server ../../socklib
#----------------------------------------------------------------------------
#OPTIM := -g -O0
OPTIM := -Os
WARN  := -Wall
#----------------------------------------------------------------------------
ifndef WIN32
# Linux:
DEFS  := -DVSTCPD_DEBUG -DVSTCPS_DEBUG -DVSTHREAD_DEBUG -DVSTHREAD_LINUX_RT
#DEFS := -DVSTCPD_DEBUG -DVSTCPS_DEBUG -DVSTHREAD_DEBUG -DVSTHREAD_LINUX -DVSTHREAD_POOL
else
# Windows (MinGW):
DEFS  := -DVSTCPD_DEBUG -DVSTCPS_DEBUG -DVSTHREAD_DEBUG 
endif
#----------------------------------------------------------------------------
include ../Makefile.os
include ../Makefile.skel
#----------------------------------------------------------------------------

#----------------------------------------------------------------------------
OUT_NAME     := vstcps_test
OUT_DIR      := .
#CLEAN_FILES := "$(OUT_DIR)/$(OUT_NAME).exe"
#----------------------------------------------------------------------------
# 1-st way to select source files
SRCS := \
	../../socklib/socklib.c \
	../../vssync.c \
	../../vsthread.c \
	../../vstcps.c \
	test_vstcps.c

HDRS := \
	../../socklib/socklib.h \
	../../vssync.h \
	../../vsthread.h \
	../../vstcps.h

# 2-nd way to select source files
#SRC_DIRS := . ../..
#HDR_DIRS := . ../..
#----------------------------------------------------------------------------
#OPTIM := -g -O0
OPTIM := -Os
WARN  := -Wall
#----------------------------------------------------------------------------
ifndef WIN32
# Linux:
DEFS  := -DVSTCPS_DEBUG -DVSTHREAD_DEBUG -DVSTHREAD_LINUX
#DEFS := -DVSTCPS_DEBUG -DVSTHREAD_DEBUG -DVSTHREAD_LINUX -DVSTHREAD_POOL 
else
# Windows (MinGW):
DEFS  := -DVSTCPS_DEBUG -DVSTHREAD_DEBUG -DVSTHREAD_POOL 
endif
#----------------------------------------------------------------------------
include ../Makefile.os
include ../Makefile.skel
#----------------------------------------------------------------------------


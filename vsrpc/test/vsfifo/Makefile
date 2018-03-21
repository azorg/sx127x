#----------------------------------------------------------------------------
OUT_NAME    := test_vsfifo
OUT_DIR     := .
#CLEAN_FILES := "$(OUT_DIR)/$(OUT_NAME).exe"
#----------------------------------------------------------------------------
# 1-st way to select source files
SRCS := \
	../../vssync.c \
	../../vsthread.c \
	../../vsfifo.c \
	test_vsfifo.c

HDRS := \
	../../vssync.h \
	../../vsthread.h \
	../../vsfifo.h

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
#DEFS := -DVSTHREAD_DEBUG -DVSTHREAD_LINUX_RT
DEFS  := -DVSTHREAD_DEBUG -DVSTHREAD_LINUX -DVSTHREAD_POOL 
else
# Windows (MinGW):
DEFS  := -DVSTHREAD_DEBUG
endif
#----------------------------------------------------------------------------
include ../Makefile.os
include ../Makefile.skel
#----------------------------------------------------------------------------


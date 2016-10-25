AM_CFLAGS = -Wall -I@AXIOM_MEM_DEV_INC@

if EXTLD
AM_CFLAGS += -DUSE_EXT_LDSCRIPT
endif

if DBG
AM_CFLAGS += -DDEBUG
endif

lib_LTLIBRARIES = libevi_lmm.la
libevi_lmm_la_SOURCES = init.c freeidx_list.c evi_allocator.c
include_HEADERS = evi_lmm.h evi_err.h evi_allocator.h
noinst_HEADERS = freeidx_list.h debug.h

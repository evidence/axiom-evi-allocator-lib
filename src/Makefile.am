AM_CFLAGS = -Wall

if DBG
AM_CFLAGS += -DDEBUG
endif

lib_LTLIBRARIES = libevi_lmm.la
libevi_lmm_la_SOURCES = init.c freeidx_list.c
include_HEADERS = evi_lmm.h evi_err.h
noinst_HEADERS = freeidx_list.h debug.h

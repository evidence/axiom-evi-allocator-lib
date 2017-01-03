ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -Wall

pkgconfigdir = $(prefix)/lib/pkgconfig
pkgconfig_DATA = evi_lmm.pc

SUBDIRS = src test

if EXTLD
SUBDIRS += ldscript
endif

EXTRA_DIST = $(DX_CONFIG)
#docs/output

@DX_RULES@

ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -Wall

SUBDIRS = src test

if EXTLD
SUBDIRS += ldscript
endif

EXTRA_DIST = $(DX_CONFIG)
#docs/output

@DX_RULES@

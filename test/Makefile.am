AM_CPPFLAGS = -I$(top_srcdir)/src/

if EXTLD
AM_LDFLAGS = -Wl,-T$(top_srcdir)/@AXIOM_LD_SCRIPTS_PATH@
endif

#bin_PROGRAMS = a64allocator
noinst_PROGRAMS = testA testB testFreelist

testA_SOURCES = testA.c
testA_LDADD = $(top_srcdir)/src/libevi_lmm.la


testB_SOURCES = testB.c
testB_LDADD = $(top_srcdir)/src/libevi_lmm.la

testFreelist_SOURCES = testFreelist.c
testFreelist_LDADD = $(top_srcdir)/src/libevi_lmm.la

#a64allocator_SOURCES = aarch64_allocator_test.c
#a64allocator_LDADD = $(top_srcdir)/src/libevi_lmm.la

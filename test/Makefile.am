AM_CPPFLAGS = -I$(top_srcdir)/src/

bin_PROGRAMS = a64allocator
noinst_PROGRAMS = testA testB testFreelist

testA_SOURCES = testA.c
testA_LDADD = $(top_srcdir)/src/libaxiom_lmm.la


testB_SOURCES = testB.c
testB_LDADD = $(top_srcdir)/src/libaxiom_lmm.la

testFreelist_SOURCES = testFreelist.c
testFreelist_LDADD = $(top_srcdir)/src/libaxiom_lmm.la

a64allocator_SOURCES = aarch64_allocator_test.c
a64allocator_LDADD = $(top_srcdir)/src/libaxiom_lmm.la

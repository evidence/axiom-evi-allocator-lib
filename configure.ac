#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([axiom-evi-allocator-lib], [0.1], [BUG-REPORT-ADDRESS])

AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_CONFIG_HEADERS([config.h])


AM_PROG_AR

LT_INIT([disable-static])

AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_GREP


######################################################################
# DOXYGEN SUPPORT
######################################################################
DX_HTML_FEATURE(ON)
DX_CHM_FEATURE(OFF)
DX_CHI_FEATURE(OFF)
DX_MAN_FEATURE(OFF)
DX_RTF_FEATURE(OFF)
DX_XML_FEATURE(OFF)
DX_PDF_FEATURE(OFF)
DX_PS_FEATURE(OFF)

DX_INIT_DOXYGEN([$PACKAGE_NAME],[docs/doxygen.cfg],[docs/output])

# Checks for libraries.


######################################################################
# axiom-dev-mem header file
######################################################################
AXIOM_MEM_DEV_INC="/usr/include"

AC_ARG_WITH([axiom-mem-dev],
           AS_HELP_STRING([--with-axiom-mem-dev=dir], [Include directory fo axiom-mem-dev]),
           [
                AXIOM_MEM_DEV_INC="$withval"
           ]
)
AC_SUBST([AXIOM_MEM_DEV_INC])

AXIOM_LD_SCRIPTS=""
AC_ARG_ENABLE([extld],
	      AS_HELP_STRING([--enable-extld],
			     [Enable the use of the external Linker Script (only aarch64)]),
	      [case "${enableval}" in
	        yes) AXIOM_LD_SCRIPTS="xs_map64.lds"; use_extld=yes ;;
	        no) use_extld=no ;;
	        *) AC_MSG_ERROR([bad value ${enableval} for --enable-extld]) ;;
	       esac], [use_extld=no])
AM_CONDITIONAL([EXTLD], [test x"$use_extld" = "xyes"])

AC_ARG_ENABLE([debug],
	      AS_HELP_STRING([--enable-debug],
			     [build library with debugging symbols]),
	      [case "${enableval}" in
	        yes) dbg=yes ;;
	        no) dbg=no ;;
	        *) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
	       esac], [dbg=no])
AM_CONDITIONAL([DBG], [test x"$dbg" = "xyes"])

# Checks for header files.
AC_CHECK_HEADERS([stddef.h stdint.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_MALLOC

AXIOM_LD_SCRIPTS_PATH=""
AXIOM_LD_SCRIPTS_MAKE=""
AXIOM_LD_SCRIPTS_MAKE_ARCH=""
if test x"$use_extld" = "xyes"; then
LINKER_SEARCH_PATH_BLOCK=$($CC -Wl,--verbose 2>&1 | $GREP "SEARCH_DIR")
AXIOM_LD_SCRIPTS_PATH="ldscript/aarch64/$AXIOM_LD_SCRIPTS"
AXIOM_LD_SCRIPTS_MAKE="ldscript/Makefile"
AXIOM_LD_SCRIPTS_MAKE_ARCH="ldscript/aarch64/Makefile"
AC_SUBST([AXIOM_LD_SCRIPTS])
AC_SUBST([LINKER_SEARCH_PATH_BLOCK])
AC_SUBST([AXIOM_LD_SCRIPTS_PATH])
fi

AC_CONFIG_FILES([Makefile
		 ldscript/Makefile
		 ldscript/aarch64/Makefile
		 $AXIOM_LD_SCRIPTS_MAKE
		 src/Makefile
		 test/Makefile
		 docs/doxygen.cfg])
AC_OUTPUT

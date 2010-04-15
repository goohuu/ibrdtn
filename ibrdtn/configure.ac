
dnl --------------------------------
dnl Initialization macros.
dnl --------------------------------

AC_INIT(ibrdtn/ibrdtn.h)
AC_CONFIG_HEADER(ibrdtn/config.h)

AC_CONFIG_MACRO_DIR([m4])
LT_INIT
AM_MAINTAINER_MODE

dnl -----------------------------------------------
dnl Package name and version number (user defined)
dnl -----------------------------------------------

GENERIC_LIBRARY_NAME=ibrdtn

#release versioning
GENERIC_MAJOR_VERSION=0
GENERIC_MINOR_VERSION=3
GENERIC_MICRO_VERSION=0

#API version (often = GENERIC_MAJOR_VERSION.GENERIC_MINOR_VERSION)
GENERIC_API_VERSION=0.3
AC_SUBST(GENERIC_API_VERSION)

#shared library versioning
GENERIC_LIBRARY_VERSION=1:1:0
#                       | | |
#                +------+ | +---+
#                |        |     |
#             current:revision:age
#                |        |     |
#                |        |     +- increment if interfaces have been added
#                |        |        set to zero if interfaces have been removed
#                                  or changed
#                |        +- increment if source code has changed
#                |           set to zero if current is incremented
#                +- increment if interfaces have been added, removed or changed

dnl --------------------------------
dnl Package name and version number
dnl --------------------------------

AC_SUBST(GENERIC_LIBRARY_VERSION)

PACKAGE=$GENERIC_LIBRARY_NAME
AC_SUBST(GENERIC_LIBRARY_NAME)

GENERIC_VERSION=$GENERIC_MAJOR_VERSION.$GENERIC_MINOR_VERSION.$GENERIC_MICRO_VERSION
GENERIC_RELEASE=$GENERIC_MAJOR_VERSION.$GENERIC_MINOR_VERSION
AC_SUBST(GENERIC_RELEASE)
AC_SUBST(GENERIC_VERSION)

VERSION=$GENERIC_VERSION

AM_INIT_AUTOMAKE($PACKAGE, $VERSION, no-define)

dnl -----------------------------------------------
dnl Checks for programs.
dnl -----------------------------------------------

abi_save_CFLAGS="$CFLAGS"
abi_save_CXXFLAGS="$CXXFLAGS"

AC_PROG_CXX
AC_PROG_CC

CFLAGS="$abi_save_CFLAGS"
CXXFLAGS="$abi_save_CXXFLAGS"

AC_PROG_INSTALL
AC_PROG_LIBTOOL
AM_SANITY_CHECK

AC_LANG_CPLUSPLUS

# Checks for libraries.
AC_CHECK_LIB([rt], [clock_gettime])

# check for ibrcommon library
PKG_CHECK_MODULES([ibrcommon], [ibrcommon >= 0.1])
AC_SUBST(ibrcommon_CFLAGS)
AC_SUBST(ibrcommon_LIBS)

# Checks for header files.
AC_CHECK_HEADERS([inttypes.h stddef.h stdint.h stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_CHECK_FUNCS([gettimeofday sqrt])

dnl -----------------------------------------------
dnl debugging switches
dnl -----------------------------------------------

AC_ARG_ENABLE([debug],
	[AC_HELP_STRING([--enable-debug], [enable extra debugging output [default=no]])],
	[
		AC_DEFINE([DO_DEBUG_OUTPUT], [], [Define to enable extra debugging output.])
		DEBUG_CFLAGS="-ggdb"
	], [
		DEBUG_CFLAGS="-O3"
	])
	
AC_SUBST(DEBUG_CFLAGS)
	
AC_ARG_ENABLE([extended-debug],
	[AC_HELP_STRING([--enable-extended-debug], [enable extended debugging output [default=no]])],
	[AC_DEFINE([DO_EXTENDED_DEBUG_OUTPUT], [], [Define to enable extended debugging output.])],
	[])


dnl -----------------------------------------------
dnl Generates Makefile's, configuration files and scripts
dnl -----------------------------------------------

AC_CONFIG_FILES([Makefile \
	ibrdtn.pc \
	ibrdtn/Makefile \
	ibrdtn/api/Makefile \
	ibrdtn/data/Makefile \
	ibrdtn/streams/Makefile \
	ibrdtn/utils/Makefile])
	
AC_OUTPUT

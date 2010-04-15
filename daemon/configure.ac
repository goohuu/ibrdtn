dnl --------------------------------
dnl Initialization macros.
dnl --------------------------------

AC_INIT(src/Main.cpp)
AC_CONFIG_HEADER([src/config.h])

AC_CONFIG_MACRO_DIR([m4])
AM_MAINTAINER_MODE

dnl --------------------------------
dnl ibrdtn package configuration
dnl --------------------------------

PACKAGE=ibrdtn

#release versioning
GENERIC_MAJOR_VERSION=0
GENERIC_MINOR_VERSION=3
GENERIC_MICRO_VERSION=0

GENERIC_VERSION=$GENERIC_MAJOR_VERSION.$GENERIC_MINOR_VERSION.$GENERIC_MICRO_VERSION
GENERIC_RELEASE=$GENERIC_MAJOR_VERSION.$GENERIC_MINOR_VERSION
AC_SUBST(GENERIC_RELEASE)
AC_SUBST(GENERIC_VERSION)

VERSION=$GENERIC_VERSION
AM_INIT_AUTOMAKE($PACKAGE, $VERSION, morgenro@ibr.cs.tu-bs.de)

dnl -----------------------------------------------
dnl Checks for programs.
dnl -----------------------------------------------

AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB

AC_LANG_CPLUSPLUS

# Checks for libraries.
AC_CHECK_LIB([sqlite3], [sqlite3_open])
AC_CHECK_LIB([rt], [clock_gettime])

# check for ibrdtn library
#AC_CHECK_LIB([ibrdtn], [main])
PKG_CHECK_MODULES([ibrdtn], [ibrdtn >= 0.3])
AC_SUBST(ibrdtn_CFLAGS)
AC_SUBST(ibrdtn_LIBS)

# check for ibrcommon library
PKG_CHECK_MODULES([ibrcommon], [ibrcommon >= 0.1])
AC_SUBST(ibrcommon_CFLAGS)
AC_SUBST(ibrcommon_LIBS)

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netinet/in.h stdlib.h string.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([gethostname socket])

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
	src/Makefile \
	doc/Makefile \
	man/Makefile])
	
AC_OUTPUT
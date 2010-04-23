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

PACKAGE=ibrdtnd

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
dnl check for pthread library
dnl -----------------------------------------------

THREADLIB=error

AC_CHECK_LIB(pthread, pthread_attr_init,
	THREADLIB="pthread")

if test "x$THREADLIB" = xerror; then
	AC_CHECK_LIB(pthreads, pthread_attr_init,
		THREADLIB="pthreads")
fi

if test "x$THREADLIB" = xerror; then
	AC_CHECK_LIB(thread, pthread_attr_init,
		THREADLIB="thread")
fi

PTHREAD_LIBS="-l$THREADLIB"
PTHREAD_CFLAGS=""

AC_CHECK_LIB($THREADLIB,pthread_create)
AC_CHECK_LIB($THREADLIB,pthread_mutex_init)
AC_CHECK_LIB($THREADLIB,pthread_cond_init)

AC_CHECK_LIB($THREADLIB,pthread_condattr_setclock,[
	AC_DEFINE(HAVE_PTHREAD_CONDATTR_SETCLOCK, [1], ["pthread clocking"])
])
AC_CHECK_LIB($THREADLIB,pthread_setconcurrency,[
	AC_DEFINE(HAVE_PTHREAD_SETCONCURRENCY, [1], ["pthread concurrency"])
])

AC_CHECK_LIB($THREADLIB,pthread_yield,[
	AC_DEFINE(HAVE_PTHREAD_YIELD, [1], ["pthread yield"],[
		AC_CHECK_LIB($tlib,pthread_yield_np,[
			AC_DEFINE(HAVE_PTHREAD_YIELD_NP, [1], ["pthread yield np"])
		])
	])
])

AC_CHECK_LIB($THREADLIB,pthread_delay,[
	AC_DEFINE(HAVE_PTHREAD_DELAY, [1], ["pthread delay"])
])
AC_CHECK_LIB($THREADLIB,pthread_delay_np,[
	AC_DEFINE(HAVE_PTHREAD_DELAY_NP, [1], ["pthread delay np"])
])
AC_CHECK_LIB($THREADLIB,pthread_setschedprio,[
	AC_DEFINE(HAVE_PTHREAD_SETSCHEDPRIO, [1], ["pthread scheduling"])
])

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
dnl put all flags together
dnl -----------------------------------------------

CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS"
LDFLAGS="$LDFLAGS $PTHREAD_LIBS"

dnl -----------------------------------------------
dnl Generates Makefile's, configuration files and scripts
dnl -----------------------------------------------

AC_CONFIG_FILES([Makefile \
	src/Makefile \
	doc/Makefile \
	man/Makefile])
	
AC_OUTPUT
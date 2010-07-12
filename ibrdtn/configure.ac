dnl --------------------------------
dnl Package name and version number
dnl --------------------------------

# include version information
m4_include([version.m4])

dnl --------------------------------
dnl Initialization macros.
dnl --------------------------------

AC_INIT([ibrdtn], PKG_FULL_VERSION, morgenro@ibr.cs.tu-bs.de)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR(ibrdtn/ibrdtn.h)
AC_CONFIG_HEADER(ibrdtn/config.h)
LT_INIT
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

dnl --------------------------------
dnl library version number
dnl --------------------------------

#API version
GENERIC_API_VERSION=PKG_MAJOR_VERSION.PKG_MINOR_VERSION

#shared library versioning
GENERIC_LIBRARY_VERSION=PKG_LIB_VERSION
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

# define the library name
GENERIC_LIBRARY_NAME=$PACKAGE_NAME
GENERIC_VERSION=$PACKAGE_VERSION
GENERIC_RELEASE=PKG_MAJOR_VERSION.PKG_MINOR_VERSION

AC_SUBST(GENERIC_API_VERSION)
AC_SUBST(GENERIC_LIBRARY_VERSION)
AC_SUBST(GENERIC_LIBRARY_NAME)
AC_SUBST(GENERIC_RELEASE)
AC_SUBST(GENERIC_VERSION)

dnl -----------------------------------------------
dnl Checks for programs.
dnl -----------------------------------------------

AC_PROG_CXX
AC_PROG_CC

AC_PROG_INSTALL
AC_PROG_LIBTOOL
AM_SANITY_CHECK

AC_LANG_CPLUSPLUS

# Checks for libraries.
AC_CHECK_LIB([rt], [clock_gettime])

# check for ibrcommon library
PKG_CHECK_MODULES([ibrcommon], [ibrcommon >= 0.3])
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

AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_LIBS)

# check for openssl
PKG_CHECK_MODULES(OPENSSL, openssl)

dnl -----------------------------------------------
dnl Enable testsuite
dnl -----------------------------------------------

AC_ARG_ENABLE(testsuite,
	[AC_HELP_STRING([--enable-testsuite], [enable testsuite [default=no]]) ],
	[
		AC_CONFIG_FILES([tests/Makefile])
		TESTSUITE_DIR="tests"
		AC_SUBST(TESTSUITE_DIR)
	], [
		TESTSUITE_DIR=""
	])

dnl -----------------------------------------------
dnl Generates Makefiles, configuration files and scripts
dnl -----------------------------------------------

AC_CONFIG_FILES([Makefile \
	ibrdtn.pc \
	ibrdtn/Makefile \
	ibrdtn/api/Makefile \
	ibrdtn/data/Makefile \
	ibrdtn/streams/Makefile \
	ibrdtn/utils/Makefile])
	
AC_OUTPUT

dnl --------------------------------
dnl ibrdtn package configuration
dnl --------------------------------

define([PKG_VERSION], m4_normalize(m4_include([version.inc])))

AC_INIT(ibrdtnd, [PKG_VERSION], morgenro@ibr.cs.tu-bs.de)
AC_CONFIG_SRCDIR(src/Main.cpp)
AM_INIT_AUTOMAKE
LT_INIT
AM_PATH_CPPUNIT(1.9.6)

AC_CONFIG_HEADER([src/config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_MAINTAINER_MODE

dnl -----------------------------------------------
dnl define SVN revision
dnl -----------------------------------------------
define([svnversion], esyscmd([sh -c "svnversion|tr -d '\n'"]))dnl
AC_DEFINE(SVN_REVISION, "svnversion", [SVN Revision])

dnl -----------------------------------------------
dnl Checks for programs.
dnl -----------------------------------------------

AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP

AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

AC_LANG_CPLUSPLUS

# Checks for libraries.
AC_CHECK_LIB([sqlite3], [sqlite3_open])
AC_CHECK_LIB([rt], [clock_gettime])

# check for ibrdtn library
#AC_CHECK_LIB([ibrdtn], [main])
PKG_CHECK_MODULES([ibrdtn], [ibrdtn >= 0.4])
AC_SUBST(ibrdtn_CFLAGS)
AC_SUBST(ibrdtn_LIBS)

# check for ibrcommon library
PKG_CHECK_MODULES([ibrcommon], [ibrcommon >= 0.4])
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

AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_LIBS)

# check for openssl
PKG_CHECK_MODULES(OPENSSL, openssl)

AC_ARG_WITH(curl,
	AS_HELP_STRING([--with-curl], [Build with libcurl
		@<:@default=no@:>@]),
	[
	if test "x$with_curl" = "xyes"; then
		PKG_CHECK_MODULES(CURL, libcurl >= 7.16.4, use_curl="yes", use_curl="no")
		AC_DEFINE(HAVE_LIBCURL, [1], ["curl library is available"])
	else
		use_curl="no"
	fi
	],[
	use_curl="no"
])

AM_CONDITIONAL(CURL, test x$use_curl = xyes)

dnl -----------------------------------------------
dnl Generates Makefiles, configuration files and scripts
dnl -----------------------------------------------

AC_CONFIG_FILES([Makefile \
	src/Makefile \
	doc/Makefile \
	man/Makefile \
	etc/Makefile])
	
AC_OUTPUT
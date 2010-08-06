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
AC_CHECK_LIB([rt], [clock_gettime])

# check for ibrdtn library
LOCAL_IBRDTN_VERSION=0.4
LOCAL_CHECK_IBRDTN
AC_SUBST(ibrdtn_CFLAGS)
AC_SUBST(ibrdtn_LIBS)

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netinet/in.h stdlib.h string.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([gethostname socket])

AC_ARG_WITH(curl,
	AS_HELP_STRING([--with-curl], [Build with libcurl
		@<:@default=no@:>@]),
	[
	if test "x$with_curl" = "xyes"; then
		PKG_CHECK_MODULES(CURL, libcurl >= 7.16.4, use_curl="yes", use_curl="no")
		AC_DEFINE(HAVE_LIBCURL, [1], ["curl library is available"])
		AC_SUBST(CURL_CFLAGS)
		AC_SUBST(CURL_LIBS)
	else
		use_curl="no"
	fi
	],[
	use_curl="no"
])

AC_ARG_WITH(sqlite,
	AS_HELP_STRING([--with-sqlite], [Build with sqlite support
		@<:@default=no@:>@]),
	[
	if test "x$with_curl" = "xyes"; then
		PKG_CHECK_MODULES(SQLITE, sqlite3, use_sqlite="yes", use_sqlite="no")
		AC_SUBST(SQLITE_CFLAGS)
		AC_SUBST(SQLITE_LIBS)
		AC_DEFINE(HAVE_SQLITE, [1], ["sqlite library is available"])
	else
		use_sqlite="no"
	fi
	],[
	use_sqlite="no"
])

AM_CONDITIONAL(CURL, test x$use_curl = xyes)
AM_CONDITIONAL(SQLITE, test x$use_sqlite = xyes)

dnl -----------------------------------------------
dnl Generates Makefiles, configuration files and scripts
dnl -----------------------------------------------

AC_CONFIG_FILES([Makefile \
	src/Makefile \
	doc/Makefile \
	man/Makefile \
	etc/Makefile])
	
AC_OUTPUT
## Source directory

h_sources = TestBundleSummary.h
cc_sources = TestBundleSummary.cpp
				
# what flags you want to pass to the C compiler & linker
AM_CPPFLAGS = @ibrdtn_CFLAGS@ 
AM_LDFLAGS = @ibrdtn_LIBS@

if CURL
AM_CPPFLAGS += @CURL_CFLAGS@
AM_LDFLAGS += @CURL_LIBS@
endif

if SQLITE
AM_CPPFLAGS += @SQLITE_CFLAGS@
AM_LDFLAGS += @SQLITE_LIBS@
endif

INCLUDES = -I@top_srcdir@

check_PROGRAMS = testsuite
testsuite_CXXFLAGS = ${AM_CPPFLAGS} ${CPPUNIT_CFLAGS} -Wall
testsuite_LDFLAGS = ${AM_LDFLAGS} ${CPPUNIT_LIBS} @top_srcdir@/src/libdaemon.a @top_srcdir@/src/core/libcore.a @top_srcdir@/src/net/libnet.a @top_srcdir@/src/routing/librouting.a
testsuite_SOURCES = $(h_sources) $(cc_sources) testsuite.cpp

TESTS = testsuite

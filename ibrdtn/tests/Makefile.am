## Source directory

h_sources = 
cc_sources = Main.cpp 

AM_CPPFLAGS = $(PTHREAD_CFLAGS) $(ibrcommon_CFLAGS) $(ibrdtn_CFLAGS)
AM_LDFLAGS = $(PTHREAD_LIBS) $(ibrcommon_LIBS) $(ibrdtn_LIBS)

INCLUDES = -I@top_srcdir@ -I@top_srcdir@/tests

bin_PROGRAMS = ibrdtn-testsuite
ibrdtn_testsuite_SOURCES = $(h_sources) $(cc_sources)
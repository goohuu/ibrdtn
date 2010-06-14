AUTOMAKE_OPTIONS = foreign

# what flags you want to pass to the C compiler & linker
AM_CPPFLAGS = $(PTHREAD_CFLAGS) $(ibrcommon_CFLAGS) $(ibrdtn_CFLAGS)
AM_LDFLAGS = $(PTHREAD_LIBS) $(ibrcommon_LIBS) $(ibrdtn_LIBS)

INCLUDES = -I@top_srcdir@/src

# this lists the binaries to produce, the (non-PHONY, binary) targets in
# the previous manual Makefile
bin_PROGRAMS = dtnping dtninbox dtnoutbox dtnrecv dtnsend
sbin_PROGRAMS = dtntunnel
dtnping_SOURCES = dtnping.cpp
dtninbox_SOURCES = dtninbox.cpp
dtnoutbox_SOURCES = dtnoutbox.cpp
dtnrecv_SOURCES = dtnrecv.cpp
dtnsend_SOURCES = dtnsend.cpp
dtntunnel_SOURCES = dtntunnel.cpp

AUTOMAKE_OPTIONS = foreign
SUBDIRS = core net routing
noinst_HEADERS = config.h

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

if DAEMON
AM_CPPFLAGS += @DAEMON_CFLAGS@
AM_LDFLAGS += @DAEMON_LIBS@
endif

if DTNSEC
SUBDIRS += security
endif

INCLUDES = -I@top_srcdir@/src

# this lists the binaries to produce, the (non-PHONY, binary) targets in
# the previous manual Makefile
sbin_PROGRAMS = dtnd

daemon_SOURCES =  ApiServer.cpp \
				ApiServer.h \
				ClientHandler.cpp \
				ClientHandler.h \
				Component.cpp \
				Component.h \
				config.h \
				Configuration.cpp \
				Configuration.h \
				Debugger.cpp \
				Debugger.h \
				DevNull.cpp \
				DevNull.h \
				EchoWorker.cpp \
				EchoWorker.h \
				Notifier.cpp \
				Notifier.h \
				StatisticLogger.h \
				StatisticLogger.cpp

noinst_LTLIBRARIES = libdaemon.la
libdaemon_la_SOURCES= $(daemon_SOURCES)
libdaemon_la_LIBADD = core/libcore.la net/libnet.la routing/librouting.la

if DTNSEC
libdaemon_la_LIBADD += security/libsecurity.la
endif

dtnd_SOURCES = Main.cpp
dtnd_LDADD = libdaemon.la

## sub directory

AM_CPPFLAGS = -I$(top_srcdir) @ibrdtn_CFLAGS@
AM_LDFLAGS = @ibrdtn_LIBS@

core_SOURCES =	AbstractWorker.cpp \
				AbstractWorker.h \
				BundleCore.cpp \
				BundleCore.h \
				BundleEvent.cpp \
				BundleEvent.h \
				BundleExpiredEvent.cpp \
				BundleExpiredEvent.h \
				BundleGeneratedEvent.cpp \
				BundleGeneratedEvent.h \
				BundleStorage.cpp \
				BundleStorage.h \
				WallClock.cpp \
				WallClock.h \
				CustodyEvent.cpp \
				CustodyEvent.h \
				Event.cpp \
				EventDebugger.cpp \
				EventDebugger.h \
				Event.h \
				EventReceiver.cpp \
				EventReceiver.h \
				EventSwitch.cpp \
				EventSwitch.h \
				GlobalEvent.cpp \
				GlobalEvent.h \
				Node.cpp \
				NodeEvent.cpp \
				NodeEvent.h \
				Node.h \
				MemoryBundleStorage.h \
				MemoryBundleStorage.cpp \
				SimpleBundleStorage.cpp \
				SimpleBundleStorage.h \
				StatusReportGenerator.cpp \
				StatusReportGenerator.h \
				TimeEvent.cpp \
				TimeEvent.h \
				DataStorage.h \
				DataStorage.cpp
				
if SQLITE
core_SOURCES += SQLiteConfigure.h SQLiteConfigure.cpp SQLiteBundleStorage.h SQLiteBundleStorage.cpp
AM_CPPFLAGS += @SQLITE_CFLAGS@
AM_LDFLAGS += @SQLITE_LIBS@
endif

noinst_LTLIBRARIES = libcore.la
libcore_la_SOURCES= $(core_SOURCES)

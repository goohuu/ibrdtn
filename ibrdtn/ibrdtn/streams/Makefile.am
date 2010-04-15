## sub directory

h_sources = BundleCopier.h BundleFactory.h BundleHandler.h BundleStreamReader.h BundleStreamWriter.h BundleWriter.h FakeBundleHandler.h StreamConnection.h StreamContactHeader.h StreamDataSegment.h
cc_sources = BundleCopier.cpp BundleFactory.cpp BundleStreamReader.cpp BundleStreamWriter.cpp StreamBuffer.cpp StreamConnection.cpp StreamContactHeader.cpp StreamDataSegment.cpp

#Install the headers in a versioned directory
library_includedir=$(includedir)/$(GENERIC_LIBRARY_NAME)-$(GENERIC_API_VERSION)/$(GENERIC_LIBRARY_NAME)/streams
library_include_HEADERS = $(h_sources)

AM_CPPFLAGS = -I$(top_srcdir) $(DEBUG_CFLAGS) $(ibrcommon_CFLAGS)
AM_LDFLAGS = $(ibrcommon_LIBS)

noinst_LTLIBRARIES= libstreams.la
libstreams_la_SOURCES= $(h_sources) $(cc_sources)
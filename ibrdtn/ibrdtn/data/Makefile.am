## sub directory

h_sources = Serializer.h Block.h Bundle.h BundleID.h BundleMerger.h BundleString.h CustodySignalBlock.h Dictionary.h DTNTime.h EID.h Exceptions.h ExtensionBlock.h PayloadBlock.h PrimaryBlock.h SDNV.h SecurityBlock.h StatusReportBlock.h
cc_sources = Serializer.cpp Block.cpp Bundle.cpp BundleID.cpp BundleMerger.cpp BundleString.cpp CustodySignalBlock.cpp Dictionary.cpp DTNTime.cpp EID.cpp ExtensionBlock.cpp PayloadBlock.cpp PrimaryBlock.cpp SDNV.cpp SecurityBlock.cpp StatusReportBlock.cpp

#Install the headers in a versioned directory
library_includedir=$(includedir)/$(GENERIC_LIBRARY_NAME)-$(GENERIC_API_VERSION)/$(GENERIC_LIBRARY_NAME)/data
library_include_HEADERS = $(h_sources)

AM_CPPFLAGS = -I$(top_srcdir) $(DEBUG_CFLAGS) $(ibrcommon_CFLAGS)
AM_LDFLAGS = $(ibrcommon_LIBS)

noinst_LTLIBRARIES= libdata.la
libdata_la_SOURCES= $(h_sources) $(cc_sources)
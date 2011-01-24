## sub directory

AM_CPPFLAGS = -I$(top_srcdir) @ibrdtn_CFLAGS@
AM_LDFLAGS = @ibrdtn_LIBS@

routing_SOURCES = EpidemicRoutingExtension.cpp \
				EpidemicRoutingExtension.h \
				EpidemicExtensionBlock.h \
				EpidemicExtensionBlock.cpp \
				EpidemicControlMessage.h \
				EpidemicControlMessage.cpp
				
noinst_LTLIBRARIES = librtepidemic.la
librtepidemic_la_SOURCES= $(routing_SOURCES)

## sub directory

AM_CPPFLAGS = -I$(top_srcdir) @ibrdtn_CFLAGS@
AM_LDFLAGS = @ibrdtn_LIBS@

routing_SOURCES = BaseRouter.cpp \
				BaseRouter.h \
				BundleSummary.cpp \
				BundleSummary.h \
				EpidemicRoutingExtension.cpp \
				EpidemicRoutingExtension.h \
				FloodRoutingExtension.cpp \
				FloodRoutingExtension.h \
				NeighborDatabase.cpp \
				NeighborDatabase.h \
				NeighborRoutingExtension.cpp \
				NeighborRoutingExtension.h \
				QueueBundleEvent.cpp \
				QueueBundleEvent.h \
				RequeueBundleEvent.cpp \
				RequeueBundleEvent.h \
				RetransmissionExtension.cpp \
				RetransmissionExtension.h \
				StaticRoutingExtension.cpp \
				StaticRoutingExtension.h \
				SummaryVector.cpp \
				SummaryVector.h
				
noinst_LTLIBRARIES = librouting.la
librouting_la_SOURCES= $(routing_SOURCES)

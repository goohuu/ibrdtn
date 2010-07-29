AUTOMAKE_OPTIONS = foreign

# what flags you want to pass to the C compiler & linker
AM_CPPFLAGS = $(PTHREAD_CFLAGS) $(ibrcommon_CFLAGS) $(ibrdtn_CFLAGS) $(OPENSSL_CFLAGS)
AM_LDFLAGS = $(PTHREAD_LIBS) $(ibrcommon_LIBS) $(ibrdtn_LIBS) $(OPENSSL_LIBS)

if CURL
AM_CPPFLAGS += @CURL_CFLAGS@
AM_LDFLAGS += @CURL_LIBS@
endif

INCLUDES = -I@top_srcdir@/src

TESTS = testsuite
check_PROGRAMS = $(TESTS)

testsuite_CXXFLAGS = ${AM_CPPFLAGS} ${CPPUNIT_CFLAGS} -I../../src -Wall
testsuite_LDFLAGS = ${AM_LDFLAGS} ${CPPUNIT_LIBS}

# this lists the binaries to produce, the (non-PHONY, binary) targets in
# the previous manual Makefile
sbin_PROGRAMS = dtnd

core_SOURCES =	core/AbstractWorker.cpp \
				core/AbstractWorker.h \
				core/BundleCore.cpp \
				core/BundleCore.h \
				core/BundleEvent.cpp \
				core/BundleEvent.h \
				core/BundleExpiredEvent.cpp \
				core/BundleExpiredEvent.h \
				core/BundleGeneratedEvent.cpp \
				core/BundleGeneratedEvent.h \
				core/BundleStorage.cpp \
				core/BundleStorage.h \
				core/WallClock.cpp \
				core/WallClock.h \
				core/CustodyEvent.cpp \
				core/CustodyEvent.h \
				core/Event.cpp \
				core/EventDebugger.cpp \
				core/EventDebugger.h \
				core/Event.h \
				core/EventReceiver.cpp \
				core/EventReceiver.h \
				core/EventSwitch.cpp \
				core/EventSwitch.h \
				core/GlobalEvent.cpp \
				core/GlobalEvent.h \
				core/Node.cpp \
				core/NodeEvent.cpp \
				core/NodeEvent.h \
				core/Node.h \
				core/SimpleBundleStorage.cpp \
				core/SimpleBundleStorage.h \
				core/StatusReportGenerator.cpp \
				core/StatusReportGenerator.h \
				core/TimeEvent.cpp \
				core/TimeEvent.h
				
routing_SOURCES = routing/BaseRouter.cpp \
				routing/BaseRouter.h \
				routing/BundleSummary.cpp \
				routing/BundleSummary.h \
				routing/EpidemicRoutingExtension.cpp \
				routing/EpidemicRoutingExtension.h \
				routing/NeighborDatabase.cpp \
				routing/NeighborDatabase.h \
				routing/NeighborRoutingExtension.cpp \
				routing/NeighborRoutingExtension.h \
				routing/QueueBundleEvent.cpp \
				routing/QueueBundleEvent.h \
				routing/RequeueBundleEvent.cpp \
				routing/RequeueBundleEvent.h \
				routing/RetransmissionExtension.cpp \
				routing/RetransmissionExtension.h \
				routing/StaticRoutingExtension.cpp \
				routing/StaticRoutingExtension.h \
				routing/SummaryVector.cpp \
				routing/SummaryVector.h
				
net_SOURCES = 	net/BundleReceivedEvent.cpp \
				net/BundleReceivedEvent.h \
				net/BundleReceiver.h \
				net/ConnectionEvent.cpp \
				net/ConnectionEvent.h \
				net/ConnectionManager.cpp \
				net/ConnectionManager.h \
				net/ConvergenceLayer.cpp \
				net/ConvergenceLayer.h \
				net/DiscoveryAgent.cpp \
				net/DiscoveryAgent.h \
				net/DiscoveryAnnouncement.cpp \
				net/DiscoveryAnnouncement.h \
				net/DiscoveryService.cpp \
				net/DiscoveryService.h \
				net/DiscoveryServiceProvider.h \
				net/GenericServer.h \
				net/IPNDAgent.cpp \
				net/IPNDAgent.h \
				net/Neighbor.cpp \
				net/Neighbor.h \
				net/TCPConnection.cpp \
				net/TCPConvergenceLayer.cpp \
				net/TCPConvergenceLayer.h \
				net/TransferAbortedEvent.cpp \
				net/TransferAbortedEvent.h \
				net/TransferCompletedEvent.cpp \
				net/TransferCompletedEvent.h \
				net/UDPConvergenceLayer.cpp \
				net/UDPConvergenceLayer.h
				
if CURL
net_SOURCES += net/HTTPConvergenceLayer.cpp Net/HTTPConvergenceLayer.h
endif

base_SOURCES =  $(core_SOURCES) $(net_SOURCES) $(routing_SOURCES) \
				ApiServer.cpp \
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
				
dtnd_SOURCES =  $(base_SOURCES) Main.cpp

testsuite_SOURCES = $(base_SOURCES) \
				tests/TestBundleSummary.cpp \
				tests/TestBundleSummary.h \
				tests/testsuite.cpp

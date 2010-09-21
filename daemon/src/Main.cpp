#include "config.h"

#include <ibrcommon/data/BLOB.h>
#include <ibrcommon/data/File.h>
#include <ibrcommon/AutoDelete.h>
#include <ibrcommon/net/NetInterface.h>
#include "ibrcommon/Logger.h"
#include <ibrdtn/utils/Clock.h>
#include <list>

#include "core/BundleCore.h"
#include "core/EventSwitch.h"
#include "core/BundleStorage.h"
#include "core/SimpleBundleStorage.h"

#include "core/Node.h"
#include "core/EventSwitch.h"
#include "core/GlobalEvent.h"
#include "core/NodeEvent.h"

#include "routing/BaseRouter.h"
#include "routing/StaticRoutingExtension.h"
#include "routing/NeighborRoutingExtension.h"
#include "routing/EpidemicRoutingExtension.h"
#include "routing/FloodRoutingExtension.h"
#include "routing/RetransmissionExtension.h"

#include "net/UDPConvergenceLayer.h"
#include "net/TCPConvergenceLayer.h"

#ifdef HAVE_SQLITE
#include "core/SQLiteBundleStorage.h"
#endif

#ifdef HAVE_LIBCURL
#include "net/HTTPConvergenceLayer.h"
#endif

#ifdef HAVE_LOWPAN_SUPPORT
#include "net/LOWPANConvergenceLayer.h"
#endif

#include "net/IPNDAgent.h"

#include "ApiServer.h"
#include "Configuration.h"
#include "EchoWorker.h"
#include "Notifier.h"
#include "DevNull.h"
#include "StatisticLogger.h"
#include "Component.h"

#include <csignal>
#include <sys/types.h>
#include <syslog.h>
#include <set>

using namespace dtn::core;
using namespace dtn::daemon;
using namespace dtn::utils;
using namespace dtn::net;

#include "Debugger.h"

#define UNIT_MB * 1048576

// on interruption do this!
void term(int signal)
{
	if (signal >= 1)
	{
		dtn::core::GlobalEvent::raise(dtn::core::GlobalEvent::GLOBAL_SHUTDOWN);
	}
}

void reload(int)
{
	// send shutdown signal to unbound threads
	dtn::core::GlobalEvent::raise(dtn::core::GlobalEvent::GLOBAL_RELOAD);
}

void switchUser(Configuration &config)
{
    try {
        setuid( config.getUID() );
        IBRCOMMON_LOGGER(info) << "Switching UID to " << config.getUID() << IBRCOMMON_LOGGER_ENDL;
    } catch (Configuration::ParameterNotSetException ex) {

    }

    try {
        setuid( config.getGID() );
        IBRCOMMON_LOGGER(info) << "Switching GID to " << config.getGID() << IBRCOMMON_LOGGER_ENDL;
    } catch (Configuration::ParameterNotSetException ex) {

    }
}

void setGlobalVars(Configuration &config)
{
    //ConfigFile &conf = config.getConfigFile();

    // set the timezone
    dtn::utils::Clock::timezone = config.getTimezone();

    // set local eid
    dtn::core::BundleCore::local = config.getNodename();
    IBRCOMMON_LOGGER(info) << "Local node name: " << config.getNodename() << IBRCOMMON_LOGGER_ENDL;

    try {
    	// new methods for blobs
    	ibrcommon::BLOB::tmppath = config.getPath("blob");
    } catch (Configuration::ParameterNotSetException ex) {

    }

    // set block size limit
    dtn::core::BundleCore::blocksizelimit = config.getLimit("blocksize");
    if (dtn::core::BundleCore::blocksizelimit > 0)
    {
    	IBRCOMMON_LOGGER(info) << "Block size limited to " << dtn::core::BundleCore::blocksizelimit << " bytes" << IBRCOMMON_LOGGER_ENDL;
    }
}

void createBundleStorage(BundleCore &core, Configuration &conf, std::list< dtn::daemon::Component* > &components)
{
	dtn::core::BundleStorage *storage = NULL;

	try {
		// new methods for blobs
		ibrcommon::File path = conf.getPath("storage");

#ifdef HAVE_SQLITE
		dtn::core::SQLiteBundleStorage *sbs = new dtn::core::SQLiteBundleStorage(path, "sqlite.db", conf.getLimit("storage") );
#else
		// create workdir if needed
		if (!path.exists())
		{
			ibrcommon::File::createDirectory(path);
		}

		dtn::core::SimpleBundleStorage *sbs = new dtn::core::SimpleBundleStorage(path, conf.getLimit("storage"));
#endif
		components.push_back(sbs);
		storage = sbs;
	} catch (Configuration::ParameterNotSetException ex) {
		dtn::core::SimpleBundleStorage *sbs = new dtn::core::SimpleBundleStorage(conf.getLimit("storage"));
		components.push_back(sbs);
		storage = sbs;
	}

	// set the storage in the core
	core.setStorage(storage);
}

void createConvergenceLayers(BundleCore &core, Configuration &conf, std::list< dtn::daemon::Component* > &components, dtn::net::IPNDAgent *ipnd)
{
	// get the configuration of the convergence layers
	std::list<Configuration::NetConfig> nets = conf.getInterfaces();

	// create the convergence layers
 	for (std::list<Configuration::NetConfig>::const_iterator iter = nets.begin(); iter != nets.end(); iter++)
	{
		const Configuration::NetConfig &net = (*iter);

		try {
			switch (net.type)
			{
				case Configuration::NetConfig::NETWORK_UDP:
				{
					UDPConvergenceLayer *udpcl = new UDPConvergenceLayer( net.interface, net.port );
					core.addConvergenceLayer(udpcl);
					components.push_back(udpcl);
					if (ipnd != NULL) ipnd->addService(udpcl);

					IBRCOMMON_LOGGER(info) << "UDP ConvergenceLayer added on " << net.interface.getAddress() << ":" << net.port << IBRCOMMON_LOGGER_ENDL;

					break;
				}

				case Configuration::NetConfig::NETWORK_TCP:
				{
					TCPConvergenceLayer *tcpcl = new TCPConvergenceLayer( net.interface, net.port );
					core.addConvergenceLayer(tcpcl);
					components.push_back(tcpcl);
					if (ipnd != NULL) ipnd->addService(tcpcl);

					IBRCOMMON_LOGGER(info) << "TCP ConvergenceLayer added on " << net.interface.getAddress() << ":" << net.port << IBRCOMMON_LOGGER_ENDL;

					break;
				}

#ifdef HAVE_LIBCURL
				case Configuration::NetConfig::NETWORK_HTTP:
				{
					HTTPConvergenceLayer *httpcl = new HTTPConvergenceLayer( net.address );
					core.addConvergenceLayer(httpcl);
					components.push_back(httpcl);

					IBRCOMMON_LOGGER(info) << "HTTP ConvergenceLayer added, Server: " << net.address << IBRCOMMON_LOGGER_ENDL;
					break;
				}
#endif

#ifdef HAVE_LOWPAN_SUPPORT
				case Configuration::NetConfig::NETWORK_LOWPAN:
				{
					LOWPANConvergenceLayer *lowpancl = new LOWPANConvergenceLayer( net.interface, net.port );
					core.addConvergenceLayer(lowpancl);
					components.push_back(lowpancl);
					if (ipnd != NULL) ipnd->addService(lowpancl);

					IBRCOMMON_LOGGER(info) << "LOWPAN ConvergenceLayer added on " << net.interface.getAddress() << ":" << net.port << IBRCOMMON_LOGGER_ENDL;

					break;
				}
#endif

				default:
					break;
			}
		} catch (ibrcommon::SocketException ex) {
			IBRCOMMON_LOGGER(error) << "Failed to add TCP ConvergenceLayer on " << net.interface.getAddress() << ":" << net.port << IBRCOMMON_LOGGER_ENDL;
			IBRCOMMON_LOGGER(error) << "      Error: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
			throw ex;
		}
	}
}

int main(int argc, char *argv[])
{
	// catch process signals
	signal(SIGINT, term);
	signal(SIGTERM, term);
	signal(SIGHUP, reload);

	// create a configuration
	Configuration &conf = Configuration::getInstance();

	// load parameter into the configuration
	conf.params(argc, argv);

	// setup logging capabilities
	{
		// logging options
		unsigned char logopts = ibrcommon::Logger::LOG_TIMESTAMP | ibrcommon::Logger::LOG_LEVEL;

		// error filter
		unsigned int logerr = ibrcommon::Logger::LOGGER_ERR | ibrcommon::Logger::LOGGER_CRIT;

		// logging filter, everything but debug, err and crit
		unsigned int logstd = ~(ibrcommon::Logger::LOGGER_DEBUG | ibrcommon::Logger::LOGGER_ERR | ibrcommon::Logger::LOGGER_CRIT);

		// init syslog
		ibrcommon::Logger::enableSyslog("ibrdtn-daemon", LOG_PID, LOG_DAEMON, ibrcommon::Logger::LOGGER_INFO | ibrcommon::Logger::LOGGER_NOTICE);

		if (!conf.getDebug().quiet())
		{
			// add logging to the cout
			ibrcommon::Logger::addStream(std::cout, logstd, logopts);

			// add logging to the cerr
			ibrcommon::Logger::addStream(std::cerr, logerr, logopts);
		}

		// greeting
		IBRCOMMON_LOGGER(info) << "IBR-DTN daemon " << conf.version() << IBRCOMMON_LOGGER_ENDL;

		// activate debugging
		if (conf.getDebug().enabled() && !conf.getDebug().quiet())
		{
			// init logger
			ibrcommon::Logger::setVerbosity(conf.getDebug().level());

			IBRCOMMON_LOGGER(info) << "debug level set to " << conf.getDebug().level() << IBRCOMMON_LOGGER_ENDL;

			ibrcommon::Logger::addStream(std::cout, ibrcommon::Logger::LOGGER_DEBUG, logopts);
		}
	}

	// load the configuration file
	conf.load();

	// switch the user is requested
	switchUser(conf);

	// set global vars
	setGlobalVars(conf);

	// list of components
	std::list< dtn::daemon::Component* > components;

	// create a notifier if configured
	try {
		components.push_back( new dtn::daemon::Notifier( conf.getNotifyCommand() ) );
	} catch (Configuration::ParameterNotSetException ex) {

	}

	// create the bundle core object
	BundleCore &core = BundleCore::getInstance();

	// create the event switch object
	dtn::core::EventSwitch &esw = dtn::core::EventSwitch::getInstance();

	// create a storage for bundles
	createBundleStorage(core, conf, components);

	// initialize the DiscoveryAgent
	dtn::net::IPNDAgent *ipnd = NULL;

	if (conf.getDiscovery().enabled())
	{
		// get the discovery port
		int disco_port = conf.getDiscovery().port();

		try {
			ipnd = new dtn::net::IPNDAgent( disco_port, conf.getDiscovery().address() );
		} catch (Configuration::ParameterNotFoundException ex) {
			ipnd = new dtn::net::IPNDAgent( disco_port, "255.255.255.255" );
		}

		// collect all interfaces of convergence layer instances
		std::set<ibrcommon::NetInterface> interfaces;

		std::list<Configuration::NetConfig> nets = conf.getInterfaces();
		for (std::list<Configuration::NetConfig>::const_iterator iter = nets.begin(); iter != nets.end(); iter++)
		{
			const Configuration::NetConfig &net = (*iter);
			interfaces.insert(net.interface);
		}

		for (std::set<ibrcommon::NetInterface>::const_iterator iter = interfaces.begin(); iter != interfaces.end(); iter++)
		{
			// add interfaces to discovery
			ipnd->bind(*iter);
		}

		components.push_back(ipnd);
	}
	else
	{
		IBRCOMMON_LOGGER(info) << "Discovery disabled" << IBRCOMMON_LOGGER_ENDL;
	}

	// create the base router
	dtn::routing::BaseRouter *router = new dtn::routing::BaseRouter(core.getStorage());

	// add routing extensions
	switch (conf.getRoutingExtension())
	{
	case Configuration::FLOOD_ROUTING:
	{
		IBRCOMMON_LOGGER(info) << "Using flooding routing extensions" << IBRCOMMON_LOGGER_ENDL;
		dtn::routing::FloodRoutingExtension *flooding = new dtn::routing::FloodRoutingExtension();
		router->addExtension( flooding );
		router->addExtension( new dtn::routing::RetransmissionExtension() );
		break;
	}

	case Configuration::EPIDEMIC_ROUTING:
	{
		IBRCOMMON_LOGGER(info) << "Using epidemic routing extensions" << IBRCOMMON_LOGGER_ENDL;
		dtn::routing::EpidemicRoutingExtension *epidemic = new dtn::routing::EpidemicRoutingExtension();
		router->addExtension( epidemic );
		router->addExtension( new dtn::routing::RetransmissionExtension() );
		if (ipnd != NULL) ipnd->addService(epidemic);
		break;
	}

	default:
		IBRCOMMON_LOGGER(info) << "Using default routing extensions" << IBRCOMMON_LOGGER_ENDL;
		router->addExtension( new dtn::routing::StaticRoutingExtension( conf.getStaticRoutes() ) );
		router->addExtension( new dtn::routing::NeighborRoutingExtension() );
		break;
	}

	components.push_back(router);

	// enable or disable forwarding of bundles
	if (conf.doForwarding())
	{
		IBRCOMMON_LOGGER(info) << "Forwarding of bundles enabled." << IBRCOMMON_LOGGER_ENDL;
		BundleCore::forwarding = true;
	}
	else
	{
		IBRCOMMON_LOGGER(info) << "Forwarding of bundles disabled." << IBRCOMMON_LOGGER_ENDL;
		BundleCore::forwarding = false;
	}

	// initialize all convergence layers
	createConvergenceLayers(core, conf, components, ipnd);

	if (conf.doAPI())
	{
		Configuration::NetConfig lo = conf.getAPIInterface();

		try {
			// instance a API server, first create a socket
			components.push_back( new ApiServer(lo.interface, lo.port) );
		} catch (ibrcommon::SocketException ex) {
			IBRCOMMON_LOGGER(error) << "Unable to bind to " << lo.interface.getAddress() << ":" << lo.port << ". API not initialized!" << IBRCOMMON_LOGGER_ENDL;
			exit(-1);
		}
	}
	else
	{
		IBRCOMMON_LOGGER(info) << "API disabled" << IBRCOMMON_LOGGER_ENDL;
	}

	// create a statistic logger if configured
	if (conf.getStatistic().enabled())
	{
		try {
			if (conf.getStatistic().type() == "stdout")
			{
				components.push_back( new StatisticLogger( dtn::daemon::StatisticLogger::LOGGER_STDOUT, conf.getStatistic().interval() ) );
			}
			else if (conf.getStatistic().type() == "syslog")
			{
				components.push_back( new StatisticLogger( dtn::daemon::StatisticLogger::LOGGER_SYSLOG, conf.getStatistic().interval() ) );
			}
			else if (conf.getStatistic().type() == "plain")
			{
				components.push_back( new StatisticLogger( dtn::daemon::StatisticLogger::LOGGER_FILE_PLAIN, conf.getStatistic().interval(), conf.getStatistic().logfile() ) );
			}
			else if (conf.getStatistic().type() == "csv")
			{
				components.push_back( new StatisticLogger( dtn::daemon::StatisticLogger::LOGGER_FILE_CSV, conf.getStatistic().interval(), conf.getStatistic().logfile() ) );
			}
			else if (conf.getStatistic().type() == "stat")
			{
				components.push_back( new StatisticLogger( dtn::daemon::StatisticLogger::LOGGER_FILE_STAT, conf.getStatistic().interval(), conf.getStatistic().logfile() ) );
			}
			else if (conf.getStatistic().type() == "udp")
			{
				components.push_back( new StatisticLogger( dtn::daemon::StatisticLogger::LOGGER_UDP, conf.getStatistic().interval(), conf.getStatistic().address(), conf.getStatistic().port() ) );
			}
		} catch (Configuration::ParameterNotSetException ex) {
			IBRCOMMON_LOGGER(error) << "StatisticLogger: Parameter statistic_file is not set! Fallback to stdout logging." << IBRCOMMON_LOGGER_ENDL;
			components.push_back( new StatisticLogger( dtn::daemon::StatisticLogger::LOGGER_STDOUT, conf.getStatistic().interval() ) );
		}
	}

	// initialize core component
	core.initialize();

	// initialize the event switch
	esw.initialize();

	/**
	 * initialize all components!
	 */
	for (std::list< dtn::daemon::Component* >::iterator iter = components.begin(); iter != components.end(); iter++ )
	{
		(*iter)->initialize();
	}

	// run core component
	core.startup();

	/**
	 * run all components!
	 */
	for (std::list< dtn::daemon::Component* >::iterator iter = components.begin(); iter != components.end(); iter++ )
	{
		(*iter)->startup();
	}

	// Debugger
	Debugger debugger;

	// add echo module
	EchoWorker echo;

	// add DevNull module
	DevNull devnull;

	// announce static nodes, create a list of static nodes
	list<Node> static_nodes = conf.getStaticNodes();

	for (list<Node>::iterator iter = static_nodes.begin(); iter != static_nodes.end(); iter++)
	{
		core.addConnection(*iter);
	}

	// run the event switch loop forever
	esw.loop();

	IBRCOMMON_LOGGER(info) << "shutdown dtn node" << IBRCOMMON_LOGGER_ENDL;

	// send shutdown signal to unbound threads
	dtn::core::GlobalEvent::raise(dtn::core::GlobalEvent::GLOBAL_SHUTDOWN);

	/**
	 * terminate all components!
	 */
	for (std::list< dtn::daemon::Component* >::iterator iter = components.begin(); iter != components.end(); iter++ )
	{
		(*iter)->terminate();
	}

	// terminate event switch component
	esw.terminate();

	// terminate core component
	core.terminate();

	// delete all components
	for (std::list< dtn::daemon::Component* >::iterator iter = components.begin(); iter != components.end(); iter++ )
	{
		delete (*iter);
	}

	return 0;
};

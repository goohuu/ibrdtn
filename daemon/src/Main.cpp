#include "config.h"

#include <ibrcommon/data/BLOB.h>
#include <ibrcommon/data/File.h>
#include <ibrcommon/AutoDelete.h>
#include <ibrcommon/net/NetInterface.h>
#include <ibrdtn/utils/Utils.h>
#include <list>

#include "core/BundleCore.h"
#include "core/EventSwitch.h"
#include "core/BundleStorage.h"
#include "core/SimpleBundleStorage.h"
#include "core/SQLiteBundleStorage.h"
#include "core/Node.h"
#include "core/EventSwitch.h"
#include "core/GlobalEvent.h"
#include "core/NodeEvent.h"
#include "core/CustodyManager.h"

#include "routing/BaseRouter.h"
#include "routing/StaticRoutingExtension.h"
#include "routing/NeighborRoutingExtension.h"
#include "routing/EpidemicRoutingExtension.h"
#include "routing/RetransmissionExtension.h"

#include "net/UDPConvergenceLayer.h"
#include "net/TCPConvergenceLayer.h"
#include "net/IPNDAgent.h"

#include "ApiServer.h"
#include "Configuration.h"
#include "EchoWorker.h"
#include "Notifier.h"
#include "DevNull.h"
#include "StatisticLogger.h"
#include "Component.h"

using namespace dtn::core;
using namespace dtn::daemon;
using namespace dtn::utils;
using namespace dtn::net;

#ifdef DO_DEBUG_OUTPUT
#include "Debugger.h"
#endif

#define UNIT_MB * 1048576

// global variable. true if running
bool m_running = true;

void term(int signal)
{
	if (signal >= 1)
	{
		m_running = false;
	}
}

void reload(int signal)
{
	// send shutdown signal to unbound threads
	dtn::core::GlobalEvent::raise(dtn::core::GlobalEvent::GLOBAL_RELOAD);
}

void switchUser(Configuration &config)
{
    try {
        setuid( config.getUID() );
        cout << "Switching UID to " << config.getUID() << endl;
    } catch (Configuration::ParameterNotSetException ex) {

    }

    try {
        setuid( config.getGID() );
        cout << "Switching GID to " << config.getGID() << endl;
    } catch (Configuration::ParameterNotSetException ex) {

    }
}

void setGlobalVars(Configuration &config)
{
    //ConfigFile &conf = config.getConfigFile();

    // set the timezone
    dtn::utils::Utils::timezone = config.getTimezone();

    // set local eid
    dtn::core::BundleCore::local = config.getNodename();
    cout << "Local node name: " << config.getNodename() << endl;

    try {
    	// new methods for blobs
    	ibrcommon::BLOB::tmppath = config.getPath("blob");
    } catch (Configuration::ParameterNotSetException ex) {

    }
}

void createBundleStorage(BundleCore &core, Configuration &conf, std::list< dtn::daemon::Component* > &components)
{
	dtn::core::BundleStorage *storage = NULL;

	try {
		// new methods for blobs
		//storage = new dtn::core::SQLiteBundleStorage(conf.getPath("storage"), "sqlite.db", 2 UNIT_MB);
		ibrcommon::File path = conf.getPath("storage");

		// create workdir if needed
		if (!path.exists())
		{
			ibrcommon::File::createDirectory(path);
		}

		dtn::core::SimpleBundleStorage *sbs = new dtn::core::SimpleBundleStorage(path);
		components.push_back(sbs);
	} catch (Configuration::ParameterNotSetException ex) {
		dtn::core::SimpleBundleStorage *sbs = new dtn::core::SimpleBundleStorage();
		components.push_back(sbs);
		storage = sbs;
	}

	// set the storage in the core
	core.setStorage(storage);
}

void createConvergenceLayers(BundleCore &core, Configuration &conf, std::list< dtn::daemon::Component* > &components, dtn::net::IPNDAgent *ipnd)
{
	// get the configuration of the convergence layers
	list<ibrcommon::NetInterface> nets = conf.getNetInterfaces();

	// create the convergence layers
 	for (list<ibrcommon::NetInterface>::const_iterator iter = nets.begin(); iter != nets.end(); iter++)
	{
		const ibrcommon::NetInterface &net = (*iter);

		try {
			switch (net.getType())
			{
				case ibrcommon::NetInterface::NETWORK_UDP:
				{
					UDPConvergenceLayer *udpcl = new UDPConvergenceLayer( net );
					core.addConvergenceLayer(udpcl);
					components.push_back(udpcl);

					stringstream service; service << "ip=" << net.getAddress() << ";port=" << net.getPort() << ";";
					if (ipnd != NULL) ipnd->addService("udpcl", service.str());

					cout << "UDP ConvergenceLayer added on " << net.getAddress() << ":" << net.getPort() << endl;

					break;
				}

				case ibrcommon::NetInterface::NETWORK_TCP:
				{
					TCPConvergenceLayer *tcpcl = new TCPConvergenceLayer( net );
					core.addConvergenceLayer(tcpcl);
					components.push_back(tcpcl);

					stringstream service; service << "ip=" << net.getAddress() << ";port=" << net.getPort() << ";";
					if (ipnd != NULL) ipnd->addService("tcpcl", service.str());

					cout << "TCP ConvergenceLayer added on " << net.getAddress() << ":" << net.getPort() << endl;

					break;
				}
			}
		} catch (ibrcommon::tcpserver::SocketException ex) {
			cout << "Failed to add TCP ConvergenceLayer on " << net.getAddress() << ":" << net.getPort() << endl;
			cout << "      Error: " << ex.what() << endl;
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
	conf.load(argc, argv);

	// greeting to the console
	cout << "IBR-DTN daemon "; conf.version(cout); cout << endl;

	// greeting to the sydtn::utils::slog
	ibrcommon::slog << ibrcommon::SYSLOG_INFO << "IBR-DTN daemon "; conf.version(ibrcommon::slog); ibrcommon::slog << ", EID: " << conf.getNodename() << endl;

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

	if (conf.doDiscovery())
	{
		ipnd = new dtn::net::IPNDAgent( conf.getDiscoveryInterface() );
		components.push_back(ipnd);
	}

	// create the base router
	dtn::routing::BaseRouter *router = new dtn::routing::BaseRouter(core.getStorage());

	// add routing extensions
	switch (conf.getRoutingExtension())
	{
	case Configuration::EPIDEMIC_ROUTING:
	{
		cout << "Using epidemic routing extensions" << endl;
		dtn::routing::EpidemicRoutingExtension *epidemic = new dtn::routing::EpidemicRoutingExtension();
		router->addExtension( epidemic );
		router->addExtension( new dtn::routing::RetransmissionExtension() );
		if (ipnd != NULL) ipnd->addService(epidemic);
		break;
	}

	default:
		cout << "Using default routing extensions" << endl;
		router->addExtension( new dtn::routing::StaticRoutingExtension( conf.getStaticRoutes() ) );
		router->addExtension( new dtn::routing::NeighborRoutingExtension() );
		break;
	}

	components.push_back(router);

	// initialize all convergence layers
	createConvergenceLayers(core, conf, components, ipnd);

	if (conf.doAPI())
	{
		ibrcommon::NetInterface lo = conf.getAPIInterface();

		try {
			// instance a API server, first create a socket
			components.push_back( new ApiServer(lo) );
		} catch (ibrcommon::tcpserver::SocketException ex) {
			cerr << "Unable to bind to " << lo.getAddress() << ":" << lo.getPort() << ". API not initialized!" << endl;
			exit(-1);
		}
	}

	// create a statistic logger if configured
	if (conf.useStatLogger())
	{
		try {
			if (conf.getStatLogType() == "stdout")
			{
				components.push_back( new StatisticLogger( dtn::daemon::StatisticLogger::LOGGER_STDOUT, conf.getStatLogInterval() ) );
			}
			else if (conf.getStatLogType() == "syslog")
			{
				components.push_back( new StatisticLogger( dtn::daemon::StatisticLogger::LOGGER_SYSLOG, conf.getStatLogInterval() ) );
			}
			else if (conf.getStatLogType() == "plain")
			{
				components.push_back( new StatisticLogger( dtn::daemon::StatisticLogger::LOGGER_FILE_PLAIN, conf.getStatLogInterval(), conf.getStatLogfile() ) );
			}
			else if (conf.getStatLogType() == "cvs")
			{
				components.push_back( new StatisticLogger( dtn::daemon::StatisticLogger::LOGGER_FILE_CVS, conf.getStatLogInterval(), conf.getStatLogfile() ) );
			}
			else if (conf.getStatLogType() == "stat")
			{
				components.push_back( new StatisticLogger( dtn::daemon::StatisticLogger::LOGGER_FILE_STAT, conf.getStatLogInterval(), conf.getStatLogfile() ) );
			}
		} catch (Configuration::ParameterNotSetException ex) {
			std::cout << "StatisticLogger: Parameter statistic_file is not set! Fallback to stdout logging." << std::endl;
			components.push_back( new StatisticLogger( dtn::daemon::StatisticLogger::LOGGER_STDOUT, conf.getStatLogInterval() ) );
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

	// run the event switch
	esw.startup();

	// run core component
	core.startup();

	/**
	 * run all components!
	 */
	for (std::list< dtn::daemon::Component* >::iterator iter = components.begin(); iter != components.end(); iter++ )
	{
		(*iter)->startup();
	}

#ifdef DO_DEBUG_OUTPUT
	// Debugger
	Debugger debugger;
#endif

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

	while (m_running)
	{
		usleep(10000);
	}

	ibrcommon::slog << ibrcommon::SYSLOG_INFO << "shutdown dtn node" << endl;

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

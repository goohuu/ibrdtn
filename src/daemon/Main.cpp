#include "ibrdtn/default.h"

#include "ibrcommon/data/BLOB.h"
#include "ibrcommon/data/File.h"
#include "ibrcommon/AutoDelete.h"

#include "core/BundleCore.h"
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

#include "net/UDPConvergenceLayer.h"
#include "net/TCPConvergenceLayer.h"

#include "daemon/ApiServer.h"
#include "daemon/Configuration.h"
#include "daemon/EchoWorker.h"
#include "daemon/Notifier.h"
#include "daemon/DevNull.h"
#include "net/IPNDAgent.h"

#include "ibrdtn/utils/Utils.h"
#include "ibrcommon/net/NetInterface.h"

using namespace dtn::core;
using namespace dtn::daemon;
using namespace dtn::utils;
using namespace dtn::net;

#ifdef DO_DEBUG_OUTPUT
#include "daemon/Debugger.h"
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

	dtn::daemon::Notifier *notifier = NULL;

	try {
		notifier = new dtn::daemon::Notifier( conf.getNotifyCommand() );
	} catch (Configuration::ParameterNotSetException ex) {

	}

	// create the bundle core object
	BundleCore& core = BundleCore::getInstance();

	// create a storage for bundles
	dtn::core::BundleStorage *storage = NULL;

	try {
		// new methods for blobs
		storage = new dtn::core::SQLiteBundleStorage(conf.getPath("storage"), "sqlite.db", 2 UNIT_MB);
	} catch (Configuration::ParameterNotSetException ex) {
		storage = new dtn::core::SimpleBundleStorage();
	}

	ibrcommon::AutoDelete<dtn::core::BundleStorage> storage_cleaner(storage);

	// set the storage in the core
	core.setStorage(storage);

	// create the base router
	dtn::routing::BaseRouter router(*storage);

	// add routing extensions
	router.addExtension( new dtn::routing::StaticRoutingExtension( conf.getStaticRoutes() ) );
	router.addExtension( new dtn::routing::NeighborRoutingExtension() );

	// get the configuration of the convergence layers
	list<ibrcommon::NetInterface> nets = conf.getNetInterfaces();

	// initialize the DiscoveryAgent
	dtn::net::IPNDAgent *ipnd = NULL;

	if (conf.doDiscovery())
	{
		ipnd = new dtn::net::IPNDAgent( conf.getDiscoveryInterface() );
	}

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
					udpcl->start();
					core.addConvergenceLayer(udpcl);

					stringstream service; service << "ip=" << net.getAddress() << ";port=" << net.getPort() << ";";
					if (ipnd != NULL) ipnd->addService("udpcl", service.str());
					//if (ipnd != NULL) ipnd->addService(udpcl);

					cout << "UDP ConvergenceLayer added on " << net.getAddress() << ":" << net.getPort() << endl;

					break;
				}

				case ibrcommon::NetInterface::NETWORK_TCP:
				{
					TCPConvergenceLayer *tcpcl = new TCPConvergenceLayer( net );
					tcpcl->start();
					core.addConvergenceLayer(tcpcl);

					stringstream service; service << "ip=" << net.getAddress() << ";port=" << net.getPort() << ";";
					if (ipnd != NULL) ipnd->addService("tcpcl", service.str());
					//if (ipnd != NULL) ipnd->addService(tcpcl);

					cout << "TCP ConvergenceLayer added on " << net.getAddress() << ":" << net.getPort() << endl;

					break;
				}
			}
		} catch (ibrcommon::tcpserver::SocketException ex) {
				cout << "Failed to add TCP ConvergenceLayer on " << net.getAddress() << ":" << net.getPort() << endl;
				cout << "      Error: " << ex.what() << endl;
		} catch (dtn::net::UDPConvergenceLayer::SocketException ex) {
				cout << "Failed to add UDP ConvergenceLayer on " << net.getAddress() << ":" << net.getPort() << endl;
				cout << "      Error: " << ex.what() << endl;
		}
	}

#ifdef DO_DEBUG_OUTPUT
	// Debugger
	Debugger debugger;
#endif

	// add echo module
	EchoWorker echo;

	// add DevNull module
	DevNull devnull;

	// start the services
	ibrcommon::JoinableThread *service = NULL;
	if ((service = dynamic_cast<ibrcommon::JoinableThread*>(storage)) != NULL)
	{
		service->start();
	}

	// announce static nodes, create a list of static nodes
	list<Node> static_nodes = conf.getStaticNodes();

	for (list<Node>::iterator iter = static_nodes.begin(); iter != static_nodes.end(); iter++)
	{
		core.addConnection(*iter);
	}

	ApiServer *apiserv = NULL;

	if (conf.doAPI())
	{
		ibrcommon::NetInterface lo = conf.getAPIInterface();

		try {
			// instance a API server, first create a socket
			apiserv = new ApiServer(lo);
			apiserv->start();
		} catch (ibrcommon::tcpserver::SocketException ex) {
			cerr << "Unable to bind to " << lo.getAddress() << ":" << lo.getPort() << ". API not initialized!" << endl;
			exit(-1);
		}
	}

	// Fire up the Discovery Agent
	if (ipnd != NULL) ipnd->start();

	while (m_running)
	{
		usleep(10000);
	}

	if (conf.doAPI())
	{
		delete apiserv;
	}

	ibrcommon::slog << ibrcommon::SYSLOG_INFO << "shutdown dtn node" << endl;

	if (ipnd != NULL) delete ipnd;
	if (notifier != NULL) delete notifier;

	// send shutdown signal to unbound threads
	dtn::core::GlobalEvent::raise(dtn::core::GlobalEvent::GLOBAL_SHUTDOWN);

	// stop the event switch
	dtn::core::EventSwitch::stop();

	return 0;
};

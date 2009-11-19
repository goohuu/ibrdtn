#include "ibrdtn/default.h"

#include "ibrdtn/data/BLOBManager.h"
#include "core/BundleCore.h"
#include "core/BundleStorage.h"
#include "core/SimpleBundleStorage.h"
#include "core/DynamicBundleRouter.h"
#include "core/Node.h"
#include "core/EventSwitch.h"
#include "core/GlobalEvent.h"
#include "core/NodeEvent.h"
#include "core/EventDebugger.h"
#include "core/CustodyManager.h"

#include "net/UDPConvergenceLayer.h"
#include "net/TCPConvergenceLayer.h"

#include "daemon/ApiServer.h"
#include "daemon/Configuration.h"
#include "daemon/EchoWorker.h"
#include "net/IPNDAgent.h"

#include "ibrdtn/utils/Utils.h"
#include "daemon/NetInterface.h"

using namespace dtn::core;
using namespace dtn::daemon;
using namespace dtn::utils;
using namespace dtn::net;

#ifdef DO_DEBUG_OUTPUT
#include "daemon/Debugger.h"
#endif

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
    dtn::core::EventSwitch::raiseEvent(new dtn::core::GlobalEvent(dtn::core::GlobalEvent::GLOBAL_RELOAD));
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
        // assign a directory for blobs
        dtn::blob::BLOBManager::init(config.getPath("blob"));
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
        dtn::utils::slog << dtn::utils::SYSLOG_INFO << "IBR-DTN daemon "; conf.version(dtn::utils::slog); dtn::utils::slog << ", EID: " << conf.getNodename() << endl;

        // switch the user is requested
        switchUser(conf);

        // set global vars
        setGlobalVars(conf);

#ifdef DO_DEBUG_OUTPUT
	// create event debugger
	EventDebugger eventdebugger;
#endif

	// create the bundle core object
	BundleCore& core = BundleCore::getInstance();

	// create a storage for bundles
	SimpleBundleStorage storage;
	core.setStorage(&storage);

	// create a static router
	DynamicBundleRouter router( conf.getStaticRoutes(), storage );

	// get the configuration of the convergence layers
        list<NetInterface> nets = conf.getNetInterfaces();

	// initialize the DiscoveryAgent
	dtn::net::IPNDAgent *ipnd = NULL;

	if (conf.doDiscovery())
        {
            NetInterface disco_interface = conf.getDiscoveryInterface();
            ipnd = new dtn::net::IPNDAgent(disco_interface.getBroadcastAddress(), disco_interface.getPort());
        }

        // create the convergence layers
 	for (list<NetInterface>::const_iterator iter = nets.begin(); iter != nets.end(); iter++)
	{
            const NetInterface &net = (*iter);

            try {
                switch (net.getType())
                {
                    case NetInterface::NETWORK_UDP:
                    {
                        UDPConvergenceLayer *udpcl = new UDPConvergenceLayer( net.getAddress(), net.getPort() );
                        udpcl->start();
                        core.addConvergenceLayer(udpcl);

                        stringstream service; service << "ip=" << net.getAddress() << ";port=" << net.getPort() << ";";
                        if (ipnd != NULL) ipnd->addService("udpcl", service.str());
                        //if (ipnd != NULL) ipnd->addService(udpcl);

                        cout << "UDP ConvergenceLayer added on " << net.getAddress() << ":" << net.getPort() << endl;

                        break;
                    }

                    case NetInterface::NETWORK_TCP:
                    {
                        TCPConvergenceLayer *tcpcl = new TCPConvergenceLayer( net.getAddress(), net.getPort() );
                        tcpcl->start();
                        core.addConvergenceLayer(tcpcl);

                        stringstream service; service << "ip=" << net.getAddress() << ";port=" << net.getPort() << ";";
                        if (ipnd != NULL) ipnd->addService("tcpcl", service.str());
                        //if (ipnd != NULL) ipnd->addService(tcpcl);

                        cout << "TCP ConvergenceLayer added on " << net.getAddress() << ":" << net.getPort() << endl;

                        break;
                    }
                }
            } catch (dtn::utils::tcpserver::SocketException ex) {
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

	// start the services
	storage.start();
	router.start();

	// announce static nodes, create a list of static nodes
        list<Node> static_nodes = conf.getStaticNodes();

        for (list<Node>::iterator iter = static_nodes.begin(); iter != static_nodes.end(); iter++)
        {
            core.addConnection(*iter);
        }

	ApiServer *apiserv = NULL;

	if (conf.doAPI())
	{
		try {
			// instance a API server, first create a socket
			apiserv = new ApiServer("127.0.0.1", 4550);
			apiserv->start();
		} catch (dtn::utils::tcpserver::SocketException ex) {
			cerr << "Unable to bind to 127.0.0.1:4550. API not initialized!" << endl;
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

	dtn::utils::slog << dtn::utils::SYSLOG_INFO << "shutdown dtn node" << endl;

	if (ipnd != NULL) delete ipnd;

	// send shutdown signal to unbound threads
	dtn::core::EventSwitch::raiseEvent(new dtn::core::GlobalEvent(dtn::core::GlobalEvent::GLOBAL_SHUTDOWN));

	return 0;
};

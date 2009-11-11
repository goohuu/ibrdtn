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

using namespace dtn::core;
using namespace dtn::daemon;
using namespace dtn::utils;
using namespace dtn::net;

#ifdef DO_DEBUG_OUTPUT
#include "daemon/Debugger.h"
#endif

bool m_running = true;

void term(int signal)
{
	if (signal >= 1)
	{
		m_running = false;
	}
}

void version()
{
	cout << "Version: " << PACKAGE_VERSION;
#ifdef SVN_REV
	cout << "-r" << SVN_REV;
#endif
}

int main(int argc, char *argv[])
{
	// catch process signals
	signal(SIGINT, term);
	signal(SIGTERM, term);

	// create a configuration
	Configuration &conf = Configuration::getInstance();

	string configurationfile = "config.ini";
	bool api_switch = true;

	for (int i = 0; i < argc; i++)
	{
		string arg = argv[i];

		if (arg == "-c" && argc > i)
		{
			configurationfile = argv[i + 1];
		}

		if (arg == "--noapi")
		{
			cout << "API disabled" << endl;
			api_switch = false;
		}

		if ((arg == "--version") || (arg == "-v"))
		{
			cout << "IBR-DTN "; version(); cout << endl;
			return 0;
		}
	}

	cout << "IBR-DTN Daemon - "; version(); cout << endl;

	// load configuration
	ConfigFile _conf;

	try {
		_conf = ConfigFile(configurationfile);;
		cout << "Configuration: " << configurationfile << endl;
	} catch (ConfigFile::file_not_found ex) {
		cout << "No configuration file found. Using defaults." << endl;
	}

	conf.setConfigFile(_conf);

	// set user id
	if ( _conf.keyExists("user") )
	{
		cout << "Switching UID to " << _conf.read<unsigned int>( "user", 0 ) << endl;
		setuid( _conf.read<unsigned int>( "user", 0 ) );
	}

	// set group id
	if ( _conf.keyExists("group") )
	{
		cout << "Switching GID to " << _conf.read<unsigned int>( "group", 0 ) << endl;
		setgid( _conf.read<unsigned int>( "group", 0 ) );
	}

	// set the timezone
	dtn::utils::Utils::timezone = _conf.read<int>( "timezone", 0 );

	// set local eid
	dtn::core::BundleCore::local = conf.getLocalUri();
	cout << "Local node name: " << conf.getLocalUri() << endl;

#ifdef DO_DEBUG_OUTPUT
	// create event debugger
	EventDebugger eventdebugger;
#endif

	if (_conf.keyExists("local_blobpath"))
	{
		// assign a directory for blobs
		dtn::blob::BLOBManager::init(_conf.read<string>("local_blobpath"));
	}

	// create the bundle core object
	BundleCore& core = BundleCore::getInstance();

	// initialize the DiscoveryAgent
	dtn::net::IPNDAgent ipnd(conf.getDiscoveryAddress(), conf.getDiscoveryPort());

	// create a storage for bundles
	SimpleBundleStorage storage;
	core.setStorage(&storage);

	// create a static router
	DynamicBundleRouter router( conf.getStaticRoutes(), storage );

	// get names of the convergence layers
	vector<string> netlist = conf.getNetList();
	{
		vector<string>::iterator iter = netlist.begin();

		while (iter != netlist.end())
		{
			string key = (*iter);
			string type = conf.getNetType(key);

			try {
				if (type == "udp")
				{
					UDPConvergenceLayer *udpcl = new UDPConvergenceLayer( conf.getNetInterface(key), conf.getNetPort(key) );
					udpcl->start();
					core.addConvergenceLayer(udpcl);

					stringstream service; service << "ip=" << conf.getNetInterface(key) << ";port=" << conf.getNetPort(key) << ";";
					ipnd.addService("udpcl", service.str());
				}
				else if (type == "tcp")
				{
					TCPConvergenceLayer *tcpcl = new TCPConvergenceLayer( conf.getNetInterface(key), conf.getNetPort(key) );
					tcpcl->start();
					core.addConvergenceLayer(tcpcl);

					stringstream service; service << "ip=" << conf.getNetInterface(key) << ";port=" << conf.getNetPort(key) << ";";
					ipnd.addService("tcpcl", service.str());
				}

				cout << "ConvergenceLayer for " << type << " added on " << conf.getNetInterface(key) << ":" << conf.getNetPort(key) << endl;

			} catch (dtn::utils::tcpserver::SocketException ex) {
				cout << "Failed to add ConvergenceLayer for " << type << " on " << conf.getNetInterface(key) << ":" << conf.getNetPort(key) << endl;
				cout << "      Error: " << ex.what() << endl;
			} catch (dtn::net::UDPConvergenceLayer::SocketException ex) {
				cout << "Failed to add ConvergenceLayer for " << type << " on " << conf.getNetInterface(key) << ":" << conf.getNetPort(key) << endl;
				cout << "      Error: " << ex.what() << endl;
			}

			iter++;
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

	// announce static nodes
	{
		// create a list of static nodes
		vector<Node> static_nodes = conf.getStaticNodes();
		vector<Node>::iterator iter = static_nodes.begin();

		while (iter != static_nodes.end())
		{
			core.addConnection(*iter);
			iter++;
		}
	}

	ApiServer *apiserv = NULL;

	if (api_switch)
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
	ipnd.start();

	// init system
	cout << "dtn node ready" << endl;

	while (m_running)
	{
		usleep(10000);
	}

	if (api_switch)
	{
		delete apiserv;
	}

	cout << "shutdown dtn node" << endl;

	// send shutdown signal to unbound threads
	dtn::core::EventSwitch::raiseEvent(new dtn::core::GlobalEvent(dtn::core::GlobalEvent::GLOBAL_SHUTDOWN));

	return 0;
};

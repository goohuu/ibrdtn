#include "config.h"

#include <ibrcommon/data/BLOB.h>
#include <ibrcommon/data/File.h>
#include <ibrcommon/AutoDelete.h>
#include <ibrcommon/net/vinterface.h>
#include <ibrcommon/Logger.h>
#include <ibrcommon/net/LinkManager.h>
#include <ibrdtn/utils/Clock.h>
#include <list>

#include "core/BundleCore.h"
#include "core/EventSwitch.h"
#include "core/BundleStorage.h"
#include "core/MemoryBundleStorage.h"
#include "core/SimpleBundleStorage.h"

#include "core/Node.h"
#include "core/EventSwitch.h"
#include "core/GlobalEvent.h"
#include "core/NodeEvent.h"

#include "routing/BaseRouter.h"
#include "routing/StaticRoutingExtension.h"
#include "routing/NeighborRoutingExtension.h"
#include "routing/epidemic/EpidemicRoutingExtension.h"
#include "routing/flooding/FloodRoutingExtension.h"
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

#ifdef HAVE_EXTENDED_API
#include "api/ExtendedApiServer.h"
#endif

#include "Configuration.h"
#include "EchoWorker.h"
#include "CapsuleWorker.h"
#include "DTNTPWorker.h"
#include "Notifier.h"
#include "DevNull.h"
#include "StatisticLogger.h"
#include "Component.h"

#ifdef WITH_BUNDLE_SECURITY
#include "security/SecurityManager.h"
#include "security/SecurityKeyManager.h"
#endif

#ifdef HAVE_LIBDAEMON
#include <libdaemon/daemon.h>
#include <string.h>
#endif

#include <csignal>
#include <sys/types.h>
#include <syslog.h>
#include <set>
#include <pwd.h>

using namespace dtn::core;
using namespace dtn::daemon;
using namespace dtn::utils;
using namespace dtn::net;

#include "Debugger.h"

#define UNIT_MB * 1048576

/**
 * setup logging capabilities
 */

// logging options
unsigned char logopts = ibrcommon::Logger::LOG_DATETIME | ibrcommon::Logger::LOG_LEVEL;

// error filter
const unsigned char logerr = ibrcommon::Logger::LOGGER_ERR | ibrcommon::Logger::LOGGER_CRIT;

// logging filter, everything but debug, err and crit
const unsigned char logstd = ~(ibrcommon::Logger::LOGGER_DEBUG | ibrcommon::Logger::LOGGER_ERR | ibrcommon::Logger::LOGGER_CRIT);

// syslog filter, everything but DEBUG and NOTICE
const unsigned char logsys = ~(ibrcommon::Logger::LOGGER_DEBUG | ibrcommon::Logger::LOGGER_NOTICE);

// debug off by default
bool _debug = false;

// on interruption do this!
void sighandler(int signal)
{
	switch (signal)
	{
	case SIGTERM:
	case SIGINT:
		dtn::core::GlobalEvent::raise(dtn::core::GlobalEvent::GLOBAL_SHUTDOWN);
		break;
	case SIGUSR1:
		// activate debugging
		// init logger
		ibrcommon::Logger::setVerbosity(99);
		IBRCOMMON_LOGGER(info) << "debug level set to 99" << IBRCOMMON_LOGGER_ENDL;

		if (!_debug)
		{
			ibrcommon::Logger::addStream(std::cout, ibrcommon::Logger::LOGGER_DEBUG, logopts);
			_debug = true;
		}
		break;
	case SIGUSR2:
		// activate debugging
		// init logger
		ibrcommon::Logger::setVerbosity(0);
		IBRCOMMON_LOGGER(info) << "debug level set to 0" << IBRCOMMON_LOGGER_ENDL;
		break;
	case SIGHUP:
		// send shutdown signal to unbound threads
		dtn::core::GlobalEvent::raise(dtn::core::GlobalEvent::GLOBAL_RELOAD);
		break;
	}
}

void switchUser(Configuration &config)
{
	try {
		// get the username if set
		std::string username = config.getUser();

		// resolve the username to a valid user id
		struct passwd *pw = getpwnam(username.c_str());

		if (pw != NULL)
		{
			if (setuid( pw->pw_uid ) < 0) return;
			setgid( pw->pw_gid );

			IBRCOMMON_LOGGER(info) << "Switching user to " << username << IBRCOMMON_LOGGER_ENDL;
			return;
		}
	} catch (const Configuration::ParameterNotSetException&) { }

	try {
		setuid( config.getUID() );
		IBRCOMMON_LOGGER(info) << "Switching UID to " << config.getUID() << IBRCOMMON_LOGGER_ENDL;
	} catch (const Configuration::ParameterNotSetException&) { }

	try {
		setgid( config.getGID() );
		IBRCOMMON_LOGGER(info) << "Switching GID to " << config.getGID() << IBRCOMMON_LOGGER_ENDL;
	} catch (const Configuration::ParameterNotSetException&) { }
}

void setGlobalVars(Configuration &config)
{
    // set the timezone
    dtn::utils::Clock::timezone = config.getTimezone();

    // set local eid
    dtn::core::BundleCore::local = config.getNodename();
    IBRCOMMON_LOGGER(info) << "Local node name: " << config.getNodename() << IBRCOMMON_LOGGER_ENDL;

    try {
    	// new methods for blobs
    	ibrcommon::BLOB::tmppath = config.getPath("blob");

    	// check if the BLOB path exists
    	if (ibrcommon::BLOB::tmppath.exists())
    	{
    		if (ibrcommon::BLOB::tmppath.isDirectory())
    		{
    			IBRCOMMON_LOGGER(info) << "using BLOB path: " << ibrcommon::BLOB::tmppath.getPath() << IBRCOMMON_LOGGER_ENDL;
    		}
    		else
    		{
    			IBRCOMMON_LOGGER(warning) << "BLOB path exists, but is not a directory! Fallback to memory based mode." << IBRCOMMON_LOGGER_ENDL;
    			ibrcommon::BLOB::tmppath = ibrcommon::File();
    		}
    	}
    	else
    	{
    		// try to create the BLOB path
    		ibrcommon::File::createDirectory(ibrcommon::BLOB::tmppath);

    		if (ibrcommon::BLOB::tmppath.exists())
    		{
    			IBRCOMMON_LOGGER(info) << "using BLOB path: " << ibrcommon::BLOB::tmppath.getPath() << IBRCOMMON_LOGGER_ENDL;
    		}
    		else
    		{
    			IBRCOMMON_LOGGER(warning) << "Could not create BLOB path! Fallback to memory based mode." << IBRCOMMON_LOGGER_ENDL;
    			ibrcommon::BLOB::tmppath = ibrcommon::File();
    		}
    	}
    } catch (const Configuration::ParameterNotSetException&) {
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

#ifdef HAVE_SQLITE
	if (conf.getStorage() == "sqlite")
	{
		try {
			// new methods for blobs
			ibrcommon::File path = conf.getPath("storage");

			// create workdir if needed
			if (!path.exists())
			{
				ibrcommon::File::createDirectory(path);
			}

			IBRCOMMON_LOGGER(info) << "using sqlite bundle storage in " << path.getPath() << IBRCOMMON_LOGGER_ENDL;

			dtn::core::SQLiteBundleStorage *sbs = new dtn::core::SQLiteBundleStorage(path, conf.getLimit("storage") );

			components.push_back(sbs);
			storage = sbs;
		} catch (const Configuration::ParameterNotSetException&) {
			IBRCOMMON_LOGGER(error) << "storage for bundles" << IBRCOMMON_LOGGER_ENDL;
			exit(-1);
		}
	}
#endif

	if ((conf.getStorage() == "simple") || (conf.getStorage() == "default"))
	{
		// default behavior if no bundle storage is set
		try {
			// new methods for blobs
			ibrcommon::File path = conf.getPath("storage");

			// create workdir if needed
			if (!path.exists())
			{
				ibrcommon::File::createDirectory(path);
			}

			IBRCOMMON_LOGGER(info) << "using simple bundle storage in " << path.getPath() << IBRCOMMON_LOGGER_ENDL;

			dtn::core::SimpleBundleStorage *sbs = new dtn::core::SimpleBundleStorage(path, conf.getLimit("storage"), conf.getLimit("storage_buffer"));

			components.push_back(sbs);
			storage = sbs;
		} catch (const Configuration::ParameterNotSetException&) {
			IBRCOMMON_LOGGER(info) << "using bundle storage in memory-only mode" << IBRCOMMON_LOGGER_ENDL;

			dtn::core::MemoryBundleStorage *sbs = new dtn::core::MemoryBundleStorage(conf.getLimit("storage"));
			components.push_back(sbs);
			storage = sbs;
		}
	}

	if (storage == NULL)
	{
		IBRCOMMON_LOGGER(error) << "bundle storage module \"" << conf.getStorage() << "\" do not exists!" << IBRCOMMON_LOGGER_ENDL;
		exit(-1);
	}

	// set the storage in the core
	core.setStorage(storage);
}

void createConvergenceLayers(BundleCore &core, Configuration &conf, std::list< dtn::daemon::Component* > &components, dtn::net::IPNDAgent *ipnd)
{
	// get the configuration of the convergence layers
	const std::list<Configuration::NetConfig> &nets = conf.getNetwork().getInterfaces();

	// local cl map
	std::map<Configuration::NetConfig::NetType, dtn::net::ConvergenceLayer*> _cl_map;

	// create the convergence layers
 	for (std::list<Configuration::NetConfig>::const_iterator iter = nets.begin(); iter != nets.end(); iter++)
	{
		const Configuration::NetConfig &net = (*iter);

		try {
			switch (net.type)
			{
				case Configuration::NetConfig::NETWORK_UDP:
				{
					try {
						UDPConvergenceLayer *udpcl = new UDPConvergenceLayer( net.interface, net.port );
						core.addConvergenceLayer(udpcl);
						components.push_back(udpcl);
						if (ipnd != NULL) ipnd->addService(udpcl);

						IBRCOMMON_LOGGER(info) << "UDP ConvergenceLayer added on " << net.interface.toString() << ":" << net.port << IBRCOMMON_LOGGER_ENDL;
					} catch (const ibrcommon::Exception &ex) {
						IBRCOMMON_LOGGER(error) << "Failed to add UDP ConvergenceLayer on " << net.interface.toString() << ": " << ex.what() << IBRCOMMON_LOGGER_ENDL;
					}

					break;
				}

				case Configuration::NetConfig::NETWORK_TCP:
				{
					// look for an earlier instance of
					std::map<Configuration::NetConfig::NetType, dtn::net::ConvergenceLayer*>::iterator it = _cl_map.find(net.type);

					if (it == _cl_map.end())
					{
						try {
							TCPConvergenceLayer *tcpcl = new TCPConvergenceLayer();
							tcpcl->bind(net.interface, net.port);

							core.addConvergenceLayer(tcpcl);
							components.push_back(tcpcl);
							if (ipnd != NULL) ipnd->addService(tcpcl);
							_cl_map[net.type] = tcpcl;
							IBRCOMMON_LOGGER(info) << "TCP ConvergenceLayer added on " << net.interface.toString() << ":" << net.port << IBRCOMMON_LOGGER_ENDL;
						} catch (const ibrcommon::Exception &ex) {
							IBRCOMMON_LOGGER(error) << "Failed to add TCP ConvergenceLayer on " << net.interface.toString() << ": " << ex.what() << IBRCOMMON_LOGGER_ENDL;
						}
					}
					else
					{
						ConvergenceLayer *cl = it->second;
						TCPConvergenceLayer &tcpcl = dynamic_cast<TCPConvergenceLayer&>(*(cl));
						tcpcl.bind(net.interface, net.port);
					}

					break;
				}

#ifdef HAVE_LIBCURL
				case Configuration::NetConfig::NETWORK_HTTP:
				{
					try {
						HTTPConvergenceLayer *httpcl = new HTTPConvergenceLayer( net.url );
						core.addConvergenceLayer(httpcl);
						components.push_back(httpcl);

						IBRCOMMON_LOGGER(info) << "HTTP ConvergenceLayer added, Server: " << net.url << IBRCOMMON_LOGGER_ENDL;
					} catch (const ibrcommon::Exception &ex) {
						IBRCOMMON_LOGGER(error) << "Failed to add HTTP ConvergenceLayer, Server: " << net.url << ": " << ex.what() << IBRCOMMON_LOGGER_ENDL;
					}
					break;
				}
#endif

#ifdef HAVE_LOWPAN_SUPPORT
				case Configuration::NetConfig::NETWORK_LOWPAN:
				{
					try {
						LOWPANConvergenceLayer *lowpancl = new LOWPANConvergenceLayer( net.interface, net.port );
						core.addConvergenceLayer(lowpancl);
						components.push_back(lowpancl);
						if (ipnd != NULL) ipnd->addService(lowpancl);

						IBRCOMMON_LOGGER(info) << "LOWPAN ConvergenceLayer added on " << net.interface.toString() << ":" << net.port << IBRCOMMON_LOGGER_ENDL;
					} catch (const ibrcommon::Exception &ex) {
						IBRCOMMON_LOGGER(error) << "Failed to add LOWPAN ConvergenceLayer on " << net.interface.toString() << ": " << ex.what() << IBRCOMMON_LOGGER_ENDL;
					}

					break;
				}
#endif

				default:
					break;
			}
		} catch (const std::exception &ex) {
			IBRCOMMON_LOGGER(error) << "Error: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
		}
	}
}

int __daemon_run(Configuration &conf)
{
	// catch process signals
	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);
	signal(SIGHUP, sighandler);
	signal(SIGQUIT, sighandler);
	signal(SIGUSR1, sighandler);
	signal(SIGUSR2, sighandler);

	// enable timestamps in logging if requested
	if (conf.getLogger().display_timestamps())
	{
		logopts = (~(ibrcommon::Logger::LOG_DATETIME) & logopts) | ibrcommon::Logger::LOG_TIMESTAMP;
	}

	// init syslog
	ibrcommon::Logger::enableAsync(); // enable asynchronous logging feature (thread-safe)
	ibrcommon::Logger::enableSyslog("ibrdtn-daemon", LOG_PID, LOG_DAEMON, logsys);

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

		_debug = true;
	}

	// load the configuration file
	conf.load();

	// logfile output stream
	ofstream logfile_stream;

	try {
		const ibrcommon::File &lf = conf.getLogger().getLogfile();
		logfile_stream.open(lf.getPath().c_str(), std::ios::out | std::ios::app);
		logfile_stream << "IBR-DTN daemon " << conf.version() << std::endl;
		ibrcommon::Logger::addStream(logfile_stream, ~(ibrcommon::Logger::LOGGER_DEBUG), logopts);
		IBRCOMMON_LOGGER(info) << "use logfile for output: " << lf.getPath() << IBRCOMMON_LOGGER_ENDL;
	} catch (const Configuration::ParameterNotSetException&) { };

	// switch the user is requested
	switchUser(conf);

	// set global vars
	setGlobalVars(conf);

#ifdef WITH_BUNDLE_SECURITY
	const dtn::daemon::Configuration::Security &sec = conf.getSecurity();

	if (sec.enabled())
	{
		// initialize the key manager for the security extensions
		dtn::security::SecurityKeyManager::getInstance().initialize( sec.getPath(), sec.getCA(), sec.getKey() );
	}
#endif

	// list of components
	std::list< dtn::daemon::Component* > components;

	// create a notifier if configured
	try {
		components.push_back( new dtn::daemon::Notifier( conf.getNotifyCommand() ) );
	} catch (const Configuration::ParameterNotSetException&) {

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
		bool multicast = false;

		// collect all interfaces of convergence layer instances
		std::set<ibrcommon::vinterface> interfaces;

		const std::list<Configuration::NetConfig> &nets = conf.getNetwork().getInterfaces();
		for (std::list<Configuration::NetConfig>::const_iterator iter = nets.begin(); iter != nets.end(); iter++)
		{
			const Configuration::NetConfig &net = (*iter);
			interfaces.insert(net.interface);
		}

		try {
			const ibrcommon::vaddress addr = conf.getDiscovery().address();
			multicast = addr.isMulticast();
			ipnd = new dtn::net::IPNDAgent( disco_port, addr );
		} catch (const Configuration::ParameterNotFoundException&) {
			ipnd = new dtn::net::IPNDAgent( disco_port, ibrcommon::vaddress(ibrcommon::vaddress::VADDRESS_INET, "255.255.255.255") );
		}

		for (std::set<ibrcommon::vinterface>::const_iterator iter = interfaces.begin(); iter != interfaces.end(); iter++)
		{
			const ibrcommon::vinterface &i = (*iter);
			if (i.empty() && multicast)
			{
				IBRCOMMON_LOGGER(warning) << "Multicast discovery will not work with bind on ANY interfaces." << IBRCOMMON_LOGGER_ENDL;
			}
			else
			{
				// add interfaces to discovery
				ipnd->bind(*iter);
			}
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
	switch (conf.getNetwork().getRoutingExtension())
	{
	case Configuration::FLOOD_ROUTING:
	{
		IBRCOMMON_LOGGER(info) << "Using flooding routing extensions" << IBRCOMMON_LOGGER_ENDL;
		dtn::routing::FloodRoutingExtension *flooding = new dtn::routing::FloodRoutingExtension();
		router->addExtension( flooding );
		break;
	}

	case Configuration::EPIDEMIC_ROUTING:
	{
		IBRCOMMON_LOGGER(info) << "Using epidemic routing extensions" << IBRCOMMON_LOGGER_ENDL;
		router->addExtension( new dtn::routing::EpidemicRoutingExtension() );
		break;
	}

	default:
		IBRCOMMON_LOGGER(info) << "Using default routing extensions" << IBRCOMMON_LOGGER_ENDL;
		break;
	}

	// add standard routing modules
	router->addExtension( new dtn::routing::StaticRoutingExtension( conf.getNetwork().getStaticRoutes() ) );
	router->addExtension( new dtn::routing::NeighborRoutingExtension() );
	router->addExtension( new dtn::routing::RetransmissionExtension() );

	components.push_back(router);

	// enable or disable forwarding of bundles
	if (conf.getNetwork().doForwarding())
	{
		IBRCOMMON_LOGGER(info) << "Forwarding of bundles enabled." << IBRCOMMON_LOGGER_ENDL;
		BundleCore::forwarding = true;
	}
	else
	{
		IBRCOMMON_LOGGER(info) << "Forwarding of bundles disabled." << IBRCOMMON_LOGGER_ENDL;
		BundleCore::forwarding = false;
	}

	// enable netlink manager (watchdog for network interfaces)
	if (conf.getNetwork().doDynamicRebind())
	{
		ibrcommon::LinkManager::initialize();
	}

	try {
		// initialize all convergence layers
		createConvergenceLayers(core, conf, components, ipnd);
	} catch (const std::exception&) {
		return -1;
	}

	if (conf.doAPI())
	{
		Configuration::NetConfig lo = conf.getAPIInterface();

		try {
			ibrcommon::File socket = conf.getAPISocket();

			try {
				// use unix domain sockets for API
				components.push_back( new ApiServer(socket) );
				IBRCOMMON_LOGGER(info) << "API initialized using unix domain socket: " << socket.getPath() << IBRCOMMON_LOGGER_ENDL;
			} catch (const ibrcommon::vsocket_exception&) {
				IBRCOMMON_LOGGER(error) << "Unable to bind to unix domain socket " << socket.getPath() << ". API not initialized!" << IBRCOMMON_LOGGER_ENDL;
				exit(-1);
			}

		} catch (const Configuration::ParameterNotSetException&) {
			try {
				// instance a API server, first create a socket
				components.push_back( new ApiServer(lo.interface, lo.port) );
				IBRCOMMON_LOGGER(info) << "API initialized using tcp socket: " << lo.interface.toString() << ":" << lo.port << IBRCOMMON_LOGGER_ENDL;
			} catch (const ibrcommon::vsocket_exception&) {
				IBRCOMMON_LOGGER(error) << "Unable to bind to " << lo.interface.toString() << ":" << lo.port << ". API not initialized!" << IBRCOMMON_LOGGER_ENDL;
				exit(-1);
			}
		}

#ifdef HAVE_EXTENDED_API
		try {
			components.push_back( new dtn::api::ExtendedApiServer(lo.interface, lo.port - 1) );
			IBRCOMMON_LOGGER(info) << "Extended API initialized using tcp socket: " << lo.interface.toString() << ":" << (lo.port - 1) << IBRCOMMON_LOGGER_ENDL;
		} catch (const ibrcommon::vsocket_exception&) {
			IBRCOMMON_LOGGER(error) << "Unable to bind to " << lo.interface.toString() << ":" << (lo.port - 1) << ". Extended API not initialized!" << IBRCOMMON_LOGGER_ENDL;
			exit(-1);
		}
#endif
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
		} catch (const Configuration::ParameterNotSetException&) {
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
		IBRCOMMON_LOGGER_DEBUG(20) << "Initialize component " << (*iter)->getName() << IBRCOMMON_LOGGER_ENDL;
		(*iter)->initialize();
	}

	// run core component
	core.startup();

	/**
	 * run all components!
	 */
	for (std::list< dtn::daemon::Component* >::iterator iter = components.begin(); iter != components.end(); iter++ )
	{
		IBRCOMMON_LOGGER_DEBUG(20) << "Startup component " << (*iter)->getName() << IBRCOMMON_LOGGER_ENDL;
		(*iter)->startup();
	}

	// Debugger
	Debugger debugger;

	// add echo module
	EchoWorker echo;

	// add bundle-in-bundle endpoint
	CapsuleWorker capsule;

	// add DT-NTP worker
	DTNTPWorker dtntp;
	if (ipnd != NULL) ipnd->addService(&dtntp);

	// add DevNull module
	DevNull devnull;

	// announce static nodes, create a list of static nodes
	list<Node> static_nodes = conf.getNetwork().getStaticNodes();

	for (list<Node>::iterator iter = static_nodes.begin(); iter != static_nodes.end(); iter++)
	{
		core.addConnection(*iter);
	}

#ifdef HAVE_LIBDAEMON
	if (conf.getDaemon().daemonize())
	{
		/* Send OK to parent process */
		daemon_retval_send(0);
		daemon_log(LOG_INFO, "Sucessfully started");
	}
#endif

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
		IBRCOMMON_LOGGER_DEBUG(20) << "Terminate component " << (*iter)->getName() << IBRCOMMON_LOGGER_ENDL;
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

	// stop the asynchronous logger
	ibrcommon::Logger::stop();

	return 0;
};

static char* __daemon_pidfile__ = NULL;

static const char* __daemon_pid_file_proc__(void) {
	return __daemon_pidfile__;
}

int main(int argc, char *argv[])
{
	// create a configuration
	Configuration &conf = Configuration::getInstance();

	// load parameter into the configuration
	conf.params(argc, argv);

#ifdef HAVE_LIBDAEMON
	if (conf.getDaemon().daemonize())
	{
		int ret = 0;
		pid_t pid;

#ifdef HAVE_DAEMON_RESET_SIGS
		/* Reset signal handlers */
		if (daemon_reset_sigs(-1) < 0) {
			daemon_log(LOG_ERR, "Failed to reset all signal handlers: %s", strerror(errno));
			return 1;
		}

		/* Unblock signals */
		if (daemon_unblock_sigs(-1) < 0) {
			daemon_log(LOG_ERR, "Failed to unblock all signals: %s", strerror(errno));
			return 1;
		}
#endif

		/* Set identification string for the daemon for both syslog and PID file */
		daemon_pid_file_ident = daemon_log_ident = daemon_ident_from_argv0(argv[0]);

		/* set the pid file path */
		try {
			std::string p = conf.getDaemon().getPidFile().getPath();
			__daemon_pidfile__ = new char[p.length() + 1];
			::strcpy(__daemon_pidfile__, p.c_str());
			daemon_pid_file_proc = __daemon_pid_file_proc__;
		} catch (const Configuration::ParameterNotSetException&) { };

		/* Check if we are called with -k parameter */
		if (conf.getDaemon().kill_daemon())
		{
			int ret;

			/* Kill daemon with SIGTERM */

			/* Check if the new function daemon_pid_file_kill_wait() is available, if it is, use it. */
			if ((ret = daemon_pid_file_kill_wait(SIGTERM, 5)) < 0)
				daemon_log(LOG_WARNING, "Failed to kill daemon: %s", strerror(errno));

			return ret < 0 ? 1 : 0;
		}

		/* Check that the daemon is not rung twice a the same time */
		if ((pid = daemon_pid_file_is_running()) >= 0) {
			daemon_log(LOG_ERR, "Daemon already running on PID file %u", pid);
			return 1;
		}

		/* Prepare for return value passing from the initialization procedure of the daemon process */
		if (daemon_retval_init() < 0) {
			daemon_log(LOG_ERR, "Failed to create pipe.");
			return 1;
		}

		/* Do the fork */
		if ((pid = daemon_fork()) < 0) {

			/* Exit on error */
			daemon_retval_done();
			return 1;

		} else if (pid) { /* The parent */
			int ret;

			/* Wait for 20 seconds for the return value passed from the daemon process */
			if ((ret = daemon_retval_wait(20)) < 0) {
				daemon_log(LOG_ERR, "Could not recieve return value from daemon process: %s", strerror(errno));
				return 255;
			}

			//daemon_log(ret != 0 ? LOG_ERR : LOG_INFO, "Daemon returned %i as return value.", ret);
			return ret;

		} else { /* The daemon */
			/* Close FDs */
			if (daemon_close_all(-1) < 0) {
				daemon_log(LOG_ERR, "Failed to close all file descriptors: %s", strerror(errno));

				/* Send the error condition to the parent process */
				daemon_retval_send(1);
				goto finish;
			}

			/* Create the PID file */
			if (daemon_pid_file_create() < 0) {
				daemon_log(LOG_ERR, "Could not create PID file (%s).", strerror(errno));
				daemon_retval_send(2);
				goto finish;
			}

			/* Initialize signal handling */
			if (daemon_signal_init(SIGINT, SIGTERM, SIGQUIT, SIGHUP, SIGUSR1, SIGUSR2, 0) < 0) {
				daemon_log(LOG_ERR, "Could not register signal handlers (%s).", strerror(errno));
				daemon_retval_send(3);
				goto finish;
			}

			ret = __daemon_run(conf);

	finish:
			/* Do a cleanup */
			daemon_log(LOG_INFO, "Exiting...");
			daemon_retval_send(255);
			daemon_signal_done();
			daemon_pid_file_remove();
		}

		return ret;
	} else {
#endif
		// run the daemon
		return __daemon_run(conf);
#ifdef HAVE_LIBDAEMON
	}
#endif
}

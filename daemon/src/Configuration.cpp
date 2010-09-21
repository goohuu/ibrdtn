#include "config.h"
#include "Configuration.h"
#include "ibrdtn/utils/Utils.h"
#include "core/Node.h"
#include "ibrcommon/net/NetInterface.h"
#include <ibrcommon/Logger.h>
#include "net/DiscoveryAnnouncement.h"

using namespace dtn::net;
using namespace dtn::core;
using namespace dtn::utils;
using namespace ibrcommon;

namespace dtn
{
	namespace daemon
	{
		Configuration::NetConfig::NetConfig(std::string n, NetType t, const ibrcommon::NetInterface &i, int p, bool d)
		 : name(n), type(t), interface(i), port(p), discovery(d)
		{
		}

		Configuration::NetConfig::NetConfig(std::string n, NetType t, const std::string &a, int p, bool d)
		 : name(n), type(t), interface("lo"), address(a), port(p), discovery(d)
		{
		}

		Configuration::NetConfig::~NetConfig()
		{
		}

		std::string Configuration::version()
		{
			std::stringstream ss;
			ss << PACKAGE_VERSION;
		#ifdef SVN_REVISION
			ss << " (build " << SVN_REVISION << ")";
		#endif

			return ss.str();
		}

		Configuration::Configuration()
		 : _filename("config.ini"), _default_net("lo"), _use_default_net(false), _doapi(true)
		{
		}

		Configuration::~Configuration()
		{}

		Configuration::Discovery::Discovery()
		 : _enabled(true), _timeout(5) {};

		Configuration::Statistic::Statistic() {};

		Configuration::Debug::Debug()
		 : _enabled(false), _quiet(false), _level(0) {};

		Configuration::Discovery::~Discovery() {};
		Configuration::Statistic::~Statistic() {};
		Configuration::Debug::~Debug() {};

		const Configuration::Discovery& Configuration::getDiscovery() const
		{
			return _disco;
		}

		const Configuration::Statistic& Configuration::getStatistic() const
		{
			return _stats;
		}

		const Configuration::Debug& Configuration::getDebug() const
		{
			return _debug;
		}

		Configuration& Configuration::getInstance()
		{
			static Configuration conf;
			return conf;
		}

		void Configuration::params(int argc, char *argv[])
		{
			for (int i = 0; i < argc; i++)
			{
				std::string arg = argv[i];

				if (arg == "-c" && argc > i)
				{
						_filename = argv[i + 1];
				}

				if (arg == "-i" && argc > i)
				{
						_default_net = ibrcommon::NetInterface(argv[i + 1]);
						_use_default_net = true;
				}

				if (arg == "--noapi")
				{
						_doapi = false;
				}

				if ((arg == "--version") || (arg == "-v"))
				{
						std::cout << "IBR-DTN version: " << version() << std::endl;
						exit(0);
				}

				if (arg == "--nodiscovery")
				{
					_disco._enabled = false;
				}

				if (arg == "-d")
				{
					_debug._enabled = true;
					_debug._level = atoi(argv[i + 1]);
				}

				if (arg == "-q")
				{
					_debug._quiet = true;
				}
			}
		}

		void Configuration::load()
		{
			load(_filename);
		}

		void Configuration::load(string filename)
		{
			try {
					_conf = ibrcommon::ConfigFile(filename);
					_disco.load(_conf);
					_stats.load(_conf);
					_debug.load(_conf);
					IBRCOMMON_LOGGER(info) << "Configuration: " << filename << IBRCOMMON_LOGGER_ENDL;
			} catch (ibrcommon::ConfigFile::file_not_found ex) {
					IBRCOMMON_LOGGER(info) << "Using defaults. To use custom config file use parameter -c configfile." << IBRCOMMON_LOGGER_ENDL;
					_conf = ConfigFile();
			}
		}

		void Configuration::Discovery::load(const ibrcommon::ConfigFile &conf)
		{
			_timeout = conf.read<unsigned int>("discovery_timeout", 5);
		}

		void Configuration::Statistic::load(const ibrcommon::ConfigFile &conf)
		{
		}

		void Configuration::Debug::load(const ibrcommon::ConfigFile &conf)
		{
		}

		bool Configuration::Debug::quiet() const
		{
			return _quiet;
		}

		bool Configuration::Debug::enabled() const
		{
			return _enabled;
		}

		int Configuration::Debug::level() const
		{
			return _level;
		}

		string Configuration::getNodename()
		{
			try {
				return _conf.read<string>("local_uri");
			} catch (ibrcommon::ConfigFile::key_not_found ex) {
				char *hostname_array = new char[64];
				if ( gethostname(hostname_array, 64) != 0 )
				{
					// error
					delete[] hostname_array;
					return "local";
				}

				string hostname(hostname_array);
				delete[] hostname_array;

				stringstream ss;
				ss << "dtn://" << hostname;
				ss >> hostname;

				return hostname;
			}
		}

		std::list<Configuration::NetConfig> Configuration::getInterfaces()
		{
			std::list<NetConfig> ret;

			if (_use_default_net)
			{
				ret.push_back( Configuration::NetConfig("default", Configuration::NetConfig::NETWORK_TCP, _default_net, 4556) );
				return ret;
			}

			try {
				vector<string> nets = dtn::utils::Utils::tokenize(" ", _conf.read<string>("net_interfaces") );
				for (vector<string>::const_iterator iter = nets.begin(); iter != nets.end(); iter++)
				{
					std::string netname = (*iter);

					std::string key_type = "net_" + netname + "_type";
					std::string key_port = "net_" + netname + "_port";
					std::string key_interface = "net_" + netname + "_interface";
					std::string key_address = "net_" + netname + "_address";
					std::string key_discovery = "net_" + netname + "_discovery";

					std::string type_name = _conf.read<string>(key_type, "tcp");
					Configuration::NetConfig::NetType type = Configuration::NetConfig::NETWORK_UNKNOWN;

					if (type_name == "tcp") type = Configuration::NetConfig::NETWORK_TCP;
					if (type_name == "udp") type = Configuration::NetConfig::NETWORK_UDP;
					if (type_name == "http") type = Configuration::NetConfig::NETWORK_HTTP;
					if (type_name == "lowpan") type = Configuration::NetConfig::NETWORK_LOWPAN;

					switch (type)
					{
						case Configuration::NetConfig::NETWORK_HTTP:
						{
							Configuration::NetConfig::NetConfig nc(netname, type,
									_conf.read<std::string>(key_address, "http://localhost/"), 0,
									_conf.read<std::string>(key_discovery, "yes") == "no");

							ret.push_back(nc);
							break;
						}

						default:
						{
							Configuration::NetConfig::NetConfig nc(netname, type,
									ibrcommon::NetInterface(_conf.read<std::string>(key_interface, "lo")),
									_conf.read<unsigned int>(key_port, 4556),
									_conf.read<std::string>(key_discovery, "yes") == "yes");

							ret.push_back(nc);
							break;
						}
					}
				}
			} catch (ConfigFile::key_not_found ex) {
				return ret;
			}

			return ret;
		}

		std::string Configuration::Discovery::address() const throw (ParameterNotFoundException)
		{
			try {
				return Configuration::getInstance()._conf.read<string>("discovery_address");
			} catch (ConfigFile::key_not_found ex) {
				throw ParameterNotFoundException();
			}
		}

		int Configuration::Discovery::port() const
		{
			return Configuration::getInstance()._conf.read<int>("discovery_port", 4551);
		}

		unsigned int Configuration::Discovery::timeout() const
		{
			return _timeout;
		}

		Configuration::NetConfig Configuration::getAPIInterface()
		{
			return Configuration::NetConfig("local", Configuration::NetConfig::NETWORK_TCP, ibrcommon::NetInterface("lo"), 4550);
		}

		list<dtn::routing::StaticRoutingExtension::StaticRoute> Configuration::getStaticRoutes()
		{
			list<dtn::routing::StaticRoutingExtension::StaticRoute> ret;
			string key = "route1";
			unsigned int keynumber = 1;

			while (_conf.keyExists( key ))
			{
				vector<string> route = dtn::utils::Utils::tokenize(" ", _conf.read<string>(key, "dtn:none dtn:none"));
				ret.push_back( dtn::routing::StaticRoutingExtension::StaticRoute( route.front(), route.back() ) );

				keynumber++;
				stringstream ss; ss << "route" << keynumber; ss >> key;
			}

			return ret;
		}

		list<Node> Configuration::getStaticNodes()
		{
			std::list<Node> nodes;

			// read the node count
			int count = 1;

			// initial prefix
			std::string prefix = "static1_";

			while ( _conf.keyExists(prefix + "uri") )
			{
				Node n(Node::NODE_PERMANENT);

				n.setAddress( _conf.read<std::string>(prefix + "address", "127.0.0.1") );
				n.setPort( _conf.read<unsigned int>(prefix + "port", 4556) );
				n.setURI( _conf.read<std::string>(prefix + "uri", "dtn:none") );

				std::string protocol = _conf.read<std::string>(prefix + "proto", "tcp");
				if (protocol == "tcp") n.setProtocol(Node::CONN_TCPIP);
				if (protocol == "udp") n.setProtocol(Node::CONN_UDPIP);
				if (protocol == "zigbee") n.setProtocol(Node::CONN_ZIGBEE);
				if (protocol == "bluetooth") n.setProtocol(Node::CONN_BLUETOOTH);
				if (protocol == "http") n.setProtocol(Node::CONN_HTTP);

				count++;

				std::stringstream prefix_stream;
				prefix_stream << "static" << count << "_";
				prefix = prefix_stream.str();

				nodes.push_back(n);
			}

			return nodes;
		}

		int Configuration::getTimezone()
		{
			return _conf.read<int>( "timezone", 0 );
		}

		ibrcommon::File Configuration::getPath(string name)
		{
			stringstream ss;
			ss << name << "_path";
			string key; ss >> key;

			try {
				return ibrcommon::File(_conf.read<string>(key));
			} catch (ConfigFile::key_not_found ex) {
				throw ParameterNotSetException();
			}
		}

		unsigned int Configuration::getUID()
		{
			try {
				return _conf.read<unsigned int>("user");
			} catch (ConfigFile::key_not_found ex) {
				throw ParameterNotSetException();
			}
		}

		unsigned int Configuration::getGID()
		{
			try {
				return _conf.read<unsigned int>("group");
			} catch (ConfigFile::key_not_found ex) {
				throw ParameterNotSetException();
			}
		}


		bool Configuration::Discovery::enabled() const
		{
			return _enabled;
		}

		bool Configuration::Discovery::announce() const
		{
			return (Configuration::getInstance()._conf.read<int>("discovery_announce", 1) == 1);
		}

		bool Configuration::Discovery::shortbeacon() const
		{
			return (Configuration::getInstance()._conf.read<int>("discovery_short", 0) == 1);
		}

		char Configuration::Discovery::version() const
		{
			return Configuration::getInstance()._conf.read<int>("discovery_version", 2);
		}

		bool Configuration::doAPI()
		{
			return _doapi;
		}

		string Configuration::getNotifyCommand()
		{
			try {
				return _conf.read<string>("notify_cmd");
			} catch (ConfigFile::key_not_found ex) {
				throw ParameterNotSetException();
			}
		}

		Configuration::RoutingExtension Configuration::getRoutingExtension()
		{
			try {
				string mode = _conf.read<string>("routing");
				if ( mode == "epidemic" ) return EPIDEMIC_ROUTING;
				if ( mode == "flooding" ) return FLOOD_ROUTING;
				return DEFAULT_ROUTING;
			} catch (ConfigFile::key_not_found ex) {
				return DEFAULT_ROUTING;
			}
		}


		bool Configuration::doForwarding()
		{
			try {
				if (_conf.read<std::string>("routing_forwarding") == "yes")
				{
					return true;
				}
				else
				{
					return false;
				}
			} catch (ConfigFile::key_not_found ex) {
				return true;
			}
		}

		bool Configuration::Statistic::enabled() const
		{
			return Configuration::getInstance()._conf.keyExists("statistic_type");
		}

		ibrcommon::File Configuration::Statistic::logfile() const throw (ParameterNotSetException)
		{
			try {
				return ibrcommon::File(Configuration::getInstance()._conf.read<std::string>("statistic_file"));
			} catch (ConfigFile::key_not_found ex) {
				throw ParameterNotSetException();
			}
		}

		std::string Configuration::Statistic::type() const
		{
			return Configuration::getInstance()._conf.read<std::string>("statistic_type", "stdout");
		}

		unsigned int Configuration::Statistic::interval() const
		{
			return Configuration::getInstance()._conf.read<unsigned int>("statistic_interval", 300);
		}

		std::string Configuration::Statistic::address() const
		{
			return Configuration::getInstance()._conf.read<std::string>("statistic_address", "127.0.0.1");
		}

		unsigned int Configuration::Statistic::port() const
		{
			return Configuration::getInstance()._conf.read<unsigned int>("statistic_port", 1234);
		}

		size_t Configuration::getLimit(std::string suffix)
		{
			std::string unparsed = _conf.read<std::string>("limit_" + suffix, "0");

			std::stringstream ss(unparsed);

			float value; ss >> value;
			char multiplier = 0; ss >> multiplier;

			switch (multiplier)
			{
			default:
				return (size_t)value;
				break;

			case 'G':
				return (size_t)(value * 1000000000);
				break;

			case 'M':
				return (size_t)(value * 1000000);
				break;

			case 'K':
				return (size_t)(value * 1000);
				break;
			}

			return 0;
		}
	}
}

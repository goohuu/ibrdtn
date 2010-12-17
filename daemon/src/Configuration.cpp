#include "config.h"
#include "Configuration.h"
#include "net/DiscoveryAnnouncement.h"
#include "net/DiscoveryAnnouncement.h"
#include "core/Node.h"

#include <ibrdtn/utils/Utils.h>
#include <ibrdtn/utils/Clock.h>

#include <ibrcommon/net/vinterface.h>
#include <ibrcommon/Logger.h>

#ifdef __DEVELOPMENT_ASSERTIONS__
#include <cassert>
#endif

using namespace dtn::net;
using namespace dtn::core;
using namespace dtn::utils;
using namespace ibrcommon;

#ifdef WITH_BUNDLE_SECURITY
#include "security/SecurityManager.h"
#include "security/SecurityRule.h"
#include <ibrdtn/security/SecurityBlock.h>
#include <ibrdtn/security/KeyBlock.h>
#include <openssl/err.h>
#include <openssl/pem.h>

static const std::string _bab_prefix("BAB");
static const std::string _pib_prefix("PIB");
static const std::string _pcb_prefix("PCB");
static const std::string _esb_prefix("ESB");
static const std::string _pubkey_ending("_pubkey.pem");
static const std::string _sec_route_prefix("sec_route");
static const std::string _private_postfix("_private");
static const std::string _public_postfix("_public");
static const std::string _node_postfix("_node");
static const std::string _key_postfix("_key");
static const std::string _deliminiter("_");
#endif

namespace dtn
{
	namespace daemon
	{
		Configuration::NetConfig::NetConfig(std::string n, NetType t, const std::string &u, bool d)
		 : name(n), type(t), url(u), port(0), discovery(d)
		{
		}

		Configuration::NetConfig::NetConfig(std::string n, NetType t, const ibrcommon::vinterface &i, int p, bool d)
		 : name(n), type(t), interface(i), port(p), discovery(d)
		{
		}

		Configuration::NetConfig::NetConfig(std::string n, NetType t, const ibrcommon::vaddress &a, int p, bool d)
		 : name(n), type(t), interface(), address(a), port(p), discovery(d)
		{
		}

		Configuration::NetConfig::NetConfig(std::string n, NetType t, int p, bool d)
		 : name(n), type(t), interface(), port(p), discovery(d)
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
		 : _filename("config.ini"), _doapi(true)
		{
		}

		Configuration::~Configuration()
		{}

		Configuration::Discovery::Discovery()
		 : _enabled(true), _timeout(5) {};

		Configuration::Statistic::Statistic() {};

		Configuration::Debug::Debug()
		 : _enabled(false), _quiet(false), _level(0) {};

		Configuration::Logger::Logger()
		 : _quiet(false), _options(0) {};

		Configuration::Network::Network()
		 : _routing("default"), _forwarding(true), _tcp_nodelay(true), _tcp_chunksize(1024), _default_net("lo"), _use_default_net(false) {};

		Configuration::Security::Security()
		 : _enabled(false)
		{};

		Configuration::Discovery::~Discovery() {};
		Configuration::Statistic::~Statistic() {};
		Configuration::Debug::~Debug() {};
		Configuration::Logger::~Logger() {};
		Configuration::Network::~Network() {};

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

		const Configuration::Logger& Configuration::getLogger() const
		{
			return _logger;
		}

		const Configuration::Network& Configuration::getNetwork() const
		{
			return _network;
		}

		const Configuration::Security& Configuration::getSecurity() const
		{
			return _security;
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
				std::string arg(argv[i]);

				if (arg == "-c" && argc > i)
				{
						_filename = argv[i + 1];
				}

				if (arg == "-i" && argc > i)
				{
						_network._default_net = ibrcommon::vinterface(argv[i + 1]);
						_network._use_default_net = true;
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

				if (arg == "--badclock")
				{
					dtn::utils::Clock::badclock = true;
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

				if ((arg == "--help") || (arg == "-h"))
				{
						std::cout << "IBR-DTN version: " << version() << std::endl;
						std::cout << "Syntax: dtnd [options]"  << std::endl;
						std::cout << " -h|--help       display this text" << std::endl;
						std::cout << " -c <file>       set a configuration file" << std::endl;
						std::cout << " -i <interface>  interface to bind on (e.g. eth0)" << std::endl;
						std::cout << " -d <level>      enable debugging and set a verbose level" << std::endl;
						std::cout << " -q              enables the quiet mode (no logging to the console)" << std::endl;
						std::cout << " --noapi         disable API module" << std::endl;
						std::cout << " --nodiscovery   disable discovery module" << std::endl;
						std::cout << " --badclock      assume a bad clock on the system (use AgeBlock)" << std::endl;
						exit(0);
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
				// load main configuration
				_conf = ibrcommon::ConfigFile(filename);
				_filename = filename;

				IBRCOMMON_LOGGER(info) << "Configuration: " << filename << IBRCOMMON_LOGGER_ENDL;
			} catch (ibrcommon::ConfigFile::file_not_found ex) {
				IBRCOMMON_LOGGER(info) << "Using default settings. Call with --help for options." << IBRCOMMON_LOGGER_ENDL;
				_conf = ConfigFile();
			}

			// load all configuration extensions
			_disco.load(_conf);
			_stats.load(_conf);
			_debug.load(_conf);
			_logger.load(_conf);
			_network.load(_conf);
			_security.load(_conf);
		}

		void Configuration::Discovery::load(const ibrcommon::ConfigFile &conf)
		{
			_timeout = conf.read<unsigned int>("discovery_timeout", 5);
		}

		void Configuration::Statistic::load(const ibrcommon::ConfigFile&)
		{
		}

		void Configuration::Logger::load(const ibrcommon::ConfigFile&)
		{
		}

		void Configuration::Debug::load(const ibrcommon::ConfigFile&)
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

		const std::list<Configuration::NetConfig>& Configuration::Network::getInterfaces() const
		{
			return _interfaces;
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
			return Configuration::NetConfig("local", Configuration::NetConfig::NETWORK_TCP, ibrcommon::vinterface("lo"), 4550);
		}

		ibrcommon::File Configuration::getAPISocket()
		{
			try {
				return ibrcommon::File(_conf.read<std::string>("api_socket"));
			} catch (const ConfigFile::key_not_found&) {
				throw ParameterNotSetException();
			}
		}

		std::string Configuration::getStorage() const
		{
			return _conf.read<std::string>("storage", "default");
		}

		void Configuration::Network::load(const ibrcommon::ConfigFile &conf)
		{
			/**
			 * Load static routes
			 */
			_static_routes.clear();

			string key = "route1";
			unsigned int keynumber = 1;

			while (conf.keyExists( key ))
			{
				vector<string> route = dtn::utils::Utils::tokenize(" ", conf.read<string>(key, "dtn:none dtn:none"));
				_static_routes.push_back( dtn::routing::StaticRoutingExtension::StaticRoute( route.front(), route.back() ) );

				keynumber++;
				stringstream ss; ss << "route" << keynumber; ss >> key;
			}

			/**
			 * load static nodes
			 */
			// read the node count
			int count = 1;

			// initial prefix
			std::string prefix = "static1_";

			while ( conf.keyExists(prefix + "uri") )
			{
				Node n(Node::NODE_PERMANENT);

				n.setAddress( conf.read<std::string>(prefix + "address", "127.0.0.1") );
				n.setPort( conf.read<unsigned int>(prefix + "port", 4556) );
				n.setURI( conf.read<std::string>(prefix + "uri", "dtn:none") );
				n.setConnectImmediately( conf.read<std::string>(prefix + "immediately", "no") == "yes" );

				std::string protocol = conf.read<std::string>(prefix + "proto", "tcp");
				if (protocol == "tcp") n.setProtocol(Node::CONN_TCPIP);
				if (protocol == "udp") n.setProtocol(Node::CONN_UDPIP);
				if (protocol == "zigbee") n.setProtocol(Node::CONN_ZIGBEE);
				if (protocol == "bluetooth") n.setProtocol(Node::CONN_BLUETOOTH);
				if (protocol == "http") n.setProtocol(Node::CONN_HTTP);

				count++;

				std::stringstream prefix_stream;
				prefix_stream << "static" << count << "_";
				prefix = prefix_stream.str();

				_nodes.push_back(n);
			}

			/**
			 * get routing extension
			 */
			_routing = conf.read<string>("routing", "default");

			/**
			 * get the routing extension
			 */
			_forwarding = (conf.read<std::string>("routing_forwarding", "yes") == "yes");

			/**
			 * get network interfaces
			 */
			_interfaces.clear();

			if (_use_default_net)
			{
				_interfaces.push_back( Configuration::NetConfig("default", Configuration::NetConfig::NETWORK_TCP, _default_net, 4556) );
			}
			else try
			{
				vector<string> nets = dtn::utils::Utils::tokenize(" ", conf.read<string>("net_interfaces") );
				for (vector<string>::const_iterator iter = nets.begin(); iter != nets.end(); iter++)
				{
					std::string netname = (*iter);

					std::string key_type = "net_" + netname + "_type";
					std::string key_port = "net_" + netname + "_port";
					std::string key_interface = "net_" + netname + "_interface";
					std::string key_address = "net_" + netname + "_address";
					std::string key_discovery = "net_" + netname + "_discovery";

					std::string type_name = conf.read<string>(key_type, "tcp");
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
									conf.read<std::string>(key_address, "http://localhost/"),
									conf.read<std::string>(key_discovery, "yes") == "no");

							_interfaces.push_back(nc);
							break;
						}

						default:
						{
							int port = conf.read<int>(key_port, 4556);
							bool discovery = (conf.read<std::string>(key_discovery, "yes") == "yes");

							try {
								ibrcommon::vinterface interface(conf.read<std::string>(key_interface));
								Configuration::NetConfig::NetConfig nc(netname, type, interface, port, discovery);
								_interfaces.push_back(nc);
							} catch (ConfigFile::key_not_found ex) {
								ibrcommon::vaddress addr;
								Configuration::NetConfig::NetConfig nc(netname, type, addr, port, discovery);
								_interfaces.push_back(nc);
							}

							break;
						}
					}
				}
			} catch (ConfigFile::key_not_found ex) {
				// stop the one network is not found.
			}

			/**
			 * TCP options
			 */
			_tcp_nodelay = (conf.read<std::string>("tcp_nodelay", "yes") == "yes");
			_tcp_chunksize = conf.read<unsigned int>("tcp_chunksize", 1024);
		}

		const list<dtn::routing::StaticRoutingExtension::StaticRoute>& Configuration::Network::getStaticRoutes() const
		{
			return _static_routes;
		}

		const std::list<Node>& Configuration::Network::getStaticNodes() const
		{
			return _nodes;
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

		Configuration::RoutingExtension Configuration::Network::getRoutingExtension() const
		{
			if ( _routing == "epidemic" ) return EPIDEMIC_ROUTING;
			if ( _routing == "flooding" ) return FLOOD_ROUTING;
			return DEFAULT_ROUTING;
		}


		bool Configuration::Network::doForwarding() const
		{
			return _forwarding;
		}

		bool Configuration::Network::getTCPOptionNoDelay() const
		{
			return _tcp_nodelay;
		}

		size_t Configuration::Network::getTCPChunkSize() const
		{
			return _tcp_chunksize;
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

		void Configuration::Security::load(const ibrcommon::ConfigFile &conf)
		{
#ifdef WITH_BUNDLE_SECURITY
			// enable security if the security path is set
			try {
				_path = conf.read<std::string>("security_path");

				if (!_path.exists())
				{
					ibrcommon::File::createDirectory(_path);
				}

				_enabled = true;
			} catch (const ibrcommon::ConfigFile::key_not_found&) {
				return;
			}

			// load level
			_level = Level(conf.read<int>("security_level", 0));

			// load CA path
			try {
				_ca = conf.read<std::string>("security_ca");

				if (!_ca.exists())
				{
					IBRCOMMON_LOGGER(warning) << "CA file " << _ca.getPath() << " does not exists!" << IBRCOMMON_LOGGER_ENDL;
				}
			} catch (const ibrcommon::ConfigFile::key_not_found&) {
			}

			// load rules
			// read the node count
			int count = 0;

			// initial prefix
			std::string _prefix(_sec_route_prefix);
			_prefix.append("0");

			while ( conf.keyExists(_prefix) )
			{
				std::string route(conf.read<string>(_prefix));
				_rules.push_back(route);

				count++;

				std::stringstream prefix_stream;
				prefix_stream << _sec_route_prefix << count;
				_prefix = prefix_stream.str();
			}
#endif
		};

		Configuration::Security::~Security() {};

		bool Configuration::Security::enabled() const
		{
			return _enabled;
		}

#ifdef WITH_BUNDLE_SECURITY
		const ibrcommon::File& Configuration::Security::getPath() const
		{
			return _path;
		}

		const ibrcommon::File& Configuration::Security::getCA() const
		{
			return _ca;
		}

		Configuration::Security::Level Configuration::Security::getLevel() const
		{
			return _level;
		}

		const std::list<dtn::security::SecurityRule>& Configuration::Security::getSecurityRules() const
		{
			return _rules;
		}

//		bool Configuration::Security::takeRule(const dtn::security::RuleBlock& rule)
//		{
//			bool result = false;
//			switch (rule.getAction())
//			{
//				case dtn::security::RuleBlock::ADD:
//					result = addSecurityRuleToConfiguration(rule);
//					break;
//				case dtn::security::RuleBlock::REMOVE:
//					result = removeSecurityRuleFromConfiguration(rule);
//					break;
//			}
//
//			if (result)
//			{
//				// write the new configuration
//				std::string newconf(_filename);
//				newconf.append("~");
//				std::ofstream out(newconf.c_str());
//				if( !out ) throw ibrcommon::ConfigFile::file_not_found( newconf );
//				out << _conf;
//
//				std::rename(newconf.c_str(), _filename.c_str());
//
//				// reload rules
//				SecurityManager::getInstance().readRoutingTable();
//			}
//
//			return result;
//		}
//
//		std::string Configuration::Security::findAndReplace(std::string string, char * target, char * replacement) const
//		{
//			std::string target_string(target);
//
//			size_t pos = string.find(target_string);
//			while (pos != std::string::npos)
//			{
//				string.replace(pos, target_string.size(), replacement);
//				pos = string.find(target_string);
//			}
//
//			return string;
//		}
//
//		std::string Configuration::Security::getFileNamePublicKey(dtn::data::EID target, SecurityBlock::BLOCK_TYPES type) const
//		{
//			// name format is:
//			// node_scheme + '_' + node_ssp + '_' + type_name + "_pubkey.pem"
//			std::string type_name;
//			switch (type)
//			{
//				case dtn::security::SecurityBlock::BUNDLE_AUTHENTICATION_BLOCK:
//					type_name = _bab_prefix;
//					break;
//				case dtn::security::SecurityBlock::PAYLOAD_INTEGRITY_BLOCK:
//					type_name = _pib_prefix;
//					break;
//				case dtn::security::SecurityBlock::PAYLOAD_CONFIDENTIAL_BLOCK:
//					type_name = _pcb_prefix;
//					break;
//				case dtn::security::SecurityBlock::EXTENSION_SECURITY_BLOCK:
//					type_name = _esb_prefix;
//					break;
//			}
//			std::string filename("");
//			filename.append(target.getScheme())
//				.append(_deliminiter)
//				.append(findAndReplace(target.getNode(), "/", ""))
//				.append(_deliminiter)
//				.append(type_name);
//			if (type != dtn::security::SecurityBlock::BUNDLE_AUTHENTICATION_BLOCK)
//				filename.append(_pubkey_ending);
//			return filename;
//		}
//
//		std::string Configuration::Security::getPublicKey(const dtn::data::EID& eid, dtn::security::SecurityBlock::BLOCK_TYPES bt) const
//		{
//			std::string key(getFilePathPublicKey(eid, bt));
//
//			struct stat stFileInfo;
//			int intStat;
//
//			// Attempt to get the file attributes
//			intStat = stat(key.c_str(),&stFileInfo);
//			if(intStat != 0)
//			{
//				// We were not able to get the file attributes.
//				// This may mean that we don't have permission to
//				// access the folder which contains this file. If you
//				// need to do that level of checking, lookup the
//				// return values of stat which will give you
//				// more details on why stat failed.
//				key = "";
//			}
//
//			return key;
//		}
//
//		dtn::data::EID Configuration::Security::createEIDFromFilename(const std::string& file) const
//		{
//			// first chars until '_' are the scheme, the next chars until '_' are the
//			// ssp
//			size_t scheme_pos = file.find(_deliminiter);
//			size_t ssp_pos = file.find(_deliminiter, scheme_pos+1);
//
//#ifdef __DEVELOPMENT_ASSERTIONS__
//			assert(scheme_pos != std::string::npos && ssp_pos != std::string::npos);
//#endif
//
//			std::string scheme(file.substr(0, scheme_pos));
//			std::string ssp(std::string("//").append(file.substr(scheme_pos+1, ssp_pos - scheme_pos - 1)));
//
//			return dtn::data::EID(scheme, ssp);
//		}
//
//		string Configuration::Security::getConfigurationDirectory() const
//		{
//			std::string filepath("");
//			size_t pos = _filename.rfind('/');
//			if (pos != std::string::npos)
//				filepath.append(_filename.substr(0, pos+1));
//			return filepath;
//		}
//
//		std::string Configuration::Security::getFilePathPublicKey(dtn::data::EID target, SecurityBlock::BLOCK_TYPES type) const
//		{
//			return getConfigurationDirectory().append(getFileNamePublicKey(target, type));
//		}
//
//		bool Configuration::Security::storeKey(dtn::data::EID target, SecurityBlock::BLOCK_TYPES type, const std::string& key)
//		{
//			// read key
//			RSA * rsa_key = dtn::security::KeyBlock::createRSA(key);
//
//			// save as PEM
//			std::string filename(getFilePathPublicKey(target, type));
//			FILE * file = fopen(filename.c_str(), "w");
//			if (file == NULL)
//				return false;
//			PEM_write_RSA_PUBKEY(file, rsa_key);
//			fclose(file);
//
//			RSA_free(rsa_key);
//			return true;
//		}
//
//		bool Configuration::Security::deleteKey(dtn::data::EID target, SecurityBlock::BLOCK_TYPES type)
//		{
//			std::string filename(getFilePathPublicKey(target, type));
//			if (remove(filename.c_str()) == 0)
//				return true;
//			else
//			{
//				std::string error("Error deleting a key ");
//				error.append(filename);
//				perror(error.c_str());
//				return false;
//			}
//		}
//
//		std::pair<std::string, std::string> Configuration::Security::getPrivateAndPublicKey(SecurityBlock::BLOCK_TYPES type) const
//		{
//			std::string prefix;
//			switch (type)
//			{
//				case SecurityBlock::BUNDLE_AUTHENTICATION_BLOCK:
//					prefix = _bab_prefix;
//					break;
//				case SecurityBlock::PAYLOAD_INTEGRITY_BLOCK:
//					prefix = _pib_prefix;
//					break;
//				case SecurityBlock::PAYLOAD_CONFIDENTIAL_BLOCK:
//					prefix = _pcb_prefix;
//					break;
//				case SecurityBlock::EXTENSION_SECURITY_BLOCK:
//					prefix = _esb_prefix;
//					break;
//			}
//
//			// initial prefix
//			std::string _prefix(prefix);
//			_prefix.append("0");
//			if ( _conf.keyExists(_prefix + _private_postfix) )
//			{
//				std::string private_key(_conf.read<string>(_prefix + _private_postfix));
//				std::string public_key(_conf.read<string>(_prefix + _public_postfix));
//
//				if (private_key != "")
//					private_key = getConfigurationDirectory().append(private_key);
//				if (public_key != "")
//					public_key= getConfigurationDirectory().append(public_key);
//
//				return std::pair<string, string>(private_key, public_key);
//			}
//			else
//				return std::pair<string, string>("", "");
//		}
//
//		bool Configuration::Security::addSecurityRuleToConfiguration(const dtn::security::RuleBlock& rule)
//		{
//			std::string rule_string = rule.getRule();
//			dtn::security::RuleBlock::Directions dir = rule.getDirection();
//
//			SecurityRule sec_rule(rule_string);
//			// test if parsing was successfully
//			if (sec_rule.getRules().size() == 0)
//				return false;
//
//			// get the right number of this rule
//			// read the node count
//			int count = 0;
//
//			// initial prefix
//			std::string _prefix(_sec_route_prefix);
//			_prefix.append("0");
//
//			while ( _conf.keyExists(_prefix) )
//			{
//				count++;
//				std::stringstream prefix_stream;
//				prefix_stream << _sec_route_prefix << count;
//				_prefix = prefix_stream.str();
//			}
//
//			// place the new rule, after the other rules in the configuration
//			_conf.add<std::string>(_prefix, rule_string);
//			return true;
//		}
//
//		bool Configuration::Security::removeSecurityRuleFromConfiguration(const dtn::security::RuleBlock& rule)
//		{
//			std::string rule_string = rule.getRule();
//			dtn::security::RuleBlock::Directions dir = rule.getDirection();
//
//			// look after this rule in the config
//			// read the node count
//			int count = 0;
//
//			// initial prefix
//			std::string _prefix(_sec_route_prefix);
//			_prefix.append("0");
//
//			while ( _conf.keyExists(_prefix) )
//			{
//				std::string rule_from_config = _conf.read<string>(_prefix);
//				if (rule_from_config == rule_string)
//					break;
//
//				count++;
//				std::stringstream prefix_stream;
//				prefix_stream << _sec_route_prefix << count;
//				_prefix = prefix_stream.str();
//			}
//
//			// test if rule was found
//			if (!_conf.keyExists(_prefix))
//				return false;
//			else
//			{
//				_conf.remove(_prefix);
//				return true;
//			}
//		}
#endif

		bool Configuration::Logger::quiet() const
		{
			return _quiet;
		}

		unsigned int Configuration::Logger::options() const
		{
			return _options;
		}

		std::ostream& Configuration::Logger::output() const
		{
			return std::cout;
		}
	}
}

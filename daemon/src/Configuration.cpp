#include "Configuration.h"
#include "ibrdtn/utils/Utils.h"
#include "core/Node.h"
#include "ibrcommon/net/NetInterface.h"

using namespace dtn::net;
using namespace dtn::core;
using namespace dtn::utils;
using namespace ibrcommon;

namespace dtn
{
	namespace daemon
	{
                void Configuration::version(std::ostream &stream)
                {
                        stream << PACKAGE_VERSION;
                #ifdef SVN_REV
                        stream << "-r" << SVN_REV;
                #endif
                }

		Configuration::Configuration()
                 : _filename("config.ini"), _default_net("lo"), _use_default_net(false), _doapi(true), _dodiscovery(true)
		{}

		Configuration::~Configuration()
		{}

		Configuration& Configuration::getInstance()
		{
			static Configuration conf;
			return conf;
		}

                void Configuration::load(int argc, char *argv[])
                {
                    for (int i = 0; i < argc; i++)
                    {
                            string arg = argv[i];

                            if (arg == "-c" && argc > i)
                            {
                                    _filename = argv[i + 1];
                            }

                            if (arg == "-i" && argc > i)
                            {
                                    _default_net = argv[i + 1];
                                    _use_default_net = true;
                            }

                            if (arg == "--noapi")
                            {
                                    cout << "API disabled" << endl;
                                    _doapi = false;
                            }

                            if ((arg == "--version") || (arg == "-v"))
                            {
                                    cout << "IBR-DTN version: "; version(cout); cout << endl;
                                    exit(0);
                            }

                            if (arg == "--nodiscovery")
                            {
                                    cout << "Discovery disabled" << endl;
                                    _dodiscovery = false;
                            }
                    }

                    // load the configuration
                    load(_filename);
                }

                void Configuration::load(string filename)
                {
                    try {
                            _conf = ibrcommon::ConfigFile(filename);;
                            cout << "Configuration: " << filename << endl;
                    } catch (ibrcommon::ConfigFile::file_not_found ex) {
                            cout << "Using defaults. To use custom config file use parameter -c configfile." << endl;
                            _conf = ConfigFile();
                    }
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

                NetInterface Configuration::getNetInterface(string name)
                {
                    list<NetInterface> nets = getNetInterfaces();

                    for (list<NetInterface>::iterator iter = nets.begin(); iter != nets.end(); iter++)
                    {
                        if ((*iter).getName() == name)
                        {
                            return (*iter);
                        }
                    }

                    throw ParameterNotFoundException();
                }

                list<NetInterface> Configuration::getNetInterfaces()
                {
                    list<NetInterface> ret;

                    if (_use_default_net)
                    {
                        ret.push_back( NetInterface(NetInterface::NETWORK_TCP, "default", _default_net, 4556) );
                        return ret;
                    }

                    try {
                        vector<string> nets = dtn::utils::Utils::tokenize(" ", _conf.read<string>("net_interfaces") );
                        for (vector<string>::const_iterator iter = nets.begin(); iter != nets.end(); iter++)
                        {
                            string key_type = "net_"; key_type.append(*iter); key_type.append("_type");
                            string key_port = "net_"; key_port.append(*iter); key_port.append("_port");
                            string key_interface = "net_"; key_interface.append(*iter); key_interface.append("_interface");

                            string type_name = _conf.read<string>(key_type, "tcp");
                            NetInterface::NetworkType type = NetInterface::NETWORK_UNKNOWN;

                            if (type_name == "tcp") type = NetInterface::NETWORK_TCP;
                            if (type_name == "udp") type = NetInterface::NETWORK_UDP;

                            string systemname = _conf.read<string>(key_interface, "lo");
                            unsigned int port = _conf.read<unsigned int>(key_port, 4556);

                            NetInterface net(type, (*iter), systemname, port);

                            ret.push_back(net);
                        }
                    } catch (ConfigFile::key_not_found ex) {
                        return ret;
                    }

                    return ret;
                }

		NetInterface Configuration::getDiscoveryInterface()
		{
                    if (_use_default_net)
                    {
                        return NetInterface(NetInterface::NETWORK_UDP, "disco", _default_net, 4551);
                    }

                    try {
                            string interface = _conf.read<string>("discovery_interface");
                            return NetInterface(NetInterface::NETWORK_UDP, "disco", interface, _conf.read<int>("discovery_port", 4551));
                    } catch (ConfigFile::key_not_found ex) {
                    } catch (ParameterNotFoundException ex) {
                    }

                    return NetInterface(NetInterface::NETWORK_UDP, "disco", "255.255.255.255", "255.255.255.255", 4551);
		}

		NetInterface Configuration::getAPIInterface()
		{
			return NetInterface(NetInterface::NETWORK_UDP, "local", 4550);
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
			list<Node> nodes;

			// read the node count
			int count = 1;

			// initial prefix
			string prefix = "static1_";

			while ( _conf.keyExists(prefix + "uri") )
			{
				Node n(PERMANENT);

				n.setAddress( _conf.read<string>(prefix + "address", "127.0.0.1") );
				n.setPort( _conf.read<unsigned int>(prefix + "port", 4556) );
				n.setURI( _conf.read<string>(prefix + "uri", "dtn:none") );

				string protocol = _conf.read<string>(prefix + "proto", "tcp");
				if (protocol == "tcp") n.setProtocol(TCP_CONNECTION);
				if (protocol == "udp") n.setProtocol(UDP_CONNECTION);

				count++;

				stringstream prefix_stream;
				prefix_stream << "static" << count << "_";
				prefix_stream >> prefix;

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


                bool Configuration::doDiscovery()
                {
                    return _dodiscovery;
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
                    	return DEFAULT_ROUTING;
                    } catch (ConfigFile::key_not_found ex) {
                        return DEFAULT_ROUTING;
                    }
                }
	}
}

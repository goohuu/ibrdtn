#include "daemon/Configuration.h"
#include "ibrdtn/utils/Utils.h"
#include "core/Node.h"

using namespace dtn::core;
using namespace dtn::utils;

namespace dtn
{
	namespace daemon
	{
		Configuration::Configuration()
		{}

		Configuration::~Configuration()
		{}

		Configuration& Configuration::getInstance()
		{
			static Configuration conf;
			return conf;
		}

		void Configuration::setConfigFile(ConfigFile &conf)
		{
			m_conf = conf;
		}

		string Configuration::getLocalUri()
		{
			return m_conf.read<string>("local_uri", "dtn:local");
		}

		vector<string> Configuration::getNetList()
		{
			return Utils::tokenize(" ", m_conf.read<string>("net_interfaces", "default") );
		}

		string Configuration::getNetType(const string name)
		{
			stringstream ss;
			ss << "net_" << name << "_type";
			string key;
			ss >> key;

			return m_conf.read<string>(key, "default");
		}

		unsigned int Configuration::getNetPort(const string name)
		{
			stringstream ss;
			ss << "net_" << name << "_port";
			string key;
			ss >> key;

			return m_conf.read<int>(key, 4556);
		}

		string Configuration::getNetInterface(const string name)
		{
			stringstream ss;
			ss << "net_" << name << "_interface";
			string key;
			ss >> key;

			return m_conf.read<string>(key, "0.0.0.0");
		}

		string Configuration::getNetBroadcast(const string name)
		{
			stringstream ss;
			ss << "net_" << name << "_broadcast";
			string key;
			ss >> key;

			return m_conf.read<string>(key, "255.255.255.255");
		}

		unsigned int Configuration::getNetMTU(const string name)
		{
			stringstream ss;
			ss << "net_" << name << "_mtu";
			string key;
			ss >> key;

			return m_conf.read<unsigned int>(key, 1280);
		}

		list<StaticRoute> Configuration::getStaticRoutes()
		{
			list<StaticRoute> ret;
			string key = "route1";
			unsigned int keynumber = 1;

			while (m_conf.keyExists( key ))
			{
				vector<string> route = Utils::tokenize(" ", m_conf.read<string>(key, "dtn:none dtn:none"));
				ret.push_back( StaticRoute( route.front(), route.back() ) );

				keynumber++;
				stringstream ss; ss << "route" << keynumber; ss >> key;
			}

			return ret;
		}

		vector<Node> Configuration::getStaticNodes()
		{
			vector<Node> nodes;

			// read the node count
			int count = 1;

			// initial prefix
			string prefix = "static1_";

			while ( m_conf.keyExists(prefix + "uri") )
			{
				Node n(PERMANENT);

				n.setAddress( m_conf.read<string>(prefix + "address", "127.0.0.1") );
				n.setPort( m_conf.read<unsigned int>(prefix + "port", 4556) );
				n.setURI( m_conf.read<string>(prefix + "uri", "dtn:none") );

				string protocol = m_conf.read<string>(prefix + "proto", "tcp");
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

		unsigned int Configuration::getStorageMaxSize()
		{
			return m_conf.read<unsigned int>("storage_maxsize", 1024);
		}

		bool Configuration::doStorageMerge()
		{
			return m_conf.read<unsigned int>("storage_merge", 0) == 1;
		}

		list<Node> Configuration::getFakeBroadcastNodes()
		{
			list<Node> nodes;

			string key = "emma_broadcast_recv1";
			unsigned int keynumber = 1;

			while (m_conf.keyExists( key ))
			{
				Node node(FLOATING);
				vector<string> node_data = Utils::tokenize(" ", m_conf.read<string>(key, "127.0.0.1 4556"));
				node.setAddress(node_data.front());

				int port;
				stringstream ss_port; ss_port << node_data.back(); ss_port >> port;
				node.setPort(port);

				nodes.push_back(node);

				keynumber++;
				stringstream ss; ss << "emma_broadcast_recv" << keynumber; ss >> key;
			}

			return nodes;
		}

		bool Configuration::doFakeBroadcast()
		{
			return m_conf.read<int>("emma_fake_broadcast", 0) == 1;
		}

		pair<double, double> Configuration::getStaticPosition()
		{
			return make_pair(m_conf.read<double>("gpsd_lat", 0), m_conf.read<double>("gpsd_lon", 0));
		}

		string Configuration::getGPSHost()
		{
			return m_conf.read<string>("gpsd_host", "localhost");
		}

		unsigned int Configuration::getGPSPort()
		{
			return m_conf.read<unsigned int>("gpsd_port", 2947);
		}

		bool Configuration::useGPSDaemon()
		{
			return m_conf.keyExists("gpsd_host");
		}

		bool Configuration::useSQLiteStorage()
		{
			return m_conf.keyExists( "sqlite_database" );
		}

		string Configuration::getSQLiteDatabase()
		{
			return m_conf.read<string>( "sqlite_database", "database.db" );
		}

		bool Configuration::doSQLiteFlush()
		{
			return m_conf.read<int>( "sqlite_flush", 0 );
		}
	}
}

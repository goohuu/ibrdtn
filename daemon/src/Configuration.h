#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "ibrcommon/data/ConfigFile.h"
#include "core/Node.h"
#include "routing/StaticRoutingExtension.h"
#include "ibrcommon/Exceptions.h"
#include "ibrcommon/net/NetInterface.h"

using namespace dtn::net;
using namespace dtn::core;
using namespace dtn::data;

namespace dtn
{
	namespace daemon
	{
		/**
		 * This class contains the hole configuration for the daemon.
		 */
		class Configuration
		{
		private:
			Configuration();
			virtual ~Configuration();

		public:
			class NetConfig
			{
			public:
				enum NetType
				{
					NETWORK_UNKNOWN = 0,
					NETWORK_TCP = 1,
					NETWORK_UDP = 2
				};

				NetConfig(std::string name, NetType type, const ibrcommon::NetInterface &iface, int port, bool discovery = true);
				~NetConfig();

				std::string name;
				NetType type;
				ibrcommon::NetInterface interface;
				int port;
				bool discovery;
			};

			class ParameterNotSetException : ibrcommon::Exception
			{
			};

			class ParameterNotFoundException : ibrcommon::Exception
			{
			};

			static Configuration &getInstance();

			/**
			 * load the configuration from a file
			 */
			void load();
			void load(string filename);

			void params(int argc, char *argv[]);

			/**
			 * Returns the name of the node
			 */
			string getNodename();

			/**
			 * Returns all configured network interfaces
			 */
			std::list<NetConfig> getInterfaces();

			std::string getDiscoveryAddress();
			int getDiscoveryPort();

			Configuration::NetConfig getAPIInterface();

			/**
			 * Returns all static neighboring nodes
			 */
			list<Node> getStaticNodes();

			/**
			 * Returns all static routes
			 */
			list<dtn::routing::StaticRoutingExtension::StaticRoute> getStaticRoutes();

			int getTimezone();

			ibrcommon::File getPath(string name);

			unsigned int getUID();
			unsigned int getGID();

			bool doDiscovery();
			bool doAPI();

			std::string version();

			string getNotifyCommand();

			enum RoutingExtension
			{
				DEFAULT_ROUTING = 0,
				EPIDEMIC_ROUTING = 1
			};

			/**
			 * @return the routing extension to use.
			 */
			RoutingExtension getRoutingExtension();

			/**
			 * Define if forwarding is enabled. If not, only local bundles will be accepted.
			 * @return True, if forwarding is enabled.
			 */
			bool doForwarding();

			/**
			 * @return True, if the statistic logger is activated.
			 */
			bool useStatLogger();

			/**
			 * @return The file for statistic log output.
			 */
			ibrcommon::File getStatLogfile();

			/**
			 * @return The type of the statistic logger.
			 */
			std::string getStatLogType();

			/**
			 * @return The interval for statistic log refresh.
			 */
			unsigned int getStatLogInterval();

			/**
			 * @return The address for UDP statistics
			 */
			std::string getStatAddress();

			/**
			 * @return The port for UDP statistics
			 */
			unsigned int getStatPort();

			/**
			 * @return The debug level as integer value.
			 */
			int getDebugLevel() const;

			/**
			 * @return True, if the daemon should work in debug mode.
			 */
			bool doDebug() const;

			/**
			 * Returns true if the daemon should work in quiet mode.
			 * @return True, if the daemon should be quiet.
			 */
			bool beQuiet() const;

			/**
			 * Returns a limit defined in the configuration file. The given string specify with limit is to return.
			 * If the string is "block", then the value of "limit_block" is returned.
			 * @return A limit in bytes.
			 */
			size_t getLimit(std::string);

		private:
			ibrcommon::ConfigFile _conf;

			string _filename;
			ibrcommon::NetInterface _default_net;
			bool _use_default_net;
			bool _doapi;
			bool _dodiscovery;
			int _debuglevel;
			bool _debug;
			bool _quiet;
		};
	}
}

#endif /*CONFIGURATION_H_*/

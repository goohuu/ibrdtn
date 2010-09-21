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
					NETWORK_UDP = 2,
					NETWORK_HTTP = 3,
					NETWORK_LOWPAN = 4
				};

				NetConfig(std::string name, NetType type, const std::string &address, int port, bool discovery = true);
				NetConfig(std::string name, NetType type, const ibrcommon::NetInterface &iface, int port, bool discovery = true);
				~NetConfig();

				std::string name;
				NetType type;
				ibrcommon::NetInterface interface;
				std::string address;
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

			bool doAPI();

			std::string version();

			string getNotifyCommand();

			enum RoutingExtension
			{
				DEFAULT_ROUTING = 0,
				EPIDEMIC_ROUTING = 1,
				FLOOD_ROUTING = 2
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
			 * Returns a limit defined in the configuration file. The given string specify with limit is to return.
			 * If the string is "block", then the value of "limit_block" is returned.
			 * @return A limit in bytes.
			 */
			size_t getLimit(std::string);

			class Extension
			{
			protected:
				virtual void load(const ibrcommon::ConfigFile &conf) = 0;
			};

			class Discovery : public Configuration::Extension
			{
				friend class Configuration;
			protected:
				Discovery();
				~Discovery();
				void load(const ibrcommon::ConfigFile &conf);

				bool _enabled;
				unsigned int _timeout;

			public:
				bool enabled() const;
				bool announce() const;
				bool shortbeacon() const;
				char version() const;
				std::string address() const throw (ParameterNotFoundException);
				int port() const;
				unsigned int timeout() const;
			};

			class Statistic : public Configuration::Extension
			{
				friend class Configuration;
			protected:
				Statistic();
				~Statistic();
				void load(const ibrcommon::ConfigFile &conf);

			public:
				/**
				 * @return True, if the statistic logger is activated.
				 */
				bool enabled() const;

				/**
				 * @return The file for statistic log output.
				 */
				ibrcommon::File logfile() const throw (ParameterNotSetException);

				/**
				 * @return The type of the statistic logger.
				 */
				std::string type() const;

				/**
				 * @return The interval for statistic log refresh.
				 */
				unsigned int interval() const;

				/**
				 * @return The address for UDP statistics
				 */
				std::string address() const;

				/**
				 * @return The port for UDP statistics
				 */
				unsigned int port() const;
			};

			class Debug : public Configuration::Extension
			{
				friend class Configuration;
			protected:
				Debug();
				~Debug();
				void load(const ibrcommon::ConfigFile &conf);

				bool _enabled;
				bool _quiet;
				int _level;

			public:
				/**
				 * @return The debug level as integer value.
				 */
				int level() const;

				/**
				 * @return True, if the daemon should work in debug mode.
				 */
				bool enabled() const;

				/**
				 * Returns true if the daemon should work in quiet mode.
				 * @return True, if the daemon should be quiet.
				 */
				bool quiet() const;

			};

			const Configuration::Discovery& getDiscovery() const;
			const Configuration::Statistic& getStatistic() const;
			const Configuration::Debug& getDebug() const;

		private:
			ibrcommon::ConfigFile _conf;
			Configuration::Discovery _disco;
			Configuration::Statistic _stats;
			Configuration::Debug _debug;

			string _filename;
			ibrcommon::NetInterface _default_net;
			bool _use_default_net;
			bool _doapi;
		};
	}
}

#endif /*CONFIGURATION_H_*/

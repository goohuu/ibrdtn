#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "ibrcommon/data/ConfigFile.h"
#include "core/Node.h"
#include "routing/StaticRoutingExtension.h"
#include <ibrcommon/Exceptions.h>
#include <ibrcommon/net/vinterface.h>
#include <list>

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

				NetConfig(std::string name, NetType type, const std::string &url, bool discovery = true);
				NetConfig(std::string name, NetType type, const ibrcommon::vaddress &address, int port, bool discovery = true);
				NetConfig(std::string name, NetType type, const ibrcommon::vinterface &iface, int port, bool discovery = true);
				NetConfig(std::string name, NetType type, int port, bool discovery = true);
				virtual ~NetConfig();

				std::string name;
				NetType type;
				std::string url;
				ibrcommon::vinterface interface;
				ibrcommon::vaddress address;
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
			 * Returns the manual timezone difference in hours.
			 * @return A positive or negative number containing the
			 * timezone offset in hours.
			 */
			int getTimezone();

			/**
			 * Generic command to get a specific path. If "name" is
			 * set to "foo" then the parameter "foo_path" is returned.
			 * @param name The prefix of the path to get.
			 * @return The path as file object.
			 */
			ibrcommon::File getPath(string name);

			/**
			 * The "uid" keyword in the configuration can define
			 * a user to work as. If this daemon is started as root
			 * the daemon will switch to the defined user on startup.
			 * @return The UID of the user.
			 */
			unsigned int getUID() const;

			/**
			 * The "gid" keyword in the configuration can define
			 * a group to work as. If this daemon is started as root
			 * the daemon will switch to the defined group on startup.
			 * @return The  GID of the group.
			 */
			unsigned int getGID() const;

			/**
			 * The "user" keyword in the configuration can define
			 * a user to work as. If this daemon is started as root
			 * the daemon will switch to the defined user on startup.
			 * @return The name of the user.
			 */
			const std::string getUser() const;

			/**
			 * Enable/Disable the API interface.
			 * @return True, if the API interface should be enabled.
			 */
			bool doAPI();

			Configuration::NetConfig getAPIInterface();
			ibrcommon::File getAPISocket();

			/**
			 * Get the version of this daemon.
			 * @return The version string.
			 */
			std::string version();

			/**
			 * The keyword "notify_cmd" can define an external application which
			 * is called by some events. This could be used to notify the user
			 * of some events of interest.
			 * @return A path to the notify command.
			 */
			std::string getNotifyCommand();

			/**
			 * Get the type of bundle storage to use.
			 * @return
			 */
			std::string getStorage() const;

			enum RoutingExtension
			{
				DEFAULT_ROUTING = 0,
				EPIDEMIC_ROUTING = 1,
				FLOOD_ROUTING = 2
			};

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
				virtual ~Discovery();
				void load(const ibrcommon::ConfigFile &conf);

				bool _enabled;
				unsigned int _timeout;

			public:
				bool enabled() const;
				bool announce() const;
				bool shortbeacon() const;
				char version() const;
				const ibrcommon::vaddress address() const throw (ParameterNotFoundException);
				int port() const;
				unsigned int timeout() const;
			};

			class Statistic : public Configuration::Extension
			{
				friend class Configuration;
			protected:
				Statistic();
				virtual ~Statistic();
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
				virtual ~Debug();
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

			class Logger : public Configuration::Extension
			{
				friend class Configuration;
			protected:
				Logger();
				virtual ~Logger();
				void load(const ibrcommon::ConfigFile &conf);

				bool _quiet;
				unsigned int _options;
				bool _timestamps;
				ibrcommon::File _logfile;

			public:
				/**
				 * Enable the quiet mode if set to true.
				 * @return True, if quiet mode is set.
				 */
				bool quiet() const;

				/**
				 * Get a logfile for standard logging output
				 * @return
				 */
				const ibrcommon::File& getLogfile() const;

				/**
				 * Get the options for logging.
				 * This is an unsigned integer with bit flags.
				 * 1 = DATETIME
				 * 2 = HOSTNAME
				 * 4 = LEVEL
				 * 8 = TIMESTAMP
				 * @return The options to set as bit field.
				 */
				unsigned int options() const;

				/**
				 * The output stream for the logging output
				 */
				std::ostream &output() const;

				/**
				 * Returns true if the logger display timestamp instead of datetime values.
				 */
				bool display_timestamps() const;
			};

			class Network :  public Configuration::Extension
			{
				friend class Configuration;
			protected:
				Network();
				virtual ~Network();
				void load(const ibrcommon::ConfigFile &conf);

				std::list<dtn::routing::StaticRoutingExtension::StaticRoute> _static_routes;
				std::list<Node> _nodes;
				std::list<NetConfig> _interfaces;
				std::string _routing;
				bool _forwarding;
				bool _tcp_nodelay;
				size_t _tcp_chunksize;
				ibrcommon::vinterface _default_net;
				bool _use_default_net;

			public:
				/**
				 * Returns all configured network interfaces
				 */
				const std::list<NetConfig>& getInterfaces() const;

				/**
				 * Returns all static neighboring nodes
				 */
				const std::list<Node>& getStaticNodes() const;

				/**
				 * Returns all static routes
				 */
				const list<dtn::routing::StaticRoutingExtension::StaticRoute>& getStaticRoutes() const;

				/**
				 * @return the routing extension to use.
				 */
				RoutingExtension getRoutingExtension() const;

				/**
				 * Define if forwarding is enabled. If not, only local bundles will be accepted.
				 * @return True, if forwarding is enabled.
				 */
				bool doForwarding() const;

				/**
				 * @return True, is tcp options NODELAY should be set.
				 */
				bool getTCPOptionNoDelay() const;

				/**
				 * @return The size of TCP chunks for bundles.
				 */
				size_t getTCPChunkSize() const;
			};

			class Security : public Configuration::Extension
			{
				friend class Configuration;
			private:
				bool _enabled;

			protected:
				Security();
				virtual ~Security();
				void load(const ibrcommon::ConfigFile &conf);

			public:
				bool enabled() const;

#ifdef WITH_BUNDLE_SECURITY
				enum Level
				{
					SECURITY_LEVEL_NONE = 0,
					SECURITY_LEVEL_AUTHENTICATED = 1,
					SECURITY_LEVEL_ENCRYPTED = 2
				};

				Level getLevel() const;

				const ibrcommon::File& getPath() const;

				const ibrcommon::File& getCA() const;

				const ibrcommon::File& getKey() const;

				const ibrcommon::File& getBABDefaultKey() const;

			private:
				ibrcommon::File _path;
				Level _level;
				ibrcommon::File _ca;
				ibrcommon::File _key;
				ibrcommon::File _bab_default_key;
#endif
			};

			class Daemon : public Configuration::Extension
			{
				friend class Configuration;
			private:
				bool _daemonize;
				ibrcommon::File _pidfile;
				bool _kill;

			protected:
				Daemon();
				virtual ~Daemon();
				void load(const ibrcommon::ConfigFile &conf);

			public:
				bool daemonize() const;
				const ibrcommon::File& getPidFile() const;
				bool kill_daemon() const;
			};

			class TimeSync : public Configuration::Extension
			{
				friend class Configuration;
			protected:
				TimeSync();
				virtual ~TimeSync();
				void load(const ibrcommon::ConfigFile &conf);

				bool _reference;
				bool _sync;
				bool _discovery;
				int _qot_tick;

			public:
				bool hasReference() const;
				bool syncOnDiscovery() const;
				bool sendDiscoveryAnnouncements() const;
				int getQualityOfTimeTick() const;
			};

			const Configuration::Discovery& getDiscovery() const;
			const Configuration::Statistic& getStatistic() const;
			const Configuration::Debug& getDebug() const;
			const Configuration::Logger& getLogger() const;
			const Configuration::Network& getNetwork() const;
			const Configuration::Security& getSecurity() const;
			const Configuration::Daemon& getDaemon() const;
			const Configuration::TimeSync& getTimeSync() const;

		private:
			ibrcommon::ConfigFile _conf;
			Configuration::Discovery _disco;
			Configuration::Statistic _stats;
			Configuration::Debug _debug;
			Configuration::Logger _logger;
			Configuration::Network _network;
			Configuration::Security _security;
			Configuration::Daemon _daemon;
			Configuration::TimeSync _timesync;

			std::string _filename;
			bool _doapi;
		};
	}
}

#endif /*CONFIGURATION_H_*/

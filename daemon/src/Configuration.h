#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "ibrcommon/data/ConfigFile.h"
#include "core/Node.h"
#include "routing/StaticRoutingExtension.h"
#include "ibrcommon/Exceptions.h"
#include "ibrcommon/net/NetInterface.h"

#ifdef WITH_BUNDLE_SECURITY
#include "security/SecurityRule.h"
#include "ibrdtn/security/RuleBlock.h"
#include "ibrdtn/security/SecurityBlock.h"
using namespace dtn::security;
#endif

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
			 * The "user" keyword in the configuration can define
			 * a user to work as. If this daemon is started as root
			 * the daemon will switch to the defined user on startup.
			 * @return The UID of the user.
			 */
			unsigned int getUID();

			/**
			 * The "group" keyword in the configuration can define
			 * a group to work as. If this daemon is started as root
			 * the daemon will swithc to the defined group on startup.
			 * @return The  GID of the group.
			 */
			unsigned int getGID();

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

			class Logger : public Configuration::Extension
			{
				friend class Configuration;
			protected:
				Logger();
				~Logger();
				void load(const ibrcommon::ConfigFile &conf);

				bool _quiet;
				unsigned int _options;

			public:
				/**
				 * Enable the quiet mode if set to true.
				 * @return True, if quiet mode is set.
				 */
				bool quiet() const;

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
			};

			class Network :  public Configuration::Extension
			{
				friend class Configuration;
			protected:
				Network();
				~Network();
				void load(const ibrcommon::ConfigFile &conf);

				std::list<dtn::routing::StaticRoutingExtension::StaticRoute> _static_routes;
				std::list<Node> _nodes;
				std::list<NetConfig> _interfaces;
				std::string _routing;
				bool _forwarding;
				bool _tcp_nodelay;
				size_t _tcp_chunksize;
				ibrcommon::NetInterface _default_net;
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
			protected:
				Security();
				~Security();
				void load(const ibrcommon::ConfigFile &conf);

			public:
				bool enabled() const;

#ifdef WITH_BUNDLE_SECURITY
				/**
				 Reads all security rules from the configuration.
				 @return a list of all security rules found in the configuration, but
				 instead of parsing each rule and creating a RuleBlock just the string of
				 the rule from the configuration is returned.
				 */
				std::list<std::string> getSecurityRules_string() const;

				/**
				Reads all security rules from the configuration.
				@return a list of all security rules found in the configuration
				*/
				std::list<dtn::security::SecurityRule> getSecurityRules() const;

				/**
				Modifies the configuration according to the rule. It can be to add or to
				remove a security rule.
				@param rule the rule, which rules us
				@return true if a rule has been removed or added, false if nothing
				happened
				*/
				bool takeRule(const dtn::security::RuleBlock&);

				/**
				Returns a reference to the internal ConfigFile object, which manages the
				keys and their values. It also supports reading from and writing to files.
				@return the internal used ConfigFile object
				*/
				const ibrcommon::ConfigFile& getConfigFile() const;

				/**
				 Takes a string and replaces every occurence of target with replacement             *
				 @param string the string to be modified
				 @param target the part of the string, which shall be replaced
				 @param replacement string, which shall be inserted into the position of target
				 @return the new string
				 */
				std::string findAndReplace(std::string string, char * target, char * replacement) const;

				/**
				Creates a filename of the public key of a node with blocktype
				@param target the name of the node
				@param type the type of the security block
				@return the filename of the public key
				*/
				std::string getFileNamePublicKey(dtn::data::EID target, SecurityBlock::BLOCK_TYPES type) const;

				/**
				Returns the filepath of a public key, if it exists or "" if none exists.
				@param eid the name of the node
				@param bt the kind of the security block
				@return the filepath of the public key
				*/
				std::string getPublicKey(const dtn::data::EID&, dtn::security::SecurityBlock::BLOCK_TYPES) const;

				/**
				Takes a filename of a public key and recreates the EID from it
				@param file the file which should be parsed
				@return the EID which was created from the file name
				*/
				dtn::data::EID createEIDFromFilename(const std::string&) const;

				/**
				Returns the path to the configuration directory
				@return the path to the configuration
				*/
				std::string getConfigurationDirectory() const;

				/**
				Creates the path of a public key of a node with blocktype
				@param target the name of the node
				@param type the type of the security block
				@return the filepath of the public key
				*/
				std::string getFilePathPublicKey(dtn::data::EID target, SecurityBlock::BLOCK_TYPES type) const;

				/**
				Saves a key in the key storage.
				@param target node to which this key belongs to
				@param type the type of SecurityBlocks to which this key belongs to
				@param key the key to be saved. It must be in DER format.
				@return true if the key has been added, false if not
				*/
				bool storeKey(dtn::data::EID target, SecurityBlock::BLOCK_TYPES type, const std::string& key);

				/**
				Deletes a key from the key storage.
				@param target node to which this key belongs to
				@param type the type of SecurityBlocks to which this key belongs to
				@return true if a key has been deleted, false if not
				*/
				bool deleteKey(dtn::data::EID target, SecurityBlock::BLOCK_TYPES type);

				/**
				 Searches for the private and public key of a security block
				 @param type the type of the security block
				 @return a pair with the filepath of the private key in the first position
				 and the public key in the second position for the given blocktype
				 */
				std::pair<std::string, std::string> getPrivateAndPublicKey(SecurityBlock::BLOCK_TYPES type) const;

			private:
				/**
				Adds the given Rule to the security rules
				@param rule the rule to be parsed and added
				@return true if the rule has been added, false if not
				*/
				bool addSecurityRuleToConfiguration(const dtn::security::RuleBlock&);

				/**
				Removes the given Rule from the security rules
				@param rule the rule to be removed
				@return false if no rule was removed, true if a rule was removed
				*/
				bool removeSecurityRuleFromConfiguration(const dtn::security::RuleBlock&);

				std::list<std::string> _rules;
#endif
			};

			const Configuration::Discovery& getDiscovery() const;
			const Configuration::Statistic& getStatistic() const;
			const Configuration::Debug& getDebug() const;
			const Configuration::Logger& getLogger() const;
			const Configuration::Network& getNetwork() const;

			// TODO: this should be const!!!
			Configuration::Security& getSecurity();

		private:
			ibrcommon::ConfigFile _conf;
			Configuration::Discovery _disco;
			Configuration::Statistic _stats;
			Configuration::Debug _debug;
			Configuration::Logger _logger;
			Configuration::Network _network;
			Configuration::Security _security;

			std::string _filename;
			bool _doapi;
		};
	}
}

#endif /*CONFIGURATION_H_*/

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
			list<ibrcommon::NetInterface> getNetInterfaces();

			ibrcommon::NetInterface getNetInterface(string name);

			ibrcommon::NetInterface getDiscoveryInterface();
			std::string getDiscoveryAddress();
			int getDiscoveryPort();

			ibrcommon::NetInterface getAPIInterface();

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

			RoutingExtension getRoutingExtension();

			bool useStatLogger();
			std::string getStatLogfile();
			std::string getStatLogType();
			unsigned int getStatLogInterval();

			int getDebugLevel() const;
			bool doDebug() const;

			bool beQuiet() const;

			size_t getLimit(std::string);

		private:
			ibrcommon::ConfigFile _conf;

			string _filename;
			string _default_net;
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

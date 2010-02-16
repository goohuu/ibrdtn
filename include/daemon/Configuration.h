#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "ibrdtn/default.h"
#include "ibrcommon/data/ConfigFile.h"
#include "core/Node.h"
#include "routing/StaticRoutingExtension.h"
#include "ibrdtn/data/Exceptions.h"
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
			class ParameterNotSetException : dtn::exceptions::Exception
			{
			};

			class ParameterNotFoundException : dtn::exceptions::Exception
			{
			};

			static Configuration &getInstance();

			/**
			 * load the configuration from a file
			 */
			void load(string filename);

			void load(int argc, char *argv[]);

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

			void version(std::ostream &stream);

			string getNotifyCommand();

		private:
			ibrcommon::ConfigFile _conf;

			string _filename;
			string _default_net;
			bool _use_default_net;
			bool _doapi;
			bool _dodiscovery;
		};
	}
}

#endif /*CONFIGURATION_H_*/

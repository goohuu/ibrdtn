#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "ibrdtn/default.h"
#include "ConfigFile.h"
#include "core/Node.h"
#include "core/StaticRoute.h"
#include "ibrdtn/data/Exceptions.h"
#include "ibrdtn/utils/NetInterface.h"

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
                        list<NetInterface> getNetInterfaces();

                        NetInterface getNetInterface(string name);

			NetInterface getDiscoveryInterface();
			NetInterface getAPIInterface();

                        /**
                         * Returns all static neighboring nodes
                         */
			list<Node> getStaticNodes();

                        /**
                         * Returns all static routes
                         */
			list<StaticRoute> getStaticRoutes();

                        int getTimezone();

                        string getPath(string name);

                        unsigned int getUID();
                        unsigned int getGID();

                        bool doDiscovery();
                        bool doAPI();

                        void version(std::ostream &stream);

		private:
			ConfigFile _conf;

                        string _filename;
                        string _default_net;
                        bool _use_default_net;
                        bool _doapi;
                        bool _dodiscovery;
		};
	}
}

#endif /*CONFIGURATION_H_*/

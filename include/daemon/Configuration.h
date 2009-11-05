#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "ibrdtn/default.h"
#include "ConfigFile.h"
#include "core/Node.h"
#include "core/StaticRoute.h"

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
			static Configuration &getInstance();

			string getLocalUri();

			vector<string> getNetList();

			string getNetType(const string name = "default");
			unsigned int getNetPort(const string name = "default");
			string getNetInterface(const string name = "default");
			string getNetBroadcast(const string name = "default");
			unsigned int getNetMTU(const string name = "default");

			void setConfigFile(ConfigFile &conf);

			vector<Node> getStaticNodes();
			list<StaticRoute> getStaticRoutes();

			unsigned int getStorageMaxSize();
			bool doStorageMerge();

			list<Node> getFakeBroadcastNodes();
			bool doFakeBroadcast();

			pair<double, double> getStaticPosition();
			string getGPSHost();
			unsigned int getGPSPort();
			bool useGPSDaemon();

			bool useSQLiteStorage();
			string getSQLiteDatabase();
			bool doSQLiteFlush();

		private:
			ConfigFile m_conf;
			bool m_debug;

		};
	}
}

#endif /*CONFIGURATION_H_*/

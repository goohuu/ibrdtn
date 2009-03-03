#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "ConfigFile.h"
#include "core/Node.h"
#include "core/StaticRoute.h"
#include <string>
#include <vector>
#include <list>

using namespace std;
using namespace dtn::core;
using namespace dtn::data;

namespace dtn
{
namespace daemon
{
/**
 * Eine Klasse die sich ausschließlich mir der Konfiguration des
 * DTN Layers befasst.
 */
class Configuration
{
private:
	Configuration();
	~Configuration();

public:
	static Configuration &getInstance();

	//void load(string filename);

	bool getDebug();
	void setDebug(bool value);

	string getLocalUri();

	vector<string> getNetList();

	string getNetType(const string name = "default");
	unsigned int getNetPort(const string name = "default");
	string getNetInterface(const string name = "default");
	string getNetBroadcast(const string name = "default");
	unsigned int getNetMTU(const string name = "default");

	//ConfigFile& getConfigFile();
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

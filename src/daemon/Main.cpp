#include "config.h"

#include "core/BundleCore.h"
#include "core/StaticBundleRouter.h"
#include "core/ConvergenceLayer.h"
#include "core/DummyConvergenceLayer.h"
#include "core/UDPConvergenceLayer.h"
#include "core/TCPConvergenceLayer.h"
#include "core/Node.h"
using namespace dtn::core;

#ifdef HAVE_LIBSQLITE3
#include "core/SQLiteBundleStorage.h"
#endif

#ifdef USE_EMMA_CODE
#include "emma/GPSProvider.h"
#include "emma/GPSConnector.h"
#include "emma/GPSDummy.h"
#include "emma/EmmaConvergenceLayer.h"
#include "emma/MeasurementWorker.h"
#include "emma/MeasurementJob.h"
using namespace emma;
#endif

#include "daemon/Configuration.h"
#include "daemon/EchoWorker.h"
#include "daemon/DefaultDaemon.h"
using namespace dtn::daemon;

#include "utils/Utils.h"
#include "utils/Service.h"
using namespace dtn::utils;

#include <vector>
#include <iostream>
#include <csignal>
#include <unistd.h>
#include <sys/types.h>

#ifdef DO_DEBUG_OUTPUT
#include "daemon/Debugger.h"
#endif

#ifdef HAVE_LIBLUA5_1
#include "daemon/LuaCore.h"
#endif

bool m_running = true;

void term(int signal)
{
	if (signal >= 1)
	{
		m_running = false;
	}
}

int main(int argc, char *argv[])
{
	// catch process signals
	signal(SIGINT, term);
	signal(SIGTERM, term);

#ifdef HAVE_LIBLUA5_1
	dtn::lua::LuaCore luac;
#endif

	// create a configuration
	Configuration &conf = Configuration::getInstance();

	string configurationfile = "config.ini";

	for (int i = 0; i < argc; i++)
	{
		string arg = argv[i];

		if (arg == "-c" && argc > i)
		{
			configurationfile = argv[i + 1];
		}
	}

	cout << "Configuration: " << configurationfile << endl;

	// Konfiguration laden
	ConfigFile _conf(configurationfile);
	conf.setConfigFile(_conf);

	// Benutzer ID setzen
	if ( _conf.keyExists("user") )
	{
		cout << "Switching UID to " << _conf.read<unsigned int>( "user", 0 ) << endl;
		setuid( _conf.read<unsigned int>( "user", 0 ) );
	}

	// Group ID setzen
	if ( _conf.keyExists("group") )
	{
		cout << "Switching GID to " << _conf.read<unsigned int>( "group", 0 ) << endl;
		setgid( _conf.read<unsigned int>( "group", 0 ) );
	}

	// DefaultDaemon initialisieren
	DefaultDaemon daemon(conf);

#ifdef USE_EMMA_CODE
	// GPSConnector hinzufügen
	GPSProvider *gpsconnection = NULL;
	GPSConnector *gpsd = NULL;

	if ( conf.useGPSDaemon() )
	{
		gpsd = new GPSConnector( conf.getGPSHost(), conf.getGPSPort() );
		daemon.addService( (Service*) gpsd );
		gpsconnection = gpsd;
	}
	else
	{
		// Erstelle einen GPS Dummy mit den statischen Werten aus der Config
		pair<double,double> position = conf.getStaticPosition();
		gpsconnection = new GPSDummy( position.first, position.second );
	}
#endif

	// Erstelle einen Router für die Bundles
	BundleRouter *router = new StaticBundleRouter( conf.getStaticRoutes(), conf.getLocalUri() );
	//router->setGPSProvider(gpsconnection);
	daemon.addService(router);

	// Erstelle eine Liste von statischen Knoten
	vector<Node> static_nodes = conf.getStaticNodes();

	// Schlüssel der Netzwerkverbindungen abrufen
	vector<string> netlist = conf.getNetList();
	{
		vector<string>::iterator iter = netlist.begin();

		while (iter != netlist.end())
		{
			string key = (*iter);
			string type = conf.getNetType(key);
			ConvergenceLayer *netcl = NULL;

#ifdef USE_EMMA_CODE
			if (type == "emma")
			{
				cout << "Adding ConvergenceLayer for EMMA (" << conf.getNetInterface(key) << ":" << conf.getNetPort(key) << ")" << endl;

				// ConvergenceLayer für den Einsatz zwischen Fahrzeugen
				EmmaConvergenceLayer *emma = new EmmaConvergenceLayer( conf.getLocalUri(), conf.getNetInterface(key), conf.getNetPort(key), conf.getNetBroadcast(key) );
				netcl = emma;

				// Verbinde Router und EmmaConvergenceLayer damit neu erkannte Knoten dem
				// Router mitgeteilt werden können.
				emma->setRouter(router);

				// Mit GPSConnector verbinden um Positionsdaten in das DiscoveryBundle einzusetzen
				emma->setGPSProvider( gpsconnection );

				// Fügt den EMMA-CL dem Multiplexer hinzu
				daemon.addConvergenceLayer(emma);
			}
#endif

			if (type == "udp")
			{
				cout << "Adding ConvergenceLayer for UDP (" << conf.getNetInterface(key) << ":" << conf.getNetPort(key) << ")" << endl;
				UDPConvergenceLayer *udp = new UDPConvergenceLayer( conf.getNetInterface(key), conf.getNetPort(key) );
				netcl = udp;

				daemon.addConvergenceLayer(udp);
			}
			else if (type == "tcp")
			{
				cout << "Adding ConvergenceLayer for TCP (" << conf.getNetInterface(key) << ":" << conf.getNetPort(key) << ")" << endl;
				TCPConvergenceLayer *tcp = new TCPConvergenceLayer( conf.getNetInterface(key), conf.getNetPort(key) );
				netcl = tcp;

				daemon.addConvergenceLayer(tcp);
			}

			// Suche in den statischen Knoten nach passenden DummyConvergenceLayern um diese zu ersetzen
			if (netcl != NULL)
			{
				// Statische Knoten laden, DummyConvergenceLayer austauschen
				// und am Router registrieren
				vector<Node>::iterator iter = static_nodes.begin();

				while (iter != static_nodes.end())
				{
					Node n = (*iter);
					DummyConvergenceLayer *dummy = dynamic_cast<DummyConvergenceLayer*>(n.getConvergenceLayer());

					if (dummy != NULL)
					{
						if (dummy->getName() == key)
						{
							n.setConvergenceLayer(netcl);
							delete dummy;
						}
					}

					(*iter) = n;

					iter++;
				}
			}

			iter++;
		}
	}

#ifdef WITH_SQLITE
	if ( conf.useSQLiteStorage() )
	{
		SQLiteBundleStorage *sqlite = new SQLiteBundleStorage(
					conf.getSQLiteDatabase(),
					conf.doSQLiteFlush()
					);

		daemon.addService(sqlite);
		daemon.setStorage(sqlite);
	}
#endif

	// Router zuweisen
	daemon.setRouter(router);

	// Lokale kopie vom BundleProtocol-Objekt
	BundleCore *core = daemon.getCore();


#ifdef USE_EMMA_CODE
	// Dienst für Messungen hinzufügen
	if ( _conf.read<int>( "measurement_jobs", 0 ) > 0 )
	{
		MeasurementWorkerConfig config;

		config.interval = _conf.read<int>( "measurement_interval", 5 );
		config.destination = _conf.read<string>( "measurement_destination", "dtn:none" );
		config.lifetime = _conf.read<int>( "measurement_lifetime", 20 );
		config.custody = _conf.read<int>( "measurement_custody", 0 ) == 1;

		int jobs = _conf.read<int>( "measurement_jobs", 0 );

		for (int i = 0; i < jobs; i++)
		{
			stringstream key1;
			stringstream key2;

			key1 << "measurement_job" << i+1 << "_type";
			key2 << "measurement_job" << i+1 << "_cmd";

			int type = _conf.read<int>( key1.str(), 0 );
			string cmd = _conf.read<string>( key2.str(), "" );

			MeasurementJob *job = new MeasurementJob( (unsigned char)type, cmd );
			config.jobs.push_back( job );
		}

		MeasurementWorker *mworker = new MeasurementWorker( core, config );
		daemon.addService( (Service*)mworker );
		mworker->setGPSProvider( gpsconnection );
	}
#endif

#ifdef DO_DEBUG_OUTPUT
	// Debugger
	Debugger debugger( core );
#endif

	// Echo-Modul
	EchoWorker echo( core );

	// System initialisiert
	cout << "dtn node ready" << endl;

	// Daemon initialisieren
	daemon.initialize();

	// Eigener Gültigkeitsbereicht für den Iterator
	{
		// Statische Knoten an den Router übergeben
		vector<Node>::iterator iter = static_nodes.begin();

		while (iter != static_nodes.end())
		{
			router->discovered(*iter);
			iter++;
		}
	}

#ifdef HAVE_LIBLUA5_1
	luac.run("demo.lua");
#endif

	while (m_running)
	{
		usleep(10000);
	}

	cout << "shutdown dtn node" << endl;

	// Daemon und damit alle Dienste stoppen
	daemon.abort();

#ifdef USE_EMMA_CODE
	// GPS Wegräumen wenn es kein Service ist
	Service *tmpservice = dynamic_cast<Service*>(gpsconnection);
	if (tmpservice == NULL)
	{
		delete gpsconnection;
	}
#endif

	delete router;

	return 0;
};

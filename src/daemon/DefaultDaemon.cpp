/*
 * DefaultDaemon.cpp
 *
 *  Created on: 07.11.2008
 *      Author: morgenro
 */

#include "daemon/DefaultDaemon.h"
#include "utils/Utils.h"


namespace dtn
{
namespace daemon
{
	DefaultDaemon::DefaultDaemon(Configuration &config)
	: m_config(config), m_bundleprotocol(NULL), m_storage(NULL), m_router(NULL), m_custodymanager(NULL)
	{
		m_bundleprotocol = new BundleCore(config.getLocalUri());
		addService(m_bundleprotocol);
	}

	DefaultDaemon::~DefaultDaemon()
	{
		MutexLock l(m_runlock);
		if (m_bundleprotocol != NULL) delete m_bundleprotocol;
		if (m_storage != NULL) delete m_storage;
	}

	void DefaultDaemon::setStorage(BundleStorage *s)
	{
		m_storage = s;
	}

	BundleStorage* DefaultDaemon::getStorage()
	{
		return m_storage;
	}

	void DefaultDaemon::setRouter(BundleRouter *r)
	{
		m_router = r;
	}

	BundleRouter* DefaultDaemon::getRouter()
	{
		return m_router;
	}

	void DefaultDaemon::setCustodyManager(CustodyManager *cm)
	{
		m_custodymanager = cm;
	}

	CustodyManager* DefaultDaemon::getCustodyManager()
	{
		return m_custodymanager;
	}

	void DefaultDaemon::addService(Service *s)
	{
		m_services.push_back(s);
	}

	void DefaultDaemon::addConvergenceLayer(ConvergenceLayer *c)
	{
		m_mcl.add(c);
	}

	BundleCore* DefaultDaemon::getCore()
	{
		return m_bundleprotocol;
	}

	void DefaultDaemon::initialize()
	{
		SimpleBundleStorage *simple = NULL;

		if (m_router == NULL)
		{
			m_router = new BundleRouter( m_config.getLocalUri() );
			addService(m_router);
		}

		if (m_storage == NULL)
		{
			// Storage für die Bundles erstellen
			simple = new SimpleBundleStorage(
				m_config.getStorageMaxSize() * 1024,
				4096,
				m_config.doStorageMerge()
				);

			m_storage = simple;
			addService(simple);
		}

		if (m_custodymanager == NULL)
		{
			if (simple == NULL)
			{
				simple = new SimpleBundleStorage();
				addService(simple);
			}

			m_custodymanager = simple;
		}

		m_bundleprotocol->setBundleRouter(m_router);
		m_bundleprotocol->setConvergenceLayer(&m_mcl);
		m_bundleprotocol->setCustodyManager(m_custodymanager);
		m_bundleprotocol->setStorage(m_storage);
		m_custodymanager->setCallbackClass(m_bundleprotocol);

		cout << "starting serivces... ";

		// Dienste starten
		{
			vector<Service*>::iterator iter = m_services.begin();

			while (iter != m_services.end())
			{
				Service *s = (*iter);
				cout << "[" << s->getName() << "] "; cout.flush();
				s->start();
				iter++;
			}
		}

		// Arbeiter-Dienste starten
		{
			vector<Service*>::iterator iter = m_worker.begin();

			while (iter != m_worker.end())
			{
				Service *s = (*iter);
				cout << "[" << s->getName() << "] "; cout.flush();
				s->start();
				iter++;
			}
		}

		cout << "done!" << endl;

		// ConvergenceLayer starten
		m_mcl.initialize();
	}

	void DefaultDaemon::abort()
	{
		// ConvergenceLayer stoppen
		m_mcl.terminate();

		cout << "stopping serivces... ";

		// Alle Arbeiter-Dienste stoppen
		{
			vector<Service*>::iterator iter = m_worker.begin();

			while (iter != m_worker.end())
			{
				Service *s = (*iter);
				cout << s->getName() << " "; cout.flush();
				s->abort();
				iter++;
			}
		}

		// Alle Dienste stoppen
		{
			vector<Service*>::iterator iter = m_services.begin();

			while (iter != m_services.end())
			{
				Service *s = (*iter);
				cout << "[" << s->getName() << "] "; cout.flush();
				s->abort();
				iter++;
			}
		}

		cout << "done!" << endl;
	}
}
}

/*
 * DefaultDaemon.h
 *
 *  Created on: 07.11.2008
 *      Author: morgenro
 */

#include "core/BundleCore.h"
#include "core/BundleStorage.h"
#include "core/StaticBundleRouter.h"
#include "core/SimpleBundleStorage.h"
#include "core/ConvergenceLayer.h"
#include "core/MultiplexConvergenceLayer.h"
#include "core/Node.h"

#include "utils/Service.h"
#include "daemon/Configuration.h"
#include <vector>
#include <iostream>
#include <csignal>

#include <unistd.h>
#include <sys/types.h>
#include "utils/MutexLock.h"
#include "utils/Mutex.h"

using namespace dtn::core;

#ifndef DEFAULTDAEMON_H_
#define DEFAULTDAEMON_H_

namespace dtn
{
	namespace daemon
	{
		class DefaultDaemon
		{
		public:
			DefaultDaemon(Configuration &config);
			~DefaultDaemon();

			void setStorage(BundleStorage *s);
			BundleStorage* getStorage();

			void setRouter(BundleRouter *r);
			BundleRouter* getRouter();

			void setCustodyManager(CustodyManager *cm);
			CustodyManager* getCustodyManager();

			BundleCore* getCore();

			void addService(Service *s);
			void addConvergenceLayer(ConvergenceLayer *c);

			void initialize();
			void abort();

			bool m_running;
			Mutex m_runlock;

		private:
			Configuration &m_config;
			BundleCore *m_bundleprotocol;
			BundleStorage *m_storage;
			BundleRouter *m_router;
			CustodyManager *m_custodymanager;
			MultiplexConvergenceLayer m_mcl;

			// Dieser Vector enthält alle Service Instanzen
			vector<Service*> m_services;
			vector<Service*> m_worker;
		};
	}
}


#endif /* DEFAULTDAEMON_H_ */

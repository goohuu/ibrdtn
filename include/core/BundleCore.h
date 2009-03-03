#ifndef BUNDLECORE_H_
#define BUNDLECORE_H_

#include "core/BundleStorage.h"
#include "core/ConvergenceLayer.h"
#include "data/BundleFactory.h"
#include "core/BundleRouter.h"
#include "core/CustodyTimer.h"
#include "core/CustodyManager.h"
#include "core/CustodyManagerCallback.h"
#include "core/BundleReceiver.h"
#include "data/CustodySignalBlock.h"
#include "data/StatusReportBlock.h"
#include <vector>
#include <list>
#include "utils/MutexLock.h"
#include "utils/Mutex.h"
#include "utils/Service.h"


using namespace dtn::data;
using namespace dtn::utils;

namespace dtn
{
	namespace core
	{
		class AbstractWorker;

		/**
		 * Das BundleCore übernimmt die Funktionen die zum korrekten Ablauf des
		 * DTN Protocol nötig sind. Die Klasse benötigt ein BundleStorage und einen ConvergenceLayer
		 * welche vor dem ersten Aufruf von tick() zugewiesen werden müssen.
		 */
		class BundleCore : public Service, public CustodyManagerCallback, public BundleReceiver
		{
		public:
			/**
			 * Standardkonstruktor
			 */
			BundleCore(string localeid);

			/**
			 * Destruktor
			 */
			virtual ~BundleCore();

			/**
			 * Setzt eine BundleStorage
			 * @param bs Eine Instanz einer BundleStorage
			 */
			void setStorage(BundleStorage *bs);

			/**
			 * Gibt die zugewiesene BundleStorage zurück
			 * @return Eine Instanz einer BundleStorage oder NULL, wenn keine zugewiesen wurde
			 */
			BundleStorage* getStorage();

			/**
			 * Setzt ein ConvergenceLayer
			 * @param cl Eine Instanz eines ConvergenceLayer
			 */
			void setConvergenceLayer(ConvergenceLayer *cl);

			/**
			 * Gibt den zugewiesenen timerConvergenceLayer zurück
			 * @return Eine Instanz eines ConvergenceLayer oder NULL, wenn keine zugewiesen wurde
			 */
			ConvergenceLayer* getConvergenceLayer();

			/**
			 * Setzt einen BundleRouter
			 * @param router Eine Instanz eines BundleRouters
			 */
			void setBundleRouter(BundleRouter *router);

			/**
			 * Gibt den zugewiesenen BundleRouter zurück
			 * @return Eine Instanz eines BundleRouters oder NULL, wenn keine zugewiesen wurde
			 */
			BundleRouter* getBundleRouter();

			/**
			 * Führt einen tick() aus. Dieses Model ermöglicht einen getakteten Ablauf ohne
			 * Threads. Dabei wird von allen Service Klassen regelmäßig die Funktion tick()
			 * aufgerufen.
			 * @sa Service::tick()
			 */
			virtual void tick();

			/**
			 * Sendet ein Bundle
			 */
			TransmitReport transmit(Bundle *b);

			void registerSubNode(string eid, AbstractWorker *node);
			void unregisterSubNode(string eid);

			void triggerCustodyTimeout(CustodyTimer timer);

			void setCustodyManager(CustodyManager *custodymanager);

			CustodyManager* getCustodyManager();

			virtual void received(ConvergenceLayer *cl, Bundle *b);

			string getLocalURI() const;


		private:
			/**
			 * Funktion die zu Debugzwecken ein Bundle nach COUT ausgibt.
			 * @param b Das Bundle welches angezeigt werden soll
			 */
			void debugBundle(Bundle *b);

			void processStatusReport(Bundle *b);
			void processCustody(Bundle *b);

			bool transmitBundle(BundleSchedule schedule);

			void forwardLocalBundle(Bundle *bundle);
			//void checkNeighbours();

			void dismissBundle(Bundle *b, string reason = "unknown");

			void transmitCustody(bool accept, Bundle *b);

			Bundle* createStatusReport(Bundle *b, StatusReportType type);
			Bundle* createCustodySignal(Bundle *b, bool accepted);

			ConvergenceLayer *m_clayer;

			BundleStorage *m_storage;
			BundleRouter *m_router;

			map<string, AbstractWorker*> m_worker;

			Mutex m_workerlock;

			CustodyManager *m_custodymanager;

			bool m_bundlewaiting;

			string m_localeid;
		};
	}
}

#endif /*BUNDLECORE_H_*/


#ifndef BUNDLECORE_H_
#define BUNDLECORE_H_

#include "core/BundleStorage.h"
#include "core/ConvergenceLayer.h"
#include "data/BundleFactory.h"
#include "core/BundleRouter.h"
#include "core/CustodyTimer.h"
#include "core/BundleReceiver.h"
#include "core/EventReceiver.h"
#include "core/CustodyManager.h"
#include "core/Event.h"

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
		class BundleCore : public Service, public BundleReceiver, public EventReceiver
		{
		public:
			static BundleCore& getInstance(string eid);
			static BundleCore& getInstance();

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
			 * get the current custody manager
			 */
			CustodyManager& getCustodyManager();

			void registerSubNode(string eid, AbstractWorker *node);
			void unregisterSubNode(string eid);

			virtual void received(const ConvergenceLayer &cl, Bundle &b);

			string getLocalURI() const;

			/**
			 * method to receive new events from the EventSwitch
			 */
			void raiseEvent(const Event *evt);

//			/**
//			 * transmit a bundle to a specific node
//			 * usually this is called by the storage
//			 */
//			bool transmitBundle(const BundleSchedule &schedule, const Node &node);

			/**
			 * transmit a bundle directly to a reachable node
			 */
			void transmit(const Node &n, const Bundle &b);

			/**
			 * deliver a bundle to a application
			 */
			void deliver(const Bundle &b);

		protected:
			void tick();

		private:
			/**
			 * Standardkonstruktor
			 */
			BundleCore();

			/**
			 * Destruktor
			 */
			virtual ~BundleCore();

			void transmit(Bundle &b);

			/**
			 * Forbidden copy constructor
			 */
			BundleCore operator=(const BundleCore &k) {};

			void setLocalEID(string eid);

			void transmitCustody(bool accept, const Bundle &b);

			Bundle createStatusReport(const Bundle &b, StatusReportType type, StatusReportReasonCode reason = NO_ADDITIONAL_INFORMATION);
			Bundle createCustodySignal(const Bundle &b, bool accepted);

			ConvergenceLayer *m_clayer;
			map<string, AbstractWorker*> m_worker;
			string m_localeid;
			CustodyManager m_cm;
			unsigned int m_dtntime;
		};
	}
}

#endif /*BUNDLECORE_H_*/


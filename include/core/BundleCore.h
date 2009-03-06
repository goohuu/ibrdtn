#ifndef BUNDLECORE_H_
#define BUNDLECORE_H_

#include "core/BundleStorage.h"
#include "core/ConvergenceLayer.h"
#include "data/BundleFactory.h"
#include "core/BundleRouter.h"
#include "core/CustodyTimer.h"
#include "core/BundleReceiver.h"
#include "core/EventReceiver.h"
#include "core/Event.h"

#include "data/CustodySignalBlock.h"
#include "data/StatusReportBlock.h"
#include <vector>
#include <list>
#include "utils/MutexLock.h"
#include "utils/Mutex.h"


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
		class BundleCore : public BundleReceiver, public EventReceiver
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
			 * Sendet ein Bundle
			 */
			TransmitReport transmit(Bundle *b);

			void registerSubNode(string eid, AbstractWorker *node);
			void unregisterSubNode(string eid);

			virtual void received(ConvergenceLayer *cl, Bundle *b);

			string getLocalURI() const;

			/**
			 * method to receive new events from the EventSwitch
			 */
			void raiseEvent(const Event *evt);

		private:
			void processCustody(const Bundle &b);
			bool transmitBundle(BundleSchedule schedule);
			void forwardLocalBundle(Bundle *bundle);
			void transmitCustody(bool accept, const Bundle &b);

			Bundle* createStatusReport(const Bundle &b, StatusReportType type, StatusReportReasonCode reason = NO_ADDITIONAL_INFORMATION);
			Bundle* createCustodySignal(const Bundle &b, bool accepted);

			ConvergenceLayer *m_clayer;

			map<string, AbstractWorker*> m_worker;

			Mutex m_workerlock;

			bool m_bundlewaiting;

			string m_localeid;
		};
	}
}

#endif /*BUNDLECORE_H_*/


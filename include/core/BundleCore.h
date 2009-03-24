#ifndef BUNDLECORE_H_
#define BUNDLECORE_H_

#include "core/AbstractBundleStorage.h"
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
		 * The BundleCore manage the Bundle Protocol basics
		 */
		class BundleCore : public Service, public BundleReceiver, public EventReceiver
		{
		public:
			static BundleCore& getInstance(string eid);
			static BundleCore& getInstance();

			/**
			 * Set a ConvergenceLayer Object which is used to send and receive bundles.
			 * If you want to use more than only one ConvergenceLayer the MultiplexCongerenceLayer
			 * is needed.
			 * @param cl A instance of a ConvergenceLayer class.
			 */
			void setConvergenceLayer(ConvergenceLayer *cl);

			/**
			 * Get the currently used ConvergenceLayer instance.
			 * @return The currently used ConvergenceLayer or NULL if nothing is assigned.
			 */
			ConvergenceLayer* getConvergenceLayer();

			/**
			 * get the current custody manager
			 */
			CustodyManager& getCustodyManager();

			void registerSubNode(string eid, AbstractWorker *node);
			void unregisterSubNode(string eid);

			virtual void received(const ConvergenceLayer &cl, const Bundle &b);

			string getLocalURI() const;

			/**
			 * method to receive new events from the EventSwitch
			 */
			void raiseEvent(const Event *evt);

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
			 * default constructor
			 */
			BundleCore();

			/**
			 * destructor
			 */
			virtual ~BundleCore();

			void transmit(const Bundle &b);

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


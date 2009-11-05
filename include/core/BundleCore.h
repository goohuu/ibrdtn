#ifndef BUNDLECORE_H_
#define BUNDLECORE_H_

#include "ibrdtn/default.h"

#include "core/BundleStorage.h"
#include "core/BundleRouter.h"
#include "core/CustodyTimer.h"
#include "core/EventReceiver.h"
#include "core/CustodyManager.h"
#include "core/Event.h"
#include "core/Clock.h"

#include "net/ConnectionManager.h"

#include "ibrdtn/data/EID.h"
#include "ibrdtn/data/CustodySignalBlock.h"
#include "ibrdtn/data/StatusReportBlock.h"

#include <vector>
#include <list>
#include <map>

using namespace dtn::data;

namespace dtn
{
	namespace core
	{
		class AbstractWorker;

		/**
		 * The BundleCore manage the Bundle Protocol basics
		 */
		class BundleCore : public EventReceiver, public dtn::net::ConnectionManager
		{
		public:
			static dtn::data::EID local;

			static BundleCore& getInstance();

			/**
			 * Get a specific subnode.
			 * @return
			 */
			AbstractWorker* getSubNode(EID eid);

			/**
			 * Register a worker. This method is used by the API.
			 */
			void registerSubNode(EID eid, AbstractWorker *node);

			/**
			 * Unregister a worker. This method is used by the API.
			 */
			void unregisterSubNode(EID eid);

			/**
			 * method to receive new events from the EventSwitch
			 */
			void raiseEvent(const Event *evt);

			/**
			 * transmit a bundle directly to a reachable node
			 */
			void transmit(const dtn::data::EID &eid, const Bundle &b);

			/**
			 * deliver a bundle to a application
			 */
			void deliver(const Bundle &b);

			Clock& getClock();

		protected:
			/**
			 * get the current custody manager
			 */
			CustodyManager& getCustodyManager();

		private:
			/**
			 * default constructor
			 */
			BundleCore();

			/**
			 * destructor
			 */
			virtual ~BundleCore();

			/**
			 * Shortcut to give a bundle to the router module.
			 * @param b The bundle to transfer.
			 */
			void transmit(const Bundle &b);

			/**
			 * Forbidden copy constructor
			 */
			BundleCore operator=(const BundleCore &k) {};

			void transmitCustody(bool accept, const Bundle &b);

			/**
			 * This method generates a status report.
			 * @param b The attributes of the given bundle where used to generate the status report.
			 * @param type Defines the type of the report.
			 * @param reason Give a additional reason.
			 * @return A bundle with a status report block.
			 */
			Bundle createStatusReport(const Bundle &b, StatusReportBlock::TYPE type, StatusReportBlock::REASON_CODE reason = StatusReportBlock::NO_ADDITIONAL_INFORMATION);

			/**
			 * This method generates a custody signal.
			 * @param b The attributes of the given bundle where used to generate the custody signal.
			 * @param accepted Define if a accept or reject signal is generated. True is for accept.
			 * @return A bundle with a custody signal block.
			 */
			Bundle createCustodySignal(const Bundle &b, bool accepted);

			/**
			 * This map contains all active worker. This could be worker built-in this daemon or
			 * registrations made by the API.
			 */
			std::map<EID, AbstractWorker*> m_worker;

			/**
			 * A custody manager takes care about a transmission of custody to another node.
			 */
			CustodyManager m_cm;

			dtn::utils::Conditional _register_action;

			/**
			 * This is a clock object. It can be used to synchronize methods to the local clock.
			 */
			Clock _clock;

			bool _shutdown;
		};
	}
}

#endif /*BUNDLECORE_H_*/


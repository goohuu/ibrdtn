#ifndef BUNDLECORE_H_
#define BUNDLECORE_H_

#include "ibrdtn/default.h"

#include "core/BundleStorage.h"
#include "core/CustodyTimer.h"
#include "core/EventReceiver.h"
#include "core/CustodyManager.h"
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
//		class AbstractWorker;

		/**
		 * The BundleCore manage the Bundle Protocol basics
		 */
		class BundleCore : public EventReceiver, public dtn::net::ConnectionManager
		{
		public:
			static dtn::data::EID local;

			static BundleCore& getInstance();

			/**
			 * method to receive new events from the EventSwitch
			 */
			void raiseEvent(const Event *evt);

			Clock& getClock();

			void setStorage(dtn::core::BundleStorage *storage);
			dtn::core::BundleStorage& getStorage();

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
			 * A custody manager takes care about a transmission of custody to another node.
			 */
			CustodyManager m_cm;

			/**
			 * This is a clock object. It can be used to synchronize methods to the local clock.
			 */
			Clock _clock;

			bool _shutdown;

			dtn::core::BundleStorage *_storage;
		};
	}
}

#endif /*BUNDLECORE_H_*/


#ifndef BUNDLECORE_H_
#define BUNDLECORE_H_

#include "core/BundleStorage.h"
#include "core/CustodyTimer.h"
#include "core/CustodyManager.h"
#include "core/Clock.h"

#include "net/ConnectionManager.h"
#include "net/ConvergenceLayer.h"

#include "ibrdtn/data/EID.h"
#include "ibrdtn/data/CustodySignalBlock.h"

#include "core/StatusReportGenerator.h"

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
		class BundleCore
		{
		public:
			static dtn::data::EID local;

			static BundleCore& getInstance();

			Clock& getClock();

			void setStorage(dtn::core::BundleStorage *storage);
			dtn::core::BundleStorage& getStorage();

			/**
			 * get the current custody manager
			 */
			CustodyManager& getCustodyManager();

			void transferTo(const dtn::data::EID &destination, dtn::data::Bundle &bundle);

			void addConvergenceLayer(dtn::net::ConvergenceLayer *cl);

			void addConnection(const dtn::core::Node &n);

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

			/**
			 * A custody manager takes care about a transmission of custody to another node.
			 */
			CustodyManager _cm;

			/**
			 * This is a clock object. It can be used to synchronize methods to the local clock.
			 */
			Clock _clock;

			dtn::core::BundleStorage *_storage;

			// generator for statusreports
			StatusReportGenerator _statusreportgen;

			// manager class for connections
			dtn::net::ConnectionManager _connectionmanager;
		};
	}
}

#endif /*BUNDLECORE_H_*/


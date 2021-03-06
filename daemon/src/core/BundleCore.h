#ifndef BUNDLECORE_H_
#define BUNDLECORE_H_

#include "Component.h"

#include "core/EventReceiver.h"
#include "core/StatusReportGenerator.h"
#include "core/BundleStorage.h"
#include "core/WallClock.h"
#include "routing/BaseRouter.h"

#include "net/ConnectionManager.h"
#include "net/ConvergenceLayer.h"

#include <ibrdtn/data/Serializer.h>
#include <ibrdtn/data/EID.h>
#include <ibrdtn/data/CustodySignalBlock.h>

#include <vector>
#include <set>
#include <map>

using namespace dtn::data;

namespace dtn
{
	namespace core
	{
		/**
		 * The BundleCore manage the Bundle Protocol basics
		 */
		class BundleCore : public dtn::daemon::IntegratedComponent, public dtn::core::EventReceiver, public dtn::data::Validator
		{
		public:
			static dtn::data::EID local;

			static BundleCore& getInstance();

			WallClock& getClock();

			void setStorage(dtn::core::BundleStorage *storage);
			dtn::core::BundleStorage& getStorage();

			void setRouter(dtn::routing::BaseRouter *router);
			dtn::routing::BaseRouter& getRouter() const;

			void transferTo(const dtn::data::EID &destination, const dtn::data::BundleID &bundle);

			void addConvergenceLayer(dtn::net::ConvergenceLayer *cl);

			void addConnection(const dtn::core::Node &n);
			void removeConnection(const dtn::core::Node &n);

			const std::set<dtn::core::Node> getNeighbors();

			void raiseEvent(const dtn::core::Event *evt);

			virtual void validate(const dtn::data::PrimaryBlock &obj) const throw (RejectedException);
			virtual void validate(const dtn::data::Block &obj, const size_t length) const throw (RejectedException);
			virtual void validate(const dtn::data::Bundle &obj) const throw (RejectedException);

			/**
			 * Define a global block size limit. This is used in the validator to reject bundles while receiving.
			 */
			static size_t blocksizelimit;

			/**
			 * Define the maximum lifetime for accepted bundles
			 */
			static size_t max_lifetime;

			/**
			 * Define the maximum offset for the timestamp of pre-dated bundles
			 */
			static size_t max_timestamp_future;

			/**
			 * Define if forwarding is allowed. If set to false, this daemon only accepts bundles for local applications.
			 */
			static bool forwarding;

			/**
			 * @see Component::getName()
			 */
			virtual const std::string getName() const;

			static void processBlocks(dtn::data::Bundle &b);

		protected:
			virtual void componentUp();
			virtual void componentDown();

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
			BundleCore operator=(const BundleCore &k) { return k; };

			/**
			 * This is a clock object. It can be used to synchronize methods to the local clock.
			 */
			WallClock _clock;

			dtn::core::BundleStorage *_storage;
			dtn::routing::BaseRouter *_router;

			// generator for statusreports
			StatusReportGenerator _statusreportgen;

			// manager class for connections
			dtn::net::ConnectionManager _connectionmanager;
		};
	}
}

#endif /*BUNDLECORE_H_*/


#ifndef SIMPLEBUNDLESTORAGE_H_
#define SIMPLEBUNDLESTORAGE_H_

#include "ibrdtn/default.h"

#include "core/BundleCore.h"
#include "core/BundleStorage.h"
#include "core/Node.h"
#include "ibrcommon/thread/Conditional.h"
#include "ibrcommon/thread/AtomicCounter.h"
#include "core/EventReceiver.h"
#include "routing/BundleList.h"

#include "ibrcommon/data/File.h"
#include "ibrdtn/data/Bundle.h"

#include <set>

using namespace dtn::data;

namespace dtn
{
	namespace core
	{
		/**
		 * This storage holds all bundles and fragments in the system memory.
		 */
		class SimpleBundleStorage : public BundleStorage, public EventReceiver
		{
		public:
			/**
			 * Constructor
			 */
			SimpleBundleStorage();

			/**
			 * Constructor
			 */
			SimpleBundleStorage(const ibrcommon::File &workdir);

			/**
			 * Destructor
			 */
			virtual ~SimpleBundleStorage();

			/**
			 * Stores a bundle in the storage.
			 * @param bundle The bundle to store.
			 */
			virtual void store(const dtn::data::Bundle &bundle);

			/**
			 * This method returns a specific bundle which is identified by
			 * its id.
			 * @param id The ID of the bundle to return.
			 * @return A bundle object of the
			 */
			virtual dtn::data::Bundle get(const dtn::data::BundleID &id);

			virtual dtn::data::Bundle get(const dtn::data::EID &eid);
			virtual void unblock(const dtn::data::EID &eid);

			/**
			 * This method deletes a specific bundle in the storage.
			 * No reports will be generated here.
			 * @param id The ID of the bundle to remove.
			 */
			void remove(const dtn::data::BundleID &id);

			/**
			 * @sa BundleStorage::clear()
			 */
			void clear();

			/**
			 * @sa BundleStorage::empty()
			 */
			bool empty();

			/**
			 * @sa BundleStorage::count()
			 */
			unsigned int count();

			/**
			 * This method is used to receive events.
			 * @param evt
			 */
			void raiseEvent(const Event *evt);

		private:
			enum RunMode
			{
				MODE_NONPERSISTENT = 0,
				MODE_PERSISTENT = 1
			};

			class BundleStore : private dtn::routing::BundleList
			{
			public:
				BundleStore(SimpleBundleStorage &sbs);
				~BundleStore();

				void store(const dtn::data::Bundle &bundle);
				void remove(const dtn::data::BundleID &id);
				void clear();

				void expire(const size_t timestamp);

				std::set<dtn::data::Bundle> bundles;

			protected:
				virtual void eventBundleExpired(const ExpiringBundle &b);

			private:
				SimpleBundleStorage &_sbs;
			};

			BundleStore _store;

			virtual void shutdown();

			bool _running;
			RunMode _mode;
			ibrcommon::File _workdir;
			std::map<dtn::data::BundleID, ibrcommon::File> _bundlefiles;

			ibrcommon::Conditional _dbchanged;
			ibrcommon::AtomicCounter _blocker;
			dtn::data::EID _unblock_eid;
		};
	}
}

#endif /*SIMPLEBUNDLESTORAGE_H_*/

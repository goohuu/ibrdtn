#ifndef SIMPLEBUNDLESTORAGE_H_
#define SIMPLEBUNDLESTORAGE_H_

#include "ibrdtn/default.h"

#include "core/BundleCore.h"
#include "core/BundleStorage.h"
#include "core/Node.h"
#include "ibrdtn/utils/WaitForConditional.h"
#include "core/EventReceiver.h"

#include "ibrdtn/data/Bundle.h"

using namespace dtn::data;

namespace dtn
{
	namespace core
	{
		/**
		 * This storage holds all bundles and fragments in the system memory.
		 */
		class SimpleBundleStorage : public dtn::utils::JoinableThread, public BundleStorage, public EventReceiver
		{
		public:
			/**
			 * Constructor
			 */
			SimpleBundleStorage();

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

		protected:
			virtual void run();

		private:
			list<Bundle> _bundles;
			list<list<Bundle> > _fragments;

//			/**
//			 * Try to find outdated bundles and delete them.
//			 * Additional a deleted report is created if necessary.
//			 */
//			void deleteDeprecated();

			dtn::utils::WaitForConditional _timecond;
			bool _running;
		};
	}
}

#endif /*SIMPLEBUNDLESTORAGE_H_*/

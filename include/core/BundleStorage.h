#ifndef BUNDLESTORAGE_H_
#define BUNDLESTORAGE_H_

#include "core/BundleSchedule.h"
#include "data/Bundle.h"
#include <vector>

using namespace dtn::data;

namespace dtn
{
	namespace core
	{
		/**
		 * A BundleStorage can store bundles and fragments for later use.
		 */
		class BundleStorage
		{
			public:
			/**
			 * constructor
			 */
			BundleStorage() {};

			/**
			 * destructor
			 */
			virtual ~BundleStorage() {};

			/**
			 * store a schedule for later use
			 * @param schedule the schedule to store
			 */
			virtual void store(const BundleSchedule &schedule) = 0;

			/**
			 * Store a bundle fragment in the storage. If all fragments are available
			 * in the storage, they are merged and return by this method. If not, only
			 * NULL is returned.
			 */
			virtual Bundle* storeFragment(const Bundle &bundle) = 0;

			/**
			 * Clears all bundles, schedules and fragments in the storage.
			 */
			virtual void clear() = 0;

			/**
			 * @return True, if no bundles in the storage.
			 */
			virtual bool isEmpty() = 0;

			/**
			 * @return the count of bundles in the storage
			 */
			virtual unsigned int getCount() = 0;
		};
	}
}

#endif /*BUNDLESTORAGE_H_*/

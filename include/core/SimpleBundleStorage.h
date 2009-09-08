#ifndef SIMPLEBUNDLESTORAGE_H_
#define SIMPLEBUNDLESTORAGE_H_

#include "core/BundleCore.h"
#include "core/AbstractBundleStorage.h"
#include "core/Node.h"
#include "data/Bundle.h"
#include "data/DefragmentContainer.h"
#include "utils/Service.h"
#include "utils/MutexLock.h"
#include "utils/Mutex.h"
#include "utils/Conditional.h"
#include <list>
#include <map>

using namespace dtn::data;
using namespace dtn::utils;

namespace dtn
{
	namespace core
	{
		/**
		 * This storage holds all bundles, fragments and schedules in the system memory.
		 */
		class SimpleBundleStorage : public Service, public AbstractBundleStorage
		{
		public:
			/**
			 * Constructor
			 * @param[in] size Size of the memory storage in kBytes. (e.g. 1024*1024 = 1MB)
			 * @param[in] bundle_maxsize Maximum size of one bundle in bytes.
			 */
			SimpleBundleStorage(unsigned int size = 1024 * 1024, unsigned int bundle_maxsize = 1024);

			/**
			 * Destructor
			 */
			virtual ~SimpleBundleStorage();

			/**
			 * @sa AbstractBundleStorage::store(BundleSchedule &schedule)
			 */
			void store(const BundleSchedule &schedule);

			/**
			 * @sa storeFragment();
			 */
			Bundle* storeFragment(const Bundle &bundle);

			/**
			 * @sa getSchedule(unsigned int dtntime)
			 */
			BundleSchedule getSchedule(unsigned int dtntime);

			/**
			 * @sa getSchedule(string destination)
			 */
			BundleSchedule getSchedule(string destination);

			/**
			 * @sa BundleStorage::clear()
			 */
			void clear();

			/**
			 * @sa BundleStorage::isEmpty()
			 */
			bool isEmpty();

			/**
			 * @sa BundleStorage::getCount()
			 */
			unsigned int getCount();

		protected:
			/**
			 * @sa Service::tick()
			 */
			virtual void tick();

			void terminate();

			void eventNodeAvailable(const Node &node);
			void eventNodeUnavailable(const Node &node);

		private:
			list<BundleSchedule> m_schedules;
			list<DefragmentContainer* > _fragments;

			/**
			 * Try to find outdated bundles and delete them.
			 * Additional a deleted report is created if necessary.
			 */
			void deleteDeprecated();

			/**
			 * A variable to hold the next timeout of a bundle in the storage.
			 */
			unsigned int m_nextdeprecation;

			time_t m_last_compress;

			unsigned int m_size;
			unsigned int m_bundle_maxsize;

			unsigned int m_currentsize;

			bool m_nocleanup;

			Mutex m_readlock;
			Mutex m_fragmentslock;

			map<string,Node> m_neighbours;
		};
	}
}

#endif /*SIMPLEBUNDLESTORAGE_H_*/

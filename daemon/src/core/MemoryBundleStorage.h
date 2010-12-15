/*
 * MemoryBundleStorage.h
 *
 *  Created on: 22.11.2010
 *      Author: morgenro
 */

#ifndef MEMORYBUNDLESTORAGE_H_
#define MEMORYBUNDLESTORAGE_H_

#include "Component.h"
#include "core/BundleCore.h"
#include "core/BundleStorage.h"
#include "core/Node.h"
#include "core/EventReceiver.h"

#include <ibrdtn/data/Bundle.h>
#include <ibrdtn/data/BundleList.h>

namespace dtn
{
	namespace core
	{
		class MemoryBundleStorage : public BundleStorage, public EventReceiver, public dtn::daemon::IntegratedComponent, private dtn::data::BundleList
		{
		public:
			MemoryBundleStorage(size_t maxsize = 0);
			virtual ~MemoryBundleStorage();

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
			 * Query for a bundle with a specific destination. Set exact to true, if the application
			 * part should be compared too.
			 * @param eid
			 * @param exact
			 * @return
			 */
			virtual const dtn::data::MetaBundle getByDestination(const dtn::data::EID &eid, bool exact = false, bool singleton = true);

			/**
			 * Returns a bundle ID which is not in the bloomfilter, but in the storage
			 * @param filter
			 * @return
			 */
			virtual const dtn::data::MetaBundle getByFilter(const ibrcommon::BloomFilter &filter);

			/**
			 * This method deletes a specific bundle in the storage.
			 * No reports will be generated here.
			 * @param id The ID of the bundle to remove.
			 */
			void remove(const dtn::data::BundleID &id);

			/**
			 * Remove one bundles which match this filter
			 * @param filter
			 * @return The bundle meta data of the removed bundle.
			 */
			dtn::data::MetaBundle remove(const ibrcommon::BloomFilter &filter);

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
			 * Get the current size
			 */
			size_t size() const;

			/**
			 * @sa BundleStorage::releaseCustody();
			 */
			void releaseCustody(dtn::data::BundleID &bundle);

			/**
			 * This method is used to receive events.
			 * @param evt
			 */
			void raiseEvent(const Event *evt);

			/**
			 * @see Component::getName()
			 */
			virtual const std::string getName() const;

		protected:
			virtual void componentUp();
			virtual void componentDown();

			virtual void eventBundleExpired(const ExpiringBundle &b);

		private:
			ibrcommon::Mutex _bundleslock;
			std::set<dtn::data::Bundle> _bundles;

			size_t _maxsize;
			size_t _currentsize;
		};
	}
}

#endif /* MEMORYBUNDLESTORAGE_H_ */

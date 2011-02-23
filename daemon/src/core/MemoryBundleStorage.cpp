/*
 * MemoryBundleStorage.cpp
 *
 *  Created on: 22.11.2010
 *      Author: morgenro
 */

#include "core/MemoryBundleStorage.h"
#include "core/TimeEvent.h"
#include "core/GlobalEvent.h"
#include "core/BundleExpiredEvent.h"
#include "core/BundleEvent.h"

#include <ibrcommon/Logger.h>
#include <ibrcommon/thread/MutexLock.h>

namespace dtn
{
	namespace core
	{
		MemoryBundleStorage::MemoryBundleStorage(size_t maxsize)
		 : _maxsize(maxsize), _currentsize(0)
		{
		}

		MemoryBundleStorage::~MemoryBundleStorage()
		{
		}

		void MemoryBundleStorage::componentUp()
		{
			bindEvent(TimeEvent::className);
		}

		void MemoryBundleStorage::componentDown()
		{
			unbindEvent(TimeEvent::className);
		}

		void MemoryBundleStorage::raiseEvent(const Event *evt)
		{
			try {
				const TimeEvent &time = dynamic_cast<const TimeEvent&>(*evt);
				if (time.getAction() == dtn::core::TIME_SECOND_TICK)
				{
					// do expiration of bundles
					ibrcommon::MutexLock l(_bundleslock);
					dtn::data::BundleList::expire(time.getTimestamp());
				}
			} catch (const std::bad_cast&) { }
		}

		const std::string MemoryBundleStorage::getName() const
		{
			return "MemoryBundleStorage";
		}

		bool MemoryBundleStorage::empty()
		{
			ibrcommon::MutexLock l(_bundleslock);
			return _bundles.empty();
		}

		void MemoryBundleStorage::releaseCustody(dtn::data::BundleID&)
		{
			// custody is successful transferred to another node.
			// it is safe to delete this bundle now. (depending on the routing algorithm.)
		}

		unsigned int MemoryBundleStorage::count()
		{
			ibrcommon::MutexLock l(_bundleslock);
			return _bundles.size();
		}

		const std::list<dtn::data::MetaBundle> MemoryBundleStorage::get(BundleFilterCallback &cb)
		{
			// result list
			std::list<dtn::data::MetaBundle> result;

			// we have to iterate through all bundles
			ibrcommon::MutexLock l(_bundleslock);

			for (std::set<dtn::data::MetaBundle>::const_iterator iter = _priority_index.begin(); (iter != _priority_index.end()) && (result.size() < cb.limit()); iter++)
			{
				const dtn::data::MetaBundle &bundle = (*iter);

				if ( cb.shouldAdd(bundle) )
				{
					result.push_back(bundle);
				}
			}

			return result;
		}

		dtn::data::Bundle MemoryBundleStorage::get(const dtn::data::BundleID &id)
		{
			try {
				ibrcommon::MutexLock l(_bundleslock);

				for (std::set<dtn::data::Bundle>::const_iterator iter = _bundles.begin(); iter != _bundles.end(); iter++)
				{
					const dtn::data::Bundle &bundle = (*iter);
					if (id == bundle)
					{
						return bundle;
					}
				}
			} catch (const dtn::SerializationFailedException &ex) {
				// bundle loading failed
				IBRCOMMON_LOGGER(error) << "Error while loading bundle data: " << ex.what() << IBRCOMMON_LOGGER_ENDL;

				// the bundle is broken, delete it
				remove(id);

				throw BundleStorage::BundleLoadException();
			}

			throw BundleStorage::NoBundleFoundException();
		}

		void MemoryBundleStorage::store(const dtn::data::Bundle &bundle)
		{
			ibrcommon::MutexLock l(_bundleslock);

			// get size of the bundle
			dtn::data::DefaultSerializer s(std::cout);
			size_t size = s.getLength(bundle);

			// check if this container is too big for us.
			if ((_maxsize > 0) && (_currentsize + size > _maxsize))
			{
				throw StorageSizeExeededException();
			}

			// increment the storage size
			_currentsize += size;

			// insert Container
			pair<set<dtn::data::Bundle>::iterator,bool> ret = _bundles.insert( bundle );

			if (ret.second)
			{
				dtn::data::BundleList::add(dtn::data::MetaBundle(bundle));
				_priority_index.insert( bundle );
			}
			else
			{
				IBRCOMMON_LOGGER_DEBUG(5) << "Storage: got bundle duplicate " << bundle.toString() << IBRCOMMON_LOGGER_ENDL;
			}
		}

		void MemoryBundleStorage::remove(const dtn::data::BundleID &id)
		{
			ibrcommon::MutexLock l(_bundleslock);

			for (std::set<dtn::data::Bundle>::const_iterator iter = _bundles.begin(); iter != _bundles.end(); iter++)
			{
				if ( id == (*iter) )
				{
					// remove item in the bundlelist
					dtn::data::Bundle bundle = (*iter);

					// remove it from the bundle list
					_priority_index.erase(bundle);
					dtn::data::BundleList::remove(bundle);

					// get size of the bundle
					dtn::data::DefaultSerializer s(std::cout);
					size_t size = s.getLength(bundle);

					// decrement the storage size
					_currentsize -= size;

					// remove the container
					_bundles.erase(iter);

					return;
				}
			}

			throw BundleStorage::NoBundleFoundException();
		}

		dtn::data::MetaBundle MemoryBundleStorage::remove(const ibrcommon::BloomFilter &filter)
		{
			ibrcommon::MutexLock l(_bundleslock);

			for (std::set<dtn::data::Bundle>::const_iterator iter = _bundles.begin(); iter != _bundles.end(); iter++)
			{
				const dtn::data::Bundle &bundle = (*iter);

				if ( filter.contains(bundle.toString()) )
				{
					// remove it from the bundle list
					_priority_index.erase(bundle);
					dtn::data::BundleList::remove(bundle);

					// get size of the bundle
					dtn::data::DefaultSerializer s(std::cout);

					// decrement the storage size
					_currentsize -= s.getLength(bundle);

					// remove the container
					_bundles.erase(iter);

					return (MetaBundle)bundle;
				}
			}

			throw BundleStorage::NoBundleFoundException();
		}

		size_t MemoryBundleStorage::size() const
		{
			return _currentsize;
		}

		void MemoryBundleStorage::clear()
		{
			ibrcommon::MutexLock l(_bundleslock);

			_bundles.clear();
			_priority_index.clear();
			dtn::data::BundleList::clear();

			// set the storage size to zero
			_currentsize = 0;
		}

		void MemoryBundleStorage::eventBundleExpired(const ExpiringBundle &b)
		{
			for (std::set<dtn::data::Bundle>::const_iterator iter = _bundles.begin(); iter != _bundles.end(); iter++)
			{
				if ( b.bundle == (*iter) )
				{
					dtn::data::Bundle bundle = (*iter);

					// get size of the bundle
					dtn::data::DefaultSerializer s(std::cout);

					// decrement the storage size
					_currentsize -= s.getLength(bundle);

					_priority_index.erase(bundle);
					_bundles.erase(iter);
					break;
				}
			}

			// raise bundle event
			dtn::core::BundleEvent::raise( b.bundle, dtn::core::BUNDLE_DELETED, dtn::data::StatusReportBlock::LIFETIME_EXPIRED);

			// raise an event
			dtn::core::BundleExpiredEvent::raise( b.bundle );
		}
	}
}

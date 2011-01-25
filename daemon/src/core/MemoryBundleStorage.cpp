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

		const dtn::data::MetaBundle MemoryBundleStorage::getByFilter(const ibrcommon::BloomFilter &filter)
		{
			// search for one bundle which is not contained in the filter
			// until we have a better strategy, we have to iterate through all bundles
			ibrcommon::MutexLock l(_bundleslock);
			for (std::set<dtn::data::Bundle>::const_iterator iter = _bundles.begin(); iter != _bundles.end(); iter++)
			{
				const dtn::data::Bundle &bundle = (*iter);

				try {
					if ( !filter.contains(bundle.toString()) )
					{
						// check if the destination is blocked by the destination filter
						if ( (_destination_filter.find(bundle._destination) == _destination_filter.end()) )
						{
							IBRCOMMON_LOGGER_DEBUG(10) << bundle.toString() << " is not in the bloomfilter" << IBRCOMMON_LOGGER_ENDL;
							return bundle;
						}
					}
				} catch (const dtn::InvalidDataException &ex) {
					IBRCOMMON_LOGGER_DEBUG(10) << "InvalidDataException on bundle get: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
				}
			}

			throw BundleStorage::NoBundleFoundException();
		}

		const dtn::data::MetaBundle MemoryBundleStorage::getByDestination(const dtn::data::EID &eid, bool exact, bool singleton)
		{
			IBRCOMMON_LOGGER_DEBUG(5) << "Storage: get bundle for " << eid.getString() << IBRCOMMON_LOGGER_ENDL;

			ibrcommon::MutexLock l(_bundleslock);
			for (std::set<dtn::data::Bundle>::const_iterator iter = _bundles.begin(); iter != _bundles.end(); iter++)
			{
				const dtn::data::Bundle &bundle = (*iter);

				try {
					// if singleton destination is requested and bundle destination is not a singleton continue ...
					if (singleton && !(bundle._procflags & dtn::data::PrimaryBlock::DESTINATION_IS_SINGLETON)) continue;

					// exact match of destination is requested
					if (exact)
					{
						// continue if destination does not match exactly
						if (bundle._destination != eid) continue;
					}
					else
					{
						// continue if destination node name does not match
						if (bundle._destination.getNodeEID() != eid.getNodeEID()) continue;
					}

					// return the current bundle
					return bundle;
				} catch (const dtn::InvalidDataException &ex) {
					IBRCOMMON_LOGGER_DEBUG(10) << "InvalidDataException on bundle get: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
				}
			}

			throw BundleStorage::NoBundleFoundException();
		}

		const std::list<dtn::data::MetaBundle> MemoryBundleStorage::get(const BundleFilterCallback &cb)
		{
			// result list
			std::list<dtn::data::MetaBundle> result;

			// we have to iterate through all bundles
			ibrcommon::MutexLock l(_bundleslock);

			for (std::set<dtn::data::Bundle>::const_iterator iter = _bundles.begin(); iter != _bundles.end(); iter++)
			{
				const dtn::data::Bundle &bundle = (*iter);

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

#include "core/SimpleBundleStorage.h"
#include "core/TimeEvent.h"
#include "core/GlobalEvent.h"
#include "core/BundleExpiredEvent.h"
#include "core/BundleEvent.h"

#include <ibrdtn/data/AgeBlock.h>
#include <ibrdtn/utils/Utils.h>
#include <ibrcommon/thread/MutexLock.h>
#include <ibrcommon/Logger.h>
#include <ibrcommon/AutoDelete.h>

#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cerrno>

namespace dtn
{
	namespace core
	{
		SimpleBundleStorage::SimpleBundleStorage(const ibrcommon::File &workdir, size_t maxsize)
		 : _datastore(*this, workdir), _maxsize(maxsize), _currentsize(0)
		{
			// load persistent bundles
			_datastore.iterateAll();

			// some output
			IBRCOMMON_LOGGER(info) << _bundles.size() << " Bundles restored." << IBRCOMMON_LOGGER_ENDL;
		}

		SimpleBundleStorage::~SimpleBundleStorage()
		{
		}

		void SimpleBundleStorage::eventDataStorageStored(const dtn::core::DataStorage::Hash &hash)
		{
			ibrcommon::MutexLock l(_bundleslock);
			dtn::data::MetaBundle meta(_pending_bundles[hash]);
			_pending_bundles.erase(hash);
			_stored_bundles[meta] = hash;
		}

		void SimpleBundleStorage::eventDataStorageStoreFailed(const dtn::core::DataStorage::Hash&, const ibrcommon::Exception &ex)
		{
			IBRCOMMON_LOGGER(error) << "store failed: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
		}

		void SimpleBundleStorage::eventDataStorageRemoved(const dtn::core::DataStorage::Hash &hash)
		{
			ibrcommon::MutexLock l(_bundleslock);

			for (std::map<dtn::data::MetaBundle, DataStorage::Hash>::iterator iter = _stored_bundles.begin();
					iter != _stored_bundles.end(); iter++)
			{
				if (iter->second == hash)
				{
					// decrement the storage size
					_currentsize -= _bundle_size[iter->first];

					_bundle_size.erase(iter->first);

					_stored_bundles.erase(iter);
					return;
				}
			}
		}

		void SimpleBundleStorage::eventDataStorageRemoveFailed(const dtn::core::DataStorage::Hash&, const ibrcommon::Exception &ex)
		{
			IBRCOMMON_LOGGER(error) << "remove failed: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
		}

		void SimpleBundleStorage::iterateDataStorage(const dtn::core::DataStorage::Hash &hash, dtn::core::DataStorage::istream &stream)
		{
			try {
				dtn::data::Bundle bundle;
				dtn::data::DefaultDeserializer ds(*stream);

				// load a bundle into the storage
				ds >> bundle;

				// extract meta data
				dtn::data::MetaBundle meta(bundle);

				// lock the bundle lists
				ibrcommon::MutexLock l(_bundleslock);

				// add the bundle to the stored bundles
				_stored_bundles[meta] = hash;

				// increment the storage size
				_bundle_size[meta] = (*stream).tellg();
				_currentsize += (*stream).tellg();

				// add it to the bundle list
				dtn::data::BundleList::add(meta);

			} catch (const std::exception&) {
				// report this error to the console
				IBRCOMMON_LOGGER(error) << "Error: Unable to restore bundle in file " << hash.value << IBRCOMMON_LOGGER_ENDL;

				// error while reading file
				_datastore.remove(hash);
			}
		}

		dtn::data::Bundle SimpleBundleStorage::__get(const dtn::data::MetaBundle &meta)
		{
			DataStorage::Hash hash(meta.toString());
			std::map<DataStorage::Hash, dtn::data::Bundle>::iterator it = _pending_bundles.find(hash);

			if (_pending_bundles.end() != it)
			{
				return it->second;
			}

			try {
				DataStorage::istream stream = _datastore.retrieve(hash);

				// load the bundle from the storage
				dtn::data::Bundle bundle;

				// load the bundle from file
				try {
					dtn::data::DefaultDeserializer(*stream) >> bundle;
				} catch (const std::exception &ex) {
					throw dtn::SerializationFailedException("bundle get failed: " + std::string(ex.what()));
				}

				try {
					dtn::data::AgeBlock &agebl = bundle.getBlock<dtn::data::AgeBlock>();

					// modify the AgeBlock with the age of the file
					time_t age = stream.lastaccess() - stream.lastmodify();

					agebl.addAge(age);
				} catch (const dtn::data::Bundle::NoSuchBlockFoundException&) { };

				return bundle;
			} catch (const DataStorage::DataNotAvailableException&) {
				throw BundleStorage::NoBundleFoundException();
			}
		}

		void SimpleBundleStorage::componentUp()
		{
			bindEvent(TimeEvent::className);
			_datastore.start();
		}

		void SimpleBundleStorage::componentDown()
		{
			unbindEvent(TimeEvent::className);
			_datastore.stop();
			_datastore.join();
		}

		void SimpleBundleStorage::raiseEvent(const Event *evt)
		{
			try {
				const TimeEvent &time = dynamic_cast<const TimeEvent&>(*evt);
				if (time.getAction() == dtn::core::TIME_SECOND_TICK)
				{
					ibrcommon::MutexLock l(_bundleslock);
					dtn::data::BundleList::expire(time.getTimestamp());
				}
			} catch (const std::bad_cast&) { }
		}

		const std::string SimpleBundleStorage::getName() const
		{
			return "SimpleBundleStorage";
		}

		bool SimpleBundleStorage::empty()
		{
			ibrcommon::MutexLock l(_bundleslock);
			return dtn::data::BundleList::empty();
		}

		void SimpleBundleStorage::releaseCustody(dtn::data::BundleID&)
		{
			// custody is successful transferred to another node.
			// it is safe to delete this bundle now. (depending on the routing algorithm.)
		}

		unsigned int SimpleBundleStorage::count()
		{
			ibrcommon::MutexLock l(_bundleslock);
			return dtn::data::BundleList::size();
		}

		const dtn::data::MetaBundle SimpleBundleStorage::getByFilter(const ibrcommon::BloomFilter &filter)
		{
			// search for one bundle which is not contained in the filter
			// until we have a better strategy, we have to iterate through all bundles
			ibrcommon::MutexLock l(_bundleslock);
			for (std::set<dtn::data::MetaBundle>::const_iterator iter = begin(); iter != end(); iter++)
			{
				const dtn::data::MetaBundle &meta = (*iter);

				try {
					if ( !filter.contains(meta.toString()) )
					{
						// check if the destination is blocked by the destination filter
						if ( (_destination_filter.find(meta.destination) == _destination_filter.end()) )
						{
							IBRCOMMON_LOGGER_DEBUG(10) << meta.toString() << " is not in the bloomfilter" << IBRCOMMON_LOGGER_ENDL;
							return __get(meta);
						}
					}
				} catch (const dtn::InvalidDataException &ex) {
					IBRCOMMON_LOGGER_DEBUG(10) << "InvalidDataException on bundle get: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
				}
			}

			throw BundleStorage::NoBundleFoundException();
		}

		const dtn::data::MetaBundle SimpleBundleStorage::getByDestination(const dtn::data::EID &eid, bool exact, bool singleton)
		{
			IBRCOMMON_LOGGER_DEBUG(5) << "Storage: get bundle for " << eid.getString() << IBRCOMMON_LOGGER_ENDL;
			
			ibrcommon::MutexLock l(_bundleslock);
			for (std::set<dtn::data::MetaBundle>::const_iterator iter = begin(); iter != end(); iter++)
			{
				const dtn::data::MetaBundle &meta = (*iter);

				try {
					// if singleton destination is requested and bundle destination is not a singleton continue ...
					if (singleton && !(meta.procflags & dtn::data::PrimaryBlock::DESTINATION_IS_SINGLETON)) continue;

					// exact match of destination is requested
					if (exact)
					{
						// continue if destination does not match exactly
						if (meta.destination != eid) continue;
					}
					else
					{
						// continue if destination node name does not match
						if (meta.destination.getNodeEID() != eid.getNodeEID()) continue;
					}

					// return the current bundle
					return __get(meta);
				} catch (const dtn::InvalidDataException &ex) {
					IBRCOMMON_LOGGER_DEBUG(10) << "InvalidDataException on bundle get: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
				}
			}

			throw BundleStorage::NoBundleFoundException();
		}

		dtn::data::Bundle SimpleBundleStorage::get(const dtn::data::BundleID &id)
		{
			try {
				ibrcommon::MutexLock l(_bundleslock);

				for (std::set<dtn::data::MetaBundle>::const_iterator iter = begin(); iter != end(); iter++)
				{
					const dtn::data::MetaBundle &meta = (*iter);
					if (id == meta)
					{
						return __get(meta);
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

		void SimpleBundleStorage::store(const dtn::data::Bundle &bundle)
		{
			ibrcommon::MutexLock l(_bundleslock);

			// get the bundle size
			dtn::data::DefaultSerializer s(std::cout);
			size_t bundle_size = s.getLength(bundle);

			// check if this container is too big for us.
			if ((_maxsize > 0) && (_currentsize + bundle_size > _maxsize))
			{
				throw StorageSizeExeededException();
			}

			// store the bundle
			DataStorage::Hash hash = _datastore.store(new BundleContainer(bundle));

			dtn::data::MetaBundle meta(bundle);

			// add the bundle to the stored bundles
			_pending_bundles[hash] = bundle;

			// increment the storage size
			_bundle_size[meta] = bundle_size;
			_currentsize += bundle_size;

			// add it to the bundle list
			dtn::data::BundleList::add(meta);
		}

		void SimpleBundleStorage::remove(const dtn::data::BundleID &id)
		{
			ibrcommon::MutexLock l(_bundleslock);

			for (std::set<dtn::data::MetaBundle>::const_iterator iter = begin(); iter != end(); iter++)
			{
				if ((*iter) == id)
				{
					// remove item in the bundlelist
					dtn::data::MetaBundle meta = (*iter);

					// remove it from the bundle list
					dtn::data::BundleList::remove(meta);

					DataStorage::Hash hash(meta.toString());

					// create a background task for removing the bundle
					_datastore.remove(hash);

					return;
				}
			}

			throw BundleStorage::NoBundleFoundException();
		}

		dtn::data::MetaBundle SimpleBundleStorage::remove(const ibrcommon::BloomFilter &filter)
		{
			ibrcommon::MutexLock l(_bundleslock);

			for (std::set<dtn::data::MetaBundle>::const_iterator iter = begin(); iter != end(); iter++)
			{
				// remove item in the bundlelist
				const dtn::data::MetaBundle meta = (*iter);

				if ( filter.contains(meta.toString()) )
				{
					// remove it from the bundle list
					dtn::data::BundleList::remove(meta);

					DataStorage::Hash hash(meta.toString());

					// create a background task for removing the bundle
					_datastore.remove(hash);

					return meta;
				}
			}

			throw BundleStorage::NoBundleFoundException();
		}

		size_t SimpleBundleStorage::size() const
		{
			return _currentsize;
		}

		void SimpleBundleStorage::clear()
		{
			ibrcommon::MutexLock l(_bundleslock);

			// mark all bundles for deletion
			for (std::set<dtn::data::MetaBundle>::const_iterator iter = begin(); iter != end(); iter++)
			{
				// remove item in the bundlelist
				const dtn::data::MetaBundle meta = (*iter);

				DataStorage::Hash hash(meta.toString());

				// create a background task for removing the bundle
				_datastore.remove(hash);
			}

			_bundles.clear();
			dtn::data::BundleList::clear();

			// set the storage size to zero
			_currentsize = 0;
		}

		void SimpleBundleStorage::eventBundleExpired(const ExpiringBundle &b)
		{
			for (std::set<dtn::data::MetaBundle>::const_iterator iter = begin(); iter != end(); iter++)
			{
				if ((*iter) == b.bundle)
				{
					// remove the bundle
					const dtn::data::MetaBundle &meta = (*iter);

					DataStorage::Hash hash(meta.toString());

					// create a background task for removing the bundle
					_datastore.remove(hash);

					break;
				}
			}

			// raise bundle event
			dtn::core::BundleEvent::raise( b.bundle, dtn::core::BUNDLE_DELETED, dtn::data::StatusReportBlock::LIFETIME_EXPIRED);

			// raise an event
			dtn::core::BundleExpiredEvent::raise( b.bundle );
		}

		const std::list<dtn::data::BundleID> SimpleBundleStorage::getList()
		{
			ibrcommon::MutexLock l(_bundleslock);
			std::list<dtn::data::BundleID> ret;

			for (std::set<dtn::data::MetaBundle>::const_iterator iter = begin(); iter != end(); iter++)
			{
				ret.push_back( *iter );
			}

			return ret;
		}

		SimpleBundleStorage::BundleContainer::BundleContainer(const dtn::data::Bundle &b)
		 : _bundle(b)
		{ }

		SimpleBundleStorage::BundleContainer::~BundleContainer()
		{ }

		std::string SimpleBundleStorage::BundleContainer::getKey() const
		{
			return dtn::data::BundleID(_bundle).toString();
		}

		std::ostream& SimpleBundleStorage::BundleContainer::serialize(std::ostream &stream)
		{
			// get an serializer for bundles
			dtn::data::DefaultSerializer s(stream);

			// length of the bundle
			unsigned int size = s.getLength(_bundle);

			// serialize the bundle
			s << _bundle; stream.flush();

			// check the streams health
			if (!stream.good())
			{
				std::stringstream ss; ss << "Output stream went bad [" << std::strerror(errno) << "]";
				throw dtn::SerializationFailedException(ss.str());
			}

			// get the write position
			if (size > stream.tellp())
			{
				std::stringstream ss; ss << "Not all data were written [" << stream.tellp() << " of " << size << " bytes]";
				throw dtn::SerializationFailedException(ss.str());
			}

			// return the stream, this allows stacking
			return stream;
		}
	}
}

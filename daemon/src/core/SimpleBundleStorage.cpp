#include "core/SimpleBundleStorage.h"
#include "core/TimeEvent.h"
#include "core/GlobalEvent.h"
#include "core/BundleExpiredEvent.h"
#include "core/BundleEvent.h"

#include <ibrdtn/utils/Utils.h>
#include <ibrcommon/thread/MutexLock.h>
#include <ibrcommon/Logger.h>
#include <ibrcommon/AutoDelete.h>

#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>

namespace dtn
{
	namespace core
	{
		SimpleBundleStorage::SimpleBundleStorage(size_t maxsize)
		 : _running(true), _mode(MODE_NONPERSISTENT), _maxsize(maxsize), _currentsize(0)
		{
		}

		SimpleBundleStorage::SimpleBundleStorage(const ibrcommon::File &workdir, size_t maxsize)
		 : _running(true), _workdir(workdir), _mode(MODE_PERSISTENT), _maxsize(maxsize), _currentsize(0)
		{
			// load persistent bundles
			std::list<ibrcommon::File> files;
			_workdir.getFiles(files);

			for (std::list<ibrcommon::File>::iterator iter = files.begin(); iter != files.end(); iter++)
			{
				ibrcommon::File &file = (*iter);
				if (!file.isDirectory() && !file.isSystem())
				{
					try {
						// load a bundle into the storage
						load(file);
					} catch (ibrcommon::IOException ex) {
						// report this error to the console
						IBRCOMMON_LOGGER(error) << "Error: Unable to restore bundle in file " << file.getPath() << IBRCOMMON_LOGGER_ENDL;

						// error while reading file
						file.remove();
					} catch (dtn::InvalidDataException ex) {
						// report this error to the console
						IBRCOMMON_LOGGER(error) << "Error: Unable to restore bundle in file " << file.getPath() << IBRCOMMON_LOGGER_ENDL;

						// error while reading file
						file.remove();
					}
				}
			}

//			// debug output of storage content
//			for (std::set<BundleContainer>::const_iterator iter = bundles.begin(); iter != bundles.end(); iter++)
//			{
//				const BundleContainer &container = (*iter);
//
//				std::cout << container.toString() << ", Priority: " << container.getPriority() << std::endl;
//			}

			// some output
			IBRCOMMON_LOGGER(info) << _bundles.size() << " Bundles restored." << IBRCOMMON_LOGGER_ENDL;
		}

		SimpleBundleStorage::~SimpleBundleStorage()
		{
			_tasks.abort();
			join();
		}

		void SimpleBundleStorage::componentUp()
		{
			bindEvent(TimeEvent::className);
		}

		void SimpleBundleStorage::componentDown()
		{
			unbindEvent(TimeEvent::className);

			_tasks.abort();
		}

		void SimpleBundleStorage::componentRun()
		{
			try {
				while (true)
				{
					Task *t = _tasks.getnpop(true);
					ibrcommon::AutoDelete<Task> ad(t);

					try {
						t->run(*this);
					} catch (const std::exception &ex) {
						IBRCOMMON_LOGGER(error) << "error in storage: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
					}

					yield();
				}
			} catch (const std::exception &ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "SimpleBundleStorage exited with " << ex.what() << IBRCOMMON_LOGGER_ENDL;
			}
		}

		bool SimpleBundleStorage::__cancellation()
		{
			_tasks.abort();
			return true;
		}

		void SimpleBundleStorage::raiseEvent(const Event *evt)
		{
			try {
				const TimeEvent &time = dynamic_cast<const TimeEvent&>(*evt);
				if (time.getAction() == dtn::core::TIME_SECOND_TICK)
				{
					_tasks.push( new SimpleBundleStorage::TaskExpireBundles(time.getTimestamp()) );
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
			return _bundles.empty();
		}

		void SimpleBundleStorage::releaseCustody(dtn::data::BundleID&)
		{
			// custody is successful transferred to another node.
			// it is safe to delete this bundle now. (depending on the routing algorithm.)
		}

		unsigned int SimpleBundleStorage::count()
		{
			ibrcommon::MutexLock l(_bundleslock);
			return _bundles.size();
		}

		dtn::data::Bundle SimpleBundleStorage::get(const ibrcommon::BloomFilter &filter)
		{
			// search for one bundle which is not contained in the filter
			// until we have a better strategy, we have to iterate through all bundles
			ibrcommon::MutexLock l(_bundleslock);
			for (std::set<BundleContainer>::const_iterator iter = _bundles.begin(); iter != _bundles.end(); iter++)
			{
				const BundleContainer &container = (*iter);

				try {
					if (!filter.contains(container.toString()))
					{
						IBRCOMMON_LOGGER_DEBUG(10) << container.toString() << " is not in the bloomfilter" << IBRCOMMON_LOGGER_ENDL;
						return container.get();
					}
				} catch (const dtn::InvalidDataException &ex) {
					IBRCOMMON_LOGGER_DEBUG(10) << "InvalidDataException on bundle get: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
				}
			}

			throw BundleStorage::NoBundleFoundException();
		}

		dtn::data::Bundle SimpleBundleStorage::get(const dtn::data::EID &eid, const bool appsensitive)
		{
			IBRCOMMON_LOGGER_DEBUG(5) << "Storage: get bundle for " << eid.getString() << IBRCOMMON_LOGGER_ENDL;
			
			ibrcommon::MutexLock l(_bundleslock);
			for (std::set<BundleContainer>::const_iterator iter = _bundles.begin(); iter != _bundles.end(); iter++)
			{
				const BundleContainer &bundle = (*iter);

				try {
					if (appsensitive)
					{
						if (bundle.destination == eid)
						{
							return bundle.get();
						}
					}
					else
					{
						if (bundle.destination.getNodeEID() == eid.getNodeEID())
						{
							return bundle.get();
						}
					}
				} catch (const dtn::InvalidDataException &ex) {
					IBRCOMMON_LOGGER_DEBUG(10) << "InvalidDataException on bundle get: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
				}
			}

			throw BundleStorage::NoBundleFoundException();
		}

		dtn::data::Bundle SimpleBundleStorage::get(const dtn::data::BundleID &id)
		{
			ibrcommon::MutexLock l(_bundleslock);
			try {
				for (std::set<BundleContainer>::const_iterator iter = _bundles.begin(); iter != _bundles.end(); iter++)
				{
					const BundleContainer &bundle = (*iter);
					if (id == bundle)
					{
						return bundle.get();
					}
				}
			} catch (dtn::SerializationFailedException) {
				// bundle loading failed
			}

			throw BundleStorage::NoBundleFoundException();
		}

		void SimpleBundleStorage::invokeExpiration(const size_t timestamp)
		{
			ibrcommon::MutexLock l(_bundleslock);
			dtn::data::BundleList::expire(timestamp);
		}

		void SimpleBundleStorage::load(const ibrcommon::File &file)
		{
			ibrcommon::MutexLock l(_bundleslock);

			BundleContainer container(file);
			_bundles.insert( container );

			// increment the storage size
			_currentsize += container.size();

			// add it to the bundle list
			dtn::data::BundleList::add(container);
		}

		void SimpleBundleStorage::store(const dtn::data::Bundle &bundle)
		{
			ibrcommon::MutexLock l(_bundleslock);

			// create a bundle container in the memory
			BundleContainer container(bundle);

			// check if this container is too big for us.
			if ((_maxsize > 0) && (_currentsize + container.size() > _maxsize))
			{
				throw StorageSizeExeededException();
			}

			// increment the storage size
			_currentsize += container.size();

			// if we are persistent, make a persistent container
			if (_mode == MODE_PERSISTENT)
			{
				container = BundleContainer(bundle, _workdir, container.size());

				// create a background task for storing the bundle
				_tasks.push( new SimpleBundleStorage::TaskStoreBundle(container) );
			}

			// insert Container
			pair<set<BundleContainer>::iterator,bool> ret = _bundles.insert( container );

			if (ret.second)
			{
				dtn::data::BundleList::add(dtn::data::MetaBundle(bundle));
			}
			else
			{
				IBRCOMMON_LOGGER_DEBUG(5) << "Storage: got bundle duplicate " << bundle.toString() << IBRCOMMON_LOGGER_ENDL;
			}
		}

		void SimpleBundleStorage::remove(const dtn::data::BundleID &id)
		{
			ibrcommon::MutexLock l(_bundleslock);

			for (std::set<BundleContainer>::const_iterator iter = _bundles.begin(); iter != _bundles.end(); iter++)
			{
				if ( id == (*iter) )
				{
					// remove item in the bundlelist
					BundleContainer container = (*iter);

					// remove it from the bundle list
					dtn::data::BundleList::remove(container);

					// decrement the storage size
					_currentsize -= container.size();

					// create a background task for removing the bundle
					_tasks.push( new SimpleBundleStorage::TaskRemoveBundle(container) );

					// remove the container
					_bundles.erase(iter);

					return;
				}
			}

			throw BundleStorage::NoBundleFoundException();
		}

		dtn::data::MetaBundle SimpleBundleStorage::remove(const ibrcommon::BloomFilter &filter)
		{
			ibrcommon::MutexLock l(_bundleslock);

			for (std::set<BundleContainer>::const_iterator iter = _bundles.begin(); iter != _bundles.end(); iter++)
			{
				const BundleContainer &container = (*iter);

				if ( filter.contains(container.toString()) )
				{
					// remove item in the bundlelist
					BundleContainer container = (*iter);

					// remove it from the bundle list
					dtn::data::BundleList::remove(container);

					// decrement the storage size
					_currentsize -= container.size();

					// create a background task for removing the bundle
					_tasks.push( new SimpleBundleStorage::TaskRemoveBundle(container) );

					// remove the container
					_bundles.erase(iter);

					return (MetaBundle)container;
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
			for (std::set<BundleContainer>::iterator iter = _bundles.begin(); iter != _bundles.end(); iter++)
			{
				BundleContainer container = (*iter);

				// mark for deletion
				container.remove();
			}

			_bundles.clear();
			dtn::data::BundleList::clear();

			// set the storage size to zero
			_currentsize = 0;
		}

		void SimpleBundleStorage::eventBundleExpired(const ExpiringBundle &b)
		{
			for (std::set<BundleContainer>::const_iterator iter = _bundles.begin(); iter != _bundles.end(); iter++)
			{
				if ( b.bundle == (*iter) )
				{
					BundleContainer container = (*iter);

					// decrement the storage size
					_currentsize -= container.size();

					container.remove();
					_bundles.erase(iter);
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

			for (std::set<BundleContainer>::const_iterator iter = _bundles.begin(); iter != _bundles.end(); iter++)
			{
				const BundleContainer &container = (*iter);
				ret.push_back( container );
			}

			return ret;
		}

		SimpleBundleStorage::BundleContainer::BundleContainer(const dtn::data::Bundle &b)
		 : dtn::data::MetaBundle(b), _holder( new SimpleBundleStorage::BundleContainer::Holder(b) )
		{
		}

		SimpleBundleStorage::BundleContainer::BundleContainer(const ibrcommon::File &file)
		 : dtn::data::MetaBundle(), _holder( new SimpleBundleStorage::BundleContainer::Holder(file) )
		{
			dtn::data::Bundle b = _holder->getBundle();
			(dtn::data::MetaBundle&)(*this) = dtn::data::MetaBundle(b);
		}

		SimpleBundleStorage::BundleContainer::BundleContainer(const dtn::data::Bundle &b, const ibrcommon::File &workdir, const size_t size)
		 : dtn::data::MetaBundle(b), _holder( new SimpleBundleStorage::BundleContainer::Holder(b, workdir, size) )
		{
		}

		SimpleBundleStorage::BundleContainer::~BundleContainer()
		{
			down();
		}

		void SimpleBundleStorage::BundleContainer::down()
		{
			try {
				ibrcommon::MutexLock l(_holder->lock);
				if (--_holder->_count == 0) throw true;
			} catch (bool) {
				delete _holder;
			}
		}

		size_t SimpleBundleStorage::BundleContainer::size() const
		{
			return _holder->size();
		}

		bool SimpleBundleStorage::BundleContainer::operator<(const SimpleBundleStorage::BundleContainer& other) const
		{
			MetaBundle &left = (MetaBundle&)*this;
			MetaBundle &right = (MetaBundle&)other;

			if (left.getPriority() > right.getPriority()) return true;
			if (left.getPriority() != right.getPriority()) return false;
			return (left < right);
		}

		SimpleBundleStorage::BundleContainer& SimpleBundleStorage::BundleContainer::operator= (const SimpleBundleStorage::BundleContainer &right)
		{
			ibrcommon::MutexLock l(right._holder->lock);
			++right._holder->_count;

			down();

			_holder = right._holder;
			return *this;
		}

		SimpleBundleStorage::BundleContainer::BundleContainer(const SimpleBundleStorage::BundleContainer& right)
		 : dtn::data::MetaBundle(right), _holder(right._holder)
		{
			ibrcommon::MutexLock l(_holder->lock);
			++_holder->_count;
		}

		dtn::data::Bundle SimpleBundleStorage::BundleContainer::get() const
		{
			return _holder->getBundle();
		}

		void SimpleBundleStorage::BundleContainer::remove()
		{
			_holder->remove();
		}

		void SimpleBundleStorage::BundleContainer::invokeStore()
		{
			_holder->invokeStore();
		}

		SimpleBundleStorage::BundleContainer::Holder::Holder( const dtn::data::Bundle &b )
		 :  _count(1), _state(HOLDER_MEMORY), _bundle(b)
		{
			dtn::data::DefaultSerializer s(std::cout);
			_size = s.getLength(_bundle);
		}

		SimpleBundleStorage::BundleContainer::Holder::Holder( const ibrcommon::File &file )
		 : _count(1), _state(HOLDER_STORED), _bundle(), _container(file)
		{
			_size = file.size();
		}

		SimpleBundleStorage::BundleContainer::Holder::Holder( const dtn::data::Bundle &b, const ibrcommon::File &workdir, const size_t size )
		 : _count(1), _state(HOLDER_PENDING), _bundle(b), _container(workdir), _size(size)
		{
		}

		SimpleBundleStorage::BundleContainer::Holder::~Holder()
		{
			ibrcommon::MutexLock l(_state_lock);
			if ((_state == HOLDER_DELETED) && (_container.exists()))
			{
				_container.remove();
			}
		}

		size_t SimpleBundleStorage::BundleContainer::Holder::size() const
		{
			return _size;
		}

		dtn::data::Bundle SimpleBundleStorage::BundleContainer::Holder::getBundle()
		{
			ibrcommon::MutexLock l(_state_lock);
			if (_state == HOLDER_DELETED)
			{
				throw dtn::SerializationFailedException("bundle deleted");
			}
			else if (_state == HOLDER_STORED)
			{
				dtn::data::Bundle bundle;

				// we need to load the bundle from file
				ibrcommon::locked_ifstream ostream(_container, ios::in|ios::binary);
				std::istream &fs = (*ostream);
				try {
					fs.exceptions(std::ios::badbit | std::ios::eofbit);
					dtn::data::DefaultDeserializer(fs, dtn::core::BundleCore::getInstance()) >> bundle;
				} catch (ios_base::failure ex) {
					throw dtn::SerializationFailedException("can not load bundle data (" + std::string(ex.what()) + ")");
				} catch (const std::exception &ex) {
					throw dtn::SerializationFailedException("bundle get failed: " + std::string(ex.what()));
				}

				ostream.close();

				return bundle;
			}
			else
			{
				return _bundle;
			}
		}

		void SimpleBundleStorage::BundleContainer::Holder::remove()
		{
			ibrcommon::MutexLock l(_state_lock);
			_state = HOLDER_DELETED;
		}

		void SimpleBundleStorage::BundleContainer::Holder::invokeStore()
		{
			ibrcommon::MutexLock l(_state_lock);
			try {
				if (_state == HOLDER_PENDING)
				{
					_container = ibrcommon::TemporaryFile(_container, "bundle");
					ibrcommon::locked_ofstream ostream(_container);
					std::ostream &out = (*ostream);

					if (!ostream.is_open()) throw ibrcommon::IOException("can not open file");
					dtn::data::DefaultSerializer(out) << _bundle; out << std::flush;
					ostream.close();

					if (_container.size() == 0)
					{
						throw ibrcommon::IOException("Write of bundle failed. File has size of zero.");
					}

					_size = _container.size();
					_bundle = dtn::data::Bundle();
					_state = HOLDER_STORED;
				}
			} catch (const std::exception &ex) {
				throw dtn::SerializationFailedException("can not write data to the storage; " + std::string(ex.what()));
			}
		}

		SimpleBundleStorage::TaskStoreBundle::TaskStoreBundle(const SimpleBundleStorage::BundleContainer &container)
		 : _container(container)
		{ }

		SimpleBundleStorage::TaskStoreBundle::~TaskStoreBundle()
		{ }

		void SimpleBundleStorage::TaskStoreBundle::run(SimpleBundleStorage &storage)
		{
			try {
				_container.invokeStore();
				IBRCOMMON_LOGGER_DEBUG(20) << "bundle stored " << _container.toString() << " (size: " << _container.size() << ")" << IBRCOMMON_LOGGER_ENDL;
			} catch (const dtn::SerializationFailedException &ex) {
				IBRCOMMON_LOGGER(error) << "failed to store bundle " << _container.toString() << " (size: " << _container.size() << "): " << ex.what() << IBRCOMMON_LOGGER_ENDL;
			} catch (const ibrcommon::IOException &ex) {
				IBRCOMMON_LOGGER(error) << "failed to store bundle " << _container.toString() << " (size: " << _container.size() << "): " << ex.what() << IBRCOMMON_LOGGER_ENDL;
				// reschedule the task
				storage._tasks.push( new SimpleBundleStorage::TaskStoreBundle(_container) );
			}
		}

		SimpleBundleStorage::TaskRemoveBundle::TaskRemoveBundle(const SimpleBundleStorage::BundleContainer &container)
		 : _container(container)
		{ }

		SimpleBundleStorage::TaskRemoveBundle::~TaskRemoveBundle()
		{ }

		void SimpleBundleStorage::TaskRemoveBundle::run(SimpleBundleStorage&)
		{
			// mark for deletion
			_container.remove();
		}

		SimpleBundleStorage::TaskExpireBundles::TaskExpireBundles(const size_t &timestamp)
		 : _timestamp(timestamp)
		{ }

		SimpleBundleStorage::TaskExpireBundles::~TaskExpireBundles()
		{ }

		void SimpleBundleStorage::TaskExpireBundles::run(SimpleBundleStorage &storage)
		{
			storage.invokeExpiration(_timestamp);
		}
	}
}

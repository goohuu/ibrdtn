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
		 : _store(maxsize), _running(true)
		{
			bindEvent(TimeEvent::className);
			bindEvent(GlobalEvent::className);
		}

		SimpleBundleStorage::SimpleBundleStorage(const ibrcommon::File &workdir, size_t maxsize)
		 : _store(workdir, maxsize), _running(true)
		{
		}

		SimpleBundleStorage::~SimpleBundleStorage()
		{
		}

		void SimpleBundleStorage::componentUp()
		{
			bindEvent(TimeEvent::className);
		}

		void SimpleBundleStorage::componentDown()
		{
			unbindEvent(TimeEvent::className);

			_tasks.unblock();
		}

		void SimpleBundleStorage::componentRun()
		{
			try {
				while (isRunning())
				{
					Task *t = _tasks.blockingpop();
					ibrcommon::AutoDelete<Task> ad(t);

					try {
						t->run();
					} catch (...) {

					}
				}
			} catch (ibrcommon::Exception) {

			}
		}

		void SimpleBundleStorage::raiseEvent(const Event *evt)
		{
			const TimeEvent *time = dynamic_cast<const TimeEvent*>(evt);

			if (time != NULL)
			{
				if (time->getAction() == dtn::core::TIME_SECOND_TICK)
				{
					_store.expire(time->getTimestamp());
				}
			}
		}

		void SimpleBundleStorage::clear()
		{
			// delete all bundles
			_store.clear();
		}

		bool SimpleBundleStorage::empty()
		{
			ibrcommon::MutexLock l(_store.bundleslock);
			return _store.bundles.empty();
		}

		unsigned int SimpleBundleStorage::count()
		{
			return _store.count();
		}

		size_t SimpleBundleStorage::size() const
		{
			return _store.size();
		}

		void SimpleBundleStorage::releaseCustody(dtn::data::BundleID&)
		{
			// custody is successful transferred to another node.
			// it is safe to delete this bundle now. (depending on the routing algorithm.)
		}

		unsigned int SimpleBundleStorage::BundleStore::count()
		{
			ibrcommon::MutexLock l(bundleslock);
			return bundles.size();
		}

		void SimpleBundleStorage::store(const dtn::data::Bundle &bundle)
		{
			_store.store(bundle, *this);
			IBRCOMMON_LOGGER_DEBUG(5) << "Storage: stored bundle " << bundle.toString() << IBRCOMMON_LOGGER_ENDL;
			dtn::core::BundleEvent::raise(bundle, dtn::core::BUNDLE_STORED);
		}

		dtn::data::Bundle SimpleBundleStorage::get(const dtn::data::EID &eid)
		{
			IBRCOMMON_LOGGER_DEBUG(5) << "Storage: get bundle for " << eid.getString() << IBRCOMMON_LOGGER_ENDL;
			return _store.get(eid);
		}

		dtn::data::Bundle SimpleBundleStorage::get(const dtn::data::BundleID &id)
		{
			return _store.get(id);
		}

		dtn::data::Bundle SimpleBundleStorage::get(const ibrcommon::BloomFilter &filter)
		{
			return _store.get(filter);
		}

		const std::list<dtn::data::BundleID> SimpleBundleStorage::getList()
		{
			return _store.getList();
		}

		void SimpleBundleStorage::remove(const dtn::data::BundleID &id)
		{
			_store.remove(id);
		}

		SimpleBundleStorage::BundleStore::BundleStore(size_t maxsize)
		 : _mode(MODE_NONPERSISTENT), _maxsize(maxsize), _currentsize(0)
		{
		}

		SimpleBundleStorage::BundleStore::BundleStore(ibrcommon::File workdir, size_t maxsize)
		 : _workdir(workdir), _mode(MODE_PERSISTENT), _maxsize(maxsize), _currentsize(0)
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

			// some output
			IBRCOMMON_LOGGER(info) << bundles.size() << " Bundles restored." << IBRCOMMON_LOGGER_ENDL;
		}

		SimpleBundleStorage::BundleStore::~BundleStore()
		{
		}

		dtn::data::Bundle SimpleBundleStorage::BundleStore::get(const ibrcommon::BloomFilter &filter)
		{
//			// print summary vector
//			const char *data = reinterpret_cast<const char*>(filter.table());
//
//			for (size_t i = 0; i < filter.size(); i++)
//			{
//				printf("%02x:", *data);
//				data++;
//			}
//
//			printf("\n");

			// search for one bundle which is not contained in the filter
			// until we have a better strategy, we have to iterate through all bundles
			ibrcommon::MutexLock l(bundleslock);
			for (std::set<BundleContainer>::const_iterator iter = bundles.begin(); iter != bundles.end(); iter++)
			{
				const BundleContainer &bundle = (*iter);
				const BundleID id(*bundle);

				if (!filter.contains(id.toString()))
				{
					IBRCOMMON_LOGGER_DEBUG(10) << id.toString() << " is not in the bloomfilter" << IBRCOMMON_LOGGER_ENDL;
					return (*bundle);
				}
			}

			throw BundleStorage::NoBundleFoundException();
		}

		dtn::data::Bundle SimpleBundleStorage::BundleStore::get(const dtn::data::EID &eid)
		{
			ibrcommon::MutexLock l(bundleslock);
			for (std::set<BundleContainer>::const_iterator iter = bundles.begin(); iter != bundles.end(); iter++)
			{
				const BundleContainer &bundle = (*iter);
				if ((*bundle)._destination == eid)
				{
					return (*bundle);
				}
			}

			throw BundleStorage::NoBundleFoundException();
		}

		dtn::data::Bundle SimpleBundleStorage::BundleStore::get(const dtn::data::BundleID &id)
		{
			ibrcommon::MutexLock l(bundleslock);

			for (std::set<BundleContainer>::const_iterator iter = bundles.begin(); iter != bundles.end(); iter++)
			{
				const BundleContainer &bundle = (*iter);
				if (id == (*bundle))
				{
					return (*bundle);
				}
			}

			throw dtn::core::BundleStorage::NoBundleFoundException();
		}

		void SimpleBundleStorage::BundleStore::expire(const size_t timestamp)
		{
			ibrcommon::MutexLock l(bundleslock);
			dtn::data::BundleList::expire(timestamp);
		}

		void SimpleBundleStorage::BundleStore::load(const ibrcommon::File &file)
		{
			ibrcommon::MutexLock l(bundleslock);

			BundleContainer container(file);
			bundles.insert( container );

			// increment the storage size
			_currentsize += container.size();

			// add it to the bundle list
			dtn::data::BundleList::add(dtn::data::MetaBundle(*container));
		}

		void SimpleBundleStorage::BundleStore::store(const dtn::data::Bundle &bundle, SimpleBundleStorage &storage)
		{
			ibrcommon::MutexLock l(bundleslock);

			pair<set<BundleContainer>::iterator,bool> ret;

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
				storage._tasks.push( new SimpleBundleStorage::TaskStoreBundle(container) );
			}

			// insert Container
			ret = bundles.insert( container );

			if (ret.second)
			{
				dtn::data::BundleList::add(dtn::data::MetaBundle(bundle));
			}
			else
			{
				IBRCOMMON_LOGGER_DEBUG(5) << "Storage: got bundle duplicate " << bundle.toString() << IBRCOMMON_LOGGER_ENDL;
			}
		}

		void SimpleBundleStorage::BundleStore::remove(const dtn::data::BundleID &id)
		{
			ibrcommon::MutexLock l(bundleslock);

			for (std::set<BundleContainer>::const_iterator iter = bundles.begin(); iter != bundles.end(); iter++)
			{
				if ( id == (*(*iter)) )
				{
					// remove item in the bundlelist
					BundleContainer container = (*iter);

					// remove it from the bundle list
					dtn::data::BundleList::remove(dtn::data::MetaBundle(*container));

					// decrement the storage size
					_currentsize -= container.size();

					// mark for deletion
					container.remove();

					// remove the container
					bundles.erase(iter);

					return;
				}
			}

			throw BundleStorage::NoBundleFoundException();
		}

		size_t SimpleBundleStorage::BundleStore::size() const
		{
			return _currentsize;
		}

		void SimpleBundleStorage::BundleStore::clear()
		{
			ibrcommon::MutexLock l(bundleslock);

			// mark all bundles for deletion
			for (std::set<BundleContainer>::iterator iter = bundles.begin(); iter != bundles.end(); iter++)
			{
				BundleContainer container = (*iter);

				// mark for deletion
				container.remove();
			}

			bundles.clear();
			dtn::data::BundleList::clear();

			// set the storage size to zero
			_currentsize = 0;
		}

		void SimpleBundleStorage::BundleStore::eventBundleExpired(const ExpiringBundle &b)
		{
			for (std::set<BundleContainer>::const_iterator iter = bundles.begin(); iter != bundles.end(); iter++)
			{
				if ( b.bundle == (*(*iter)) )
				{
					BundleContainer container = (*iter);

					// decrement the storage size
					_currentsize -= container.size();

					container.remove();
					bundles.erase(iter);
					break;
				}
			}

			// raise bundle event
			dtn::core::BundleEvent::raise( b.bundle, dtn::core::BUNDLE_DELETED, dtn::data::StatusReportBlock::LIFETIME_EXPIRED);

			// raise an event
			dtn::core::BundleExpiredEvent::raise( b.bundle );
		}

		const std::list<dtn::data::BundleID> SimpleBundleStorage::BundleStore::getList()
		{
			ibrcommon::MutexLock l(bundleslock);
			std::list<dtn::data::BundleID> ret;

			for (std::set<BundleContainer>::const_iterator iter = bundles.begin(); iter != bundles.end(); iter++)
			{
				const BundleContainer &container = (*iter);
				ret.push_back( dtn::data::BundleID(*container) );
			}

			return ret;
		}

		SimpleBundleStorage::BundleContainer::BundleContainer(const dtn::data::Bundle &b)
		 : _holder( new SimpleBundleStorage::BundleContainer::Holder(b) )
		{
		}

		SimpleBundleStorage::BundleContainer::BundleContainer(const ibrcommon::File &file)
		 : _holder( new SimpleBundleStorage::BundleContainer::Holder(file) )
		{
		}

		SimpleBundleStorage::BundleContainer::BundleContainer(const dtn::data::Bundle &b, const ibrcommon::File &workdir, const size_t size)
		 : _holder( new SimpleBundleStorage::BundleContainer::Holder(b, workdir, size) )
		{
		}

		SimpleBundleStorage::BundleContainer::~BundleContainer()
		{
			if (--_holder->_count == 0) delete _holder;
		}

		size_t SimpleBundleStorage::BundleContainer::size() const
		{
			return _holder->size();
		}

		bool SimpleBundleStorage::BundleContainer::operator<(const SimpleBundleStorage::BundleContainer& other) const
		{
			return (*(*this) < *other);
		}

		bool SimpleBundleStorage::BundleContainer::operator>(const SimpleBundleStorage::BundleContainer& other) const
		{
			return (*(*this) > *other);
		}

		bool SimpleBundleStorage::BundleContainer::operator!=(const SimpleBundleStorage::BundleContainer& other) const
		{
			return !((*this) == other);
		}

		bool SimpleBundleStorage::BundleContainer::operator==(const SimpleBundleStorage::BundleContainer& other) const
		{
			return (*(*this) == *other);
		}

		SimpleBundleStorage::BundleContainer& SimpleBundleStorage::BundleContainer::operator= (const SimpleBundleStorage::BundleContainer &right)
		{
			++right._holder->_count;
			if (--_holder->_count == 0) delete _holder;
			_holder = right._holder;
			return *this;
		}

		SimpleBundleStorage::BundleContainer& SimpleBundleStorage::BundleContainer::operator= (SimpleBundleStorage::BundleContainer &right)
		{
			++right._holder->_count;
			if (--_holder->_count == 0) delete _holder;
			_holder = right._holder;
			return *this;
		}

		SimpleBundleStorage::BundleContainer::BundleContainer(const SimpleBundleStorage::BundleContainer& right)
		 : BundleID(right), _holder(right._holder)
		{
			++_holder->_count;
		}

		dtn::data::Bundle& SimpleBundleStorage::BundleContainer::operator*()
		{
			return _holder->_bundle;
		}

		const dtn::data::Bundle& SimpleBundleStorage::BundleContainer::operator*() const
		{
			return _holder->_bundle;
		}

		void SimpleBundleStorage::BundleContainer::remove()
		{
			_holder->deletion = true;
		}

		void SimpleBundleStorage::BundleContainer::invokeStore()
		{
			_holder->invokeStore();
		}

		SimpleBundleStorage::BundleContainer::Holder::Holder( const dtn::data::Bundle &b )
		 : _bundle(b), _mode(MODE_NONPERSISTENT), _count(1), deletion(false)
		{
			dtn::data::DefaultSerializer s(std::cout);
			_size = s.getLength(_bundle);
		}

		SimpleBundleStorage::BundleContainer::Holder::Holder( const ibrcommon::File &file )
		 : _bundle(), _container(file), _mode(MODE_PERSISTENT), _count(1), deletion(false)
		{
			std::fstream fs(file.getPath().c_str(), ios::in|ios::binary);
			_size = file.size();

			try {
				fs.exceptions(std::ios::badbit | std::ios::failbit | std::ios::eofbit);
				dtn::data::DefaultDeserializer(fs, dtn::core::BundleCore::getInstance()) >> _bundle;
			} catch (ios_base::failure ex) {
				throw dtn::SerializationFailedException("can not load bundle data" + std::string(ex.what()));
			}

			fs.close();
		}

		SimpleBundleStorage::BundleContainer::Holder::Holder( const dtn::data::Bundle &b, const ibrcommon::File &workdir, const size_t size )
		 : _bundle(b), _mode(MODE_PERSISTENT), _count(1), deletion(false), _size(size)
		{
			std::string namestr = workdir.getPath() + "/bundle-XXXXXXXX";
			char name[namestr.length()];
			::strcpy(name, namestr.c_str());

			int fd = mkstemp(name);
			if (fd == -1) throw ibrcommon::IOException("Could not create a temporary name.");
			::close(fd);

			_container = ibrcommon::File(name);
		}

		SimpleBundleStorage::BundleContainer::Holder::~Holder()
		{
			if (_mode == MODE_PERSISTENT && deletion)
			{
				_container.remove();
			}
		}

		size_t SimpleBundleStorage::BundleContainer::Holder::size() const
		{
			return _size;
		}

		void SimpleBundleStorage::BundleContainer::Holder::invokeStore()
		{
			try {
				std::fstream out(_container.getPath().c_str(), ios::in|ios::out|ios::binary|ios::trunc);
				out.exceptions(std::ios::badbit | std::ios::failbit | std::ios::eofbit);
				dtn::data::DefaultSerializer(out) << _bundle; out << std::flush;
				out.close();
				_size = _container.size();
			} catch (ios_base::failure ex) {
				throw dtn::SerializationFailedException("can not write data to the storage; " + std::string(ex.what()));
			}
		}

		SimpleBundleStorage::TaskStoreBundle::TaskStoreBundle(const SimpleBundleStorage::BundleContainer &container)
		 : _container(container)
		{
		}

		SimpleBundleStorage::TaskStoreBundle::~TaskStoreBundle()
		{

		}

		void SimpleBundleStorage::TaskStoreBundle::run()
		{
			_container.invokeStore();
		}
	}
}

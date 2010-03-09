#include "core/SimpleBundleStorage.h"
#include "core/TimeEvent.h"
#include "core/GlobalEvent.h"
#include "core/BundleExpiredEvent.h"

#include "ibrdtn/utils/Utils.h"

#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>

namespace dtn
{
	namespace core
	{
		SimpleBundleStorage::SimpleBundleStorage()
		 : _store(*this), _running(true), _mode(MODE_NONPERSISTENT)
		{
			bindEvent(TimeEvent::className);
			bindEvent(GlobalEvent::className);
		}

		SimpleBundleStorage::SimpleBundleStorage(const ibrcommon::File &workdir)
		 : _store(*this), _running(true), _mode(MODE_PERSISTENT), _workdir(workdir)
		{
			bindEvent(TimeEvent::className);
			bindEvent(GlobalEvent::className);

			// load persistent bundles
			std::list<ibrcommon::File> files;
			_workdir.getFiles(files);

			for (std::list<ibrcommon::File>::iterator iter = files.begin(); iter != files.end(); iter++)
			{
				ibrcommon::File &file = (*iter);
				if (!file.isDirectory())
				{
					std::fstream fs(file.getPath().c_str(), ios::in|ios::binary);
					dtn::data::Bundle b;
					fs >> b;
					fs.close();

					ibrcommon::MutexLock l(_dbchanged);

					// add the file to the index
					_bundlefiles[dtn::data::BundleID(b)] = file;

					// store the bundle into the storage
					_store.store(b);
				}
			}

			// some output
			cout << _store.bundles.size() << " Bundles restored." << endl;
		}

		SimpleBundleStorage::~SimpleBundleStorage()
		{
			unbindEvent(TimeEvent::className);
			unbindEvent(GlobalEvent::className);

			shutdown();
		}

		void SimpleBundleStorage::raiseEvent(const Event *evt)
		{
			const TimeEvent *time = dynamic_cast<const TimeEvent*>(evt);
			const GlobalEvent *global = dynamic_cast<const GlobalEvent*>(evt);

			if (global != NULL)
			{
				if (global->getAction() == dtn::core::GlobalEvent::GLOBAL_SHUTDOWN)
				{
					shutdown();
				}
			}

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
			ibrcommon::MutexLock l(_dbchanged);

			if (_mode = MODE_PERSISTENT)
			{
				// delete all bundles
				for (std::map<dtn::data::BundleID, ibrcommon::File>::iterator iter = _bundlefiles.begin(); iter != _bundlefiles.end(); iter++)
				{
					ibrcommon::File &file = (*iter).second;
					file.remove();
				}

				_bundlefiles.clear();
			}

			// delete all bundles
			_store.clear();
		}

		bool SimpleBundleStorage::empty()
		{
			return _store.bundles.empty();
		}

		unsigned int SimpleBundleStorage::count()
		{
			return _store.bundles.size();
		}

		void SimpleBundleStorage::store(const dtn::data::Bundle &bundle)
		{
			ibrcommon::MutexLock l(_dbchanged);

			if (_mode == MODE_PERSISTENT)
			{
				std::string namestr = _workdir.getPath() + "/bundle-XXXXXXXX";
				char name[namestr.length()];
				::strcpy(name, namestr.c_str());

				int fd = mkstemp(name);
				if (fd == -1) throw ibrcommon::IOException("Could not create a temporary name.");
				::close(fd);

				ibrcommon::File file(name);

				std::fstream out(file.getPath().c_str(), ios::in|ios::out|ios::binary|ios::trunc);

				out << bundle;

				// store the bundle into a file
				_bundlefiles[dtn::data::BundleID(bundle)] = file;
			}

			_store.store(bundle);

#ifdef DO_EXTENDED_DEBUG_OUTPUT
			ibrcommon::slog << ibrcommon::SYSLOG_INFO << "Storage: stored bundle " << bundle.toString() << endl;
#endif
		}

		void SimpleBundleStorage::unblock(const dtn::data::EID &eid)
		{
			ibrcommon::MutexLock l(_dbchanged);
			_unblock_eid = eid;
			_dbchanged.signal(true);
		}

		void SimpleBundleStorage::shutdown()
		{
			ibrcommon::MutexLock l(_dbchanged);
			_running = false;
			_dbchanged.signal(true);
		}


		dtn::data::Bundle SimpleBundleStorage::get(const dtn::data::EID &eid)
		{
			ibrcommon::MutexLock l(_dbchanged);
			ibrcommon::AtomicCounter::Lock alock(_blocker);

#ifdef DO_EXTENDED_DEBUG_OUTPUT
			cout << "Storage: get bundle for " << eid.getString() << endl;
#endif

			while (_running)
			{
				for (std::set<dtn::data::Bundle>::const_iterator iter = _store.bundles.begin(); iter != _store.bundles.end(); iter++)
				{
					const dtn::data::Bundle &bundle = (*iter);
					if (bundle._destination == eid)
					{
						return bundle;
					}
				}

				_dbchanged.wait();

				// step out on shutdown
				if (!_running) break;

				// leave this method if we need to unblock
				if (_unblock_eid == eid)
				{
					_unblock_eid = EID();
					break;
				}
			}

			throw dtn::exceptions::NoBundleFoundException();
		}

		dtn::data::Bundle SimpleBundleStorage::get(const dtn::data::BundleID &id)
		{
			ibrcommon::MutexLock l(_dbchanged);

			for (std::set<dtn::data::Bundle>::const_iterator iter = _store.bundles.begin(); iter != _store.bundles.end(); iter++)
			{
				const Bundle &bundle = (*iter);
				if (id == bundle)
				{
					return bundle;
				}
			}

			throw dtn::exceptions::NoBundleFoundException();
		}

		void SimpleBundleStorage::remove(const dtn::data::BundleID &id)
		{
			ibrcommon::MutexLock l(_dbchanged);
			_store.remove(id);

			if (_mode == MODE_PERSISTENT)
			{
				// remove the file
				_bundlefiles[id].remove();
				_bundlefiles.erase(id);
			}
		}

		SimpleBundleStorage::BundleStore::BundleStore(SimpleBundleStorage &sbs)
		 : _sbs(sbs)
		{
		}

		SimpleBundleStorage::BundleStore::~BundleStore()
		{
		}

		void SimpleBundleStorage::BundleStore::expire(const size_t timestamp)
		{
			dtn::routing::BundleList::expire(timestamp);
		}

		void SimpleBundleStorage::BundleStore::store(const dtn::data::Bundle &bundle)
		{
			dtn::routing::BundleList::add(dtn::routing::MetaBundle(bundle));
			bundles.insert(bundle);
		}

		void SimpleBundleStorage::BundleStore::remove(const dtn::data::BundleID &id)
		{
			for (std::set<dtn::data::Bundle>::const_iterator iter = bundles.begin(); iter != bundles.end(); iter++)
			{
				if ( id == (*iter) )
				{
					// remove item in the bundlelist
					dtn::routing::BundleList::remove(dtn::routing::MetaBundle(*iter));

					bundles.erase(iter);

					return;
				}
			}

			throw dtn::exceptions::NoBundleFoundException();
		}

		void SimpleBundleStorage::BundleStore::clear()
		{
			bundles.clear();
			dtn::routing::BundleList::clear();
		}

		void SimpleBundleStorage::BundleStore::eventBundleExpired(const ExpiringBundle &b)
		{
			for (std::set<dtn::data::Bundle>::const_iterator iter = bundles.begin(); iter != bundles.end(); iter++)
			{
				if ( b.bundle == (*iter) )
				{
					bundles.erase(iter);
					break;
				}
			}

			if (_sbs._mode == MODE_PERSISTENT)
			{
				// remove the file
				_sbs._bundlefiles[b.bundle].remove();
				_sbs._bundlefiles.erase(b.bundle);
			}

			// raise an event
			dtn::core::BundleExpiredEvent::raise( b.bundle );
		}
	}
}

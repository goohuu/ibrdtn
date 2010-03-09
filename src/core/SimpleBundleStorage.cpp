#include "core/SimpleBundleStorage.h"
#include "core/TimeEvent.h"
#include "core/GlobalEvent.h"
#include "core/BundleExpiredEvent.h"

#include "ibrdtn/utils/Utils.h"

#include <iostream>

namespace dtn
{
	namespace core
	{
		SimpleBundleStorage::SimpleBundleStorage()
		 : _running(true)
		{
			bindEvent(TimeEvent::className);
			bindEvent(GlobalEvent::className);
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
		}

		SimpleBundleStorage::BundleStore::BundleStore()
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

			// raise an event
			dtn::core::BundleExpiredEvent::raise( b.bundle );
		}
	}
}

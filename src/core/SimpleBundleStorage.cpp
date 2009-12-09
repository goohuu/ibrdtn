#include "core/SimpleBundleStorage.h"
#include "core/EventSwitch.h"
#include "core/TimeEvent.h"
#include "core/GlobalEvent.h"

#include "ibrdtn/utils/Utils.h"

#include <iostream>

namespace dtn
{
	namespace core
	{
		SimpleBundleStorage::SimpleBundleStorage()
		 : _running(true)
		{
			EventSwitch::registerEventReceiver(TimeEvent::className, this);
			EventSwitch::registerEventReceiver(GlobalEvent::className, this);
		}

		SimpleBundleStorage::~SimpleBundleStorage()
		{
			EventSwitch::unregisterEventReceiver(TimeEvent::className, this);
			EventSwitch::unregisterEventReceiver(GlobalEvent::className, this);

			_running = false;
			_timecond.go();
			ibrcommon::MutexLock l(_dbchanged);
			_dbchanged.signal(true);
			join();
		}

		void SimpleBundleStorage::raiseEvent(const Event *evt)
		{
			const TimeEvent *time = dynamic_cast<const TimeEvent*>(evt);
			const GlobalEvent *global = dynamic_cast<const GlobalEvent*>(evt);

			if (global != NULL)
			{
				if (global->getAction() == dtn::core::GlobalEvent::GLOBAL_SHUTDOWN)
				{
					ibrcommon::MutexLock l(_dbchanged);
					_running = false;
					_timecond.go();
					_dbchanged.signal(true);
				}
			}

			if (time != NULL)
			{
				if (time->getAction() == dtn::core::TIME_SECOND_TICK)
				{
					_timecond.go();
				}
			}
		}

		void SimpleBundleStorage::clear()
		{
			ibrcommon::MutexLock l(_dbchanged);
			// delete all bundles
			_bundles.clear();
			_fragments.clear();
		}

		bool SimpleBundleStorage::empty()
		{
			return (_bundles.empty() && _fragments.empty());
		}

		unsigned int SimpleBundleStorage::count()
		{
			return (_bundles.size() + _fragments.size());
		}

		void SimpleBundleStorage::store(const dtn::data::Bundle &bundle)
		{
			ibrcommon::MutexLock l(_dbchanged);
			_bundles.push_back(bundle);
			_dbchanged.signal(true);

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

		dtn::data::Bundle SimpleBundleStorage::get(const dtn::data::EID &eid)
		{
			ibrcommon::MutexLock l(_dbchanged);

#ifdef DO_EXTENDED_DEBUG_OUTPUT
			cout << "Storage: get bundle for " << eid.getString() << endl;
#endif

			while (_running)
			{
				list<Bundle>::iterator iter = _bundles.begin();

				while (iter != _bundles.end())
				{
					dtn::data::Bundle &bundle = (*iter);
					if (bundle._destination == eid)
					{
						return bundle;
					}

					iter++;
				}

				_dbchanged.wait();

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

			list<Bundle>::iterator iter = _bundles.begin();

			while (iter != _bundles.end())
			{
				Bundle &bundle = (*iter);
				if (id == bundle)
				{
					return bundle;
				}

				iter++;
			}

			throw dtn::exceptions::NoBundleFoundException();
		}

		void SimpleBundleStorage::remove(const dtn::data::BundleID &id)
		{
			ibrcommon::MutexLock l(_dbchanged);

			list<Bundle>::iterator iter = _bundles.begin();

			while (iter != _bundles.end())
			{
				Bundle &bundle = (*iter);
				if (id == bundle)
				{
					_bundles.erase(iter);
					return;
				}

				iter++;
			}

			throw dtn::exceptions::NoBundleFoundException();
		}

		void SimpleBundleStorage::run()
		{
			while (_running)
			{
				list<dtn::data::BundleID> expired;
				list<dtn::data::Bundle>::const_iterator iter = _bundles.begin();

				// seek for expired bundles
				while (iter != _bundles.end())
				{
					const dtn::data::Bundle &bundle = (*iter);

					if ( bundle.isExpired() )
					{
						expired.push_back( BundleID(bundle) );
					}

					iter++;
				}

				// remove the expired bundles
				list<dtn::data::BundleID>::const_iterator e_iter = expired.begin();
				while (e_iter != expired.end())
				{
					remove( *iter );
					ibrcommon::slog << ibrcommon::SYSLOG_INFO << "bundle " << (*iter).toString() << " has been expired and removed" << endl;
					e_iter++;
				}

				yield();
				_timecond.wait();
			}
		}

//		Bundle SimpleBundleStorage::storeFragment(const Bundle &bundle)
//		{
//			// iterate through the list of fragment-lists.
//			list<list<Bundle> >::iterator iter = m_fragments.begin();
//			list<Bundle> fragment_list;
//
//			// search for a existing list for this fragment
//			while (iter != m_fragments.end())
//			{
//				fragment_list = (*iter);
//
//				// list found?
//				if ( bundle == fragment_list.front() )
//				{
//					m_fragments.erase( iter );
//					break;
//				}
//
//				// next list
//				iter++;
//			}
//
//			{	// check for duplicates. do this bundle already exists in the list?
//				list<Bundle>::const_iterator iter = fragment_list.begin();
//				while (iter != fragment_list.end())
//				{
//					if ( (*iter)._fragmentoffset == bundle._fragmentoffset )
//					{
//						// bundle found, discard the current fragment.
//						throw exceptions::DuplicateBundleException();
//					}
//					iter++;
//				}
//			}	// end check for duplicates
//
//			if ( ( bundle._procflags & data::Bundle::CUSTODY_REQUESTED ) && (bundle._custodian != BundleCore::local) )
//			{
//				// here i need a copy
//				Bundle b_copy = bundle;
//
//				// set me as custody
//				b_copy._custodian = BundleCore::local;
//
//				// accept custody
//				EventSwitch::raiseEvent( new CustodyEvent( bundle, CUSTODY_ACCEPT ) );
//
//				// bundle isn't in this list, add it
//				fragment_list.push_back( b_copy );
//			}
//			else
//			{
//				// bundle isn't in this list, add it
//				fragment_list.push_back( bundle );
//			}
//
//			try {
//				// try to merge the fragments
//				return utils::Utils::merge(fragment_list);
//			} catch (exceptions::FragmentationException ex) {
//				// merge not possible, store the list in front of the list of lists.
//				m_fragments.push_front(fragment_list);
//			}
//
//			throw exceptions::MissingObjectException();
//		}
	}
}

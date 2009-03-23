#include "core/SimpleBundleStorage.h"
#include "data/BundleFactory.h"
#include "utils/Utils.h"
#include "core/NodeEvent.h"
#include "core/StorageEvent.h"
#include "core/EventSwitch.h"
#include "core/BundleEvent.h"
#include "core/RouteEvent.h"
#include "core/CustodyEvent.h"
#include "core/TimeEvent.h"

#include <iostream>



namespace dtn
{
	namespace core
	{
		SimpleBundleStorage::SimpleBundleStorage(unsigned int size, unsigned int bundle_maxsize, bool merge)
		 : Service("SimpleBundleStorage"), BundleStorage(), m_nextdeprecation(0), m_last_compress(0), m_size(size),
		 	m_bundle_maxsize(bundle_maxsize), m_currentsize(0), m_nocleanup(false), m_merge(merge)
		{
			// register me for events
			EventSwitch::registerEventReceiver( NodeEvent::className, this );
			EventSwitch::registerEventReceiver( StorageEvent::className, this );
		}

		SimpleBundleStorage::~SimpleBundleStorage()
		{
			EventSwitch::unregisterEventReceiver( NodeEvent::className, this );
			EventSwitch::unregisterEventReceiver( StorageEvent::className, this );
			clear();
		}

		void SimpleBundleStorage::raiseEvent(const Event *evt)
		{
			const NodeEvent *node = dynamic_cast<const NodeEvent*>(evt);
			const StorageEvent *storage = dynamic_cast<const StorageEvent*>(evt);
			const TimeEvent *time = dynamic_cast<const TimeEvent*>(evt);

			if (time != NULL)
			{
				if (time->getAction() == TIME_SECOND_TICK)
				{
					// the time has changed
					m_breakwait.signal();
				}
			}
			else if (storage != NULL)
			{
				switch (storage->getAction())
				{
					case STORE_BUNDLE:
					{
						Bundle *ret = storeFragment(storage->getBundle());
						m_breakwait.signal();

						if (ret != NULL)
						{
							// route the joint bundle
							EventSwitch::raiseEvent( new RouteEvent(*ret, ROUTE_PROCESS_BUNDLE) );
							delete ret;
						}

						break;
					}

					case STORE_SCHEDULE:
						const BundleSchedule &sched = storage->getSchedule();

						try {
							// local eid
							string localeid = BundleCore::getInstance().getLocalURI();
							const Bundle &b = sched.getBundle();

							if ( ( b.getPrimaryFlags().isCustodyRequested() ) && (b.getCustodian() != localeid) )
							{
								// here i need a copy
								Bundle b_copy = b;

								// set me as custody
								b_copy.setCustodian(localeid);

								// Make a new schedule
								BundleSchedule sched_new(b_copy, sched.getTime(), sched.getEID());

								// store the schedule
								store(sched_new);
							}
							else
							{
								// store the schedule
								store(sched);
							}

							// accept custody
							EventSwitch::raiseEvent( new CustodyEvent( sched.getBundle(), CUSTODY_ACCEPT ) );

						} catch (NoSpaceLeftException ex) {
							// reject custody
							EventSwitch::raiseEvent( new CustodyEvent( sched.getBundle(), CUSTODY_REJECT ) );
						}

						m_breakwait.signal();
						break;
				}
			}
			else if (node != NULL)
			{
				const Node &n = node->getNode();

				switch (node->getAction())
				{
					case NODE_AVAILABLE:
						m_neighbours[n.getURI()] = n;
						break;

					case NODE_UNAVAILABLE:
						m_neighbours.erase(n.getURI());
						break;
				}
			}
		}

		void SimpleBundleStorage::store(const BundleSchedule &schedule)
		{
			MutexLock l(m_readlock);

			// check if there is enough space
			unsigned int remain = m_size - m_currentsize;

			// get reference of the bundle in the schedule
			const Bundle &bundle = schedule.getBundle();

			if ( remain < bundle.getLength() )
			{
				// not enough space left
				throw NoSpaceLeftException();
			}
			else
			{
				m_currentsize += bundle.getLength();
			}

			// storage change, clean-up allowed
			m_nocleanup = false;

			// lifetime of the bundle
			unsigned int lifedtn = bundle.getInteger(LIFETIME) + bundle.getInteger(CREATION_TIMESTAMP);

			// check if this bundle expires bevor the next cycle.
			// the clean-up is scheduled 5 seconds after the expiration.
			if ( m_nextdeprecation > (lifedtn + 5) )
			{
				m_nextdeprecation = lifedtn + 5;
			}

			// sorted insertion
			// the bundle which should be sent as next is at the end
			list<BundleSchedule>::iterator iter = m_schedules.begin();
			unsigned int time;

			while (iter != m_schedules.end())
			{
				// read the time of the current item
				time = (*iter).getTime();

				if (time <= schedule.getTime())
				{
					break;
				}

				iter++;
			}

			m_schedules.insert(iter, schedule);
		}

		void SimpleBundleStorage::clear()
		{
			MutexLock l(m_readlock);

			m_currentsize = 0;

			// delete all fragments
			m_schedules.clear();
			m_fragments.clear();
		}

		bool SimpleBundleStorage::isEmpty()
		{
			MutexLock l(m_readlock);
			return m_schedules.empty();
		}

		unsigned int SimpleBundleStorage::getCount()
		{
			MutexLock l(m_readlock);
			return m_schedules.size();
		}

		void SimpleBundleStorage::tick()
		{
			if (m_neighbours.size() != 0)
			{
				// get the first neighbour
				map<string,Node>::iterator iter = m_neighbours.begin();
				Node &node = (*iter).second;

				try {
					BundleSchedule schedule = getSchedule( node.getURI() );
					EventSwitch::raiseEvent( new RouteEvent( schedule.getBundle(), ROUTE_PROCESS_BUNDLE ) );
				} catch (exceptions::NoScheduleFoundException ex) {
					// remove the neighbour
					m_neighbours.erase(node.getURI());
				}

				// get current time
				unsigned int dtntime = BundleFactory::getDTNTime();

				// send timed schedules
				try {
					BundleSchedule schedule = getSchedule( dtntime );
					EventSwitch::raiseEvent( new RouteEvent( schedule.getBundle(), ROUTE_PROCESS_BUNDLE ) );
				} catch (exceptions::NoScheduleFoundException ex) {

				}

				return;
			}

			// TODO: deleteDeprecated();

			// wait till the dtntime has changed or a new bundles is stored (m_bundlewaiting)
			m_breakwait.wait();
		}

		void SimpleBundleStorage::terminate()
		{
			m_breakwait.signal();
		}

		BundleSchedule SimpleBundleStorage::getSchedule(string destination)
		{
			MutexLock l(m_readlock);

			list<BundleSchedule>::iterator iter = m_schedules.begin();
			bool ret = false;

			// search for matching bundles
			while (iter != m_schedules.end())
			{
				if ( (*iter).getEID() == destination )
				{
					ret = true;
				}
				else
				{
					const Bundle &b = (*iter).getBundle();

					// check if the destination of the bundle match
					if (b.getDestination().find( destination, 0 ) == 0)
					{
						ret = true;
					}
				}

				if (ret)
				{
					// bundle found
					BundleSchedule schedule = (*iter);

					// remove it from the list
					m_schedules.erase(iter);

					// shrink the size
					m_currentsize -= schedule.getBundle().getLength();

					// return
					return schedule;
				}

				iter++;
			}

			throw exceptions::NoScheduleFoundException("No schedule for this destination available");
		}

		BundleSchedule SimpleBundleStorage::getSchedule(unsigned int dtntime)
		{
			if ( isEmpty() )
			{
				throw exceptions::NoScheduleFoundException("No schedule available");
			}

			MutexLock l(m_readlock);

			BundleSchedule ret = m_schedules.back();

			if (ret.getTime() > dtntime)
			{
				throw exceptions::NoScheduleFoundException("No schedule for this time available");
			}

			// shrink the size of the storage
			m_currentsize -= ret.getBundle().getLength();

			// remove the last element
			m_schedules.pop_back();

			return ret;
		}

		void SimpleBundleStorage::deleteDeprecated()
		{
			// TODO: search for deprecated bundles in m_fragments

			unsigned int currenttime = BundleFactory::getDTNTime();

			// only run if we expect a outdated bundle
			if ( m_nextdeprecation > currenttime ) return;

			MutexLock l(m_readlock);

			// set the next cycle to +1h
			// this value could be lowered in the following algorithm
			m_nextdeprecation = currenttime + 3600;

			list<BundleSchedule>::iterator iter = m_schedules.begin();

			// search for expired bundles
			while (iter != m_schedules.end())
			{
				const Bundle &b = (*iter).getBundle();

				// check if the bundle is expired
				if (b.isExpired())
				{
					// shrink the size of the database
					m_currentsize -= b.getLength();

					// announce bundle deleted event
					EventSwitch::raiseEvent( new BundleEvent(b, BUNDLE_DELETED) );

					list<BundleSchedule>::iterator iter2 = iter;

					// next bundle
					iter++;

					// remove the bundle
					m_schedules.erase(iter2);

					continue;
				}
				else
				{
					// check if the bundle expiring bevor the next scheduled cycle
					unsigned int lifedtn = b.getInteger(LIFETIME) + b.getInteger(CREATION_TIMESTAMP);

					if ( m_nextdeprecation > (lifedtn + 5) )
					{
						m_nextdeprecation = lifedtn + 5;
					}
				}

				iter++;
			}
		}

		Bundle* SimpleBundleStorage::storeFragment(const Bundle &bundle)
		{
			MutexLock l(m_fragmentslock);

			// iterate through the list of fragment-lists.
			list<list<Bundle> >::iterator iter = m_fragments.begin();
			list<Bundle> fragment_list;

			// search for a existing list for this fragment
			while (iter != m_fragments.end())
			{
				fragment_list = (*iter);

				// list found?
				if ( bundle == fragment_list.front() )
				{
					m_fragments.erase( iter );
					break;
				}

				// next list
				iter++;
			}

			{	// check for duplicates. do this bundle already exists in the list?
				list<Bundle>::const_iterator iter = fragment_list.begin();
				while (iter != fragment_list.end())
				{
					if ( (*iter).getInteger(FRAGMENTATION_OFFSET) == bundle.getInteger(FRAGMENTATION_OFFSET) )
					{
						// bundle found, discard the current fragment.
						return NULL;
					}
					iter++;
				}
			}	// end check for duplicates

			// local eid
			string localeid = BundleCore::getInstance().getLocalURI();

			if ( ( bundle.getPrimaryFlags().isCustodyRequested() ) && (bundle.getCustodian() != localeid) )
			{
				// here i need a copy
				Bundle b_copy = bundle;

				// set me as custody
				b_copy.setCustodian(localeid);

				// accept custody
				EventSwitch::raiseEvent( new CustodyEvent( bundle, CUSTODY_ACCEPT ) );

				// bundle isn't in this list, add it
				fragment_list.push_back( b_copy );
			}
			else
			{
				// bundle isn't in this list, add it
				fragment_list.push_back( bundle );
			}

			try {
				// try to merge the fragments
				Bundle *ret = BundleFactory::merge(fragment_list);

				return ret;
			} catch (dtn::exceptions::FragmentationException ex) {
				// merge not possible, store the list in front of the list of lists.
				m_fragments.push_front(fragment_list);
			}

			return NULL;
		}
	}
}

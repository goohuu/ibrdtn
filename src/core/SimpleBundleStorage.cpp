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

						// store the bundle
						store(sched);

						// accept custody
						if (sched.getBundle().getPrimaryFlags().isCustodyRequested())
						{
							EventSwitch::raiseEvent( new CustodyEvent( sched.getBundle(), CUSTODY_ACCEPT ) );
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

			// Prüfen ob genug Platz vorhanden ist
			unsigned int remain = m_size - m_currentsize;

			// Hole Referenz auf das Bundle im Schedule
			const Bundle &bundle = schedule.getBundle();

			if ( remain < bundle.getLength() )
			{
				// Nicht genug Platz vorhanden
				throw NoSpaceLeftException();
			}
			else
			{
				m_currentsize += bundle.getLength();
			}

			// Storage ändert sich, aufräumen wieder erlaubt
			m_nocleanup = false;

			// Prüfe ob dieses Bündel vor dem nächsten geplanten Durchlauf abläuft
			// Wobei wir planen immer 5 Sekunden nach Ablauf des nächsten Bündel zu reagieren
			// um eventuell mehrere Bündel gleichzeitig zu entsorgen.
			unsigned int lifedtn = bundle.getInteger(LIFETIME) + bundle.getInteger(CREATION_TIMESTAMP);

			if ( m_nextdeprecation > (lifedtn + 5) )
			{
				m_nextdeprecation = lifedtn + 5;
			}

			// Schedules sortiert einfügen
			// Das als nächstes zu versendende Bundle ist immer unten
			list<BundleSchedule>::iterator iter = m_schedules.begin();
			unsigned int time;

			while (iter != m_schedules.end())
			{
				// Zeit des aktuellen Items lesen
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

			// Suche nach passenden Bundles
			while (iter != m_schedules.end())
			{
				if ( (*iter).getEID() == destination )
				{
					ret = true;
				}
				else
				{
					const Bundle &b = (*iter).getBundle();

					// Prüfe ob der Schedule für die Destination ist.
					if (b.getDestination().find( destination, 0 ) == 0)
					{
						ret = true;
					}
				}

				if (ret)
				{
					// Bundle gefunden
					BundleSchedule schedule = (*iter);

					// Entfernen
					m_schedules.erase(iter);

					// Gesamtgröße verkleinern
					m_currentsize -= schedule.getBundle().getLength();

					// Zurückgeben
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

			// Gesamtgröße verkleinern
			m_currentsize -= ret.getBundle().getLength();

			// Letzte Element löschen
			m_schedules.pop_back();

			return ret;
		}

		list<list<BundleSchedule>::iterator> SimpleBundleStorage::searchEquals(unsigned int maxsize, list<BundleSchedule>::iterator start, list<BundleSchedule>::iterator end)
		{
			MutexLock l(m_readlock);

			list<BundleSchedule>::iterator iter = start;
			list<list<BundleSchedule>::iterator> marked;
			Bundle *bundle = NULL;

			unsigned int cursize = 0;

			while (iter != end)
			{
				BundleSchedule schedule = (*iter);

				// Erstes Kriterium: Keine StatusReports
				if ((*iter).getBundle().getLength() < maxsize)
				if ((*iter).getBundle().getReportTo() == "dtn:none")
				{
					if ( bundle == NULL )
					{
						// Dieses Bündel merken wir uns legen es in die marked-Liste
						marked.push_back( iter );
						const Bundle &bundle = schedule.getBundle();
						cursize += bundle.getLength();
					}
					else
					{
						const Bundle &tmp = (*iter).getBundle();

						// Vergleiche das zuvor gefundene Bündel mit dem aktuellen
						if (*bundle == tmp)
						{
							// Bündel sind identisch genug, dieses nehmen wir mit
							marked.push_back( iter );
							cursize += bundle->getLength();
						}
					}
				}

				iter++;
			}

			return marked;
		}

		/*
		 * Löscht Bundles um Platz zu schaffen
		 */
		void SimpleBundleStorage::deleteDeprecated()
		{
			// TODO: search for deprecated bundles in m_fragments

			unsigned int currenttime = BundleFactory::getDTNTime();

			// Frühestens durchführen wenn ein veraltetes Bundle existiert
			if ( m_nextdeprecation > currenttime ) return;

			MutexLock l(m_readlock);

			// Setzt den nächsten Durchlauf auf spätestens +1 Std.
			// Dieser Wert wird im folgenden Algorithmus möglicherweise verringert
			m_nextdeprecation = currenttime + 3600;

			list<BundleSchedule>::iterator iter = m_schedules.begin();

			// Suche nach veralteten Bundles
			while (iter != m_schedules.end())
			{
				const Bundle &b = (*iter).getBundle();

				unsigned int lifedtn = b.getInteger(LIFETIME) + b.getInteger(CREATION_TIMESTAMP);

				// Prüfe ob die Lebenszeit für das Paket abgelaufen ist
				if (lifedtn < currenttime)
				{
					// Gesamtgröße verkleinern
					m_currentsize -= b.getLength();

					// announce bundle deleted event
					EventSwitch::raiseEvent( new BundleEvent(b, BUNDLE_DELETED) );

					list<BundleSchedule>::iterator iter2 = iter;

					iter++;

					// Zeit ist abgelaufen -> Löschen
					m_schedules.erase(iter2);

					continue;
				}
				else
				{
					// Prüfe ob dieses Bündel vor dem nächsten geplanten Durchlauf abläuft
					// Wobei wir planen immer 5 Sekunden nach Ablauf des nächsten Bündel zu reagieren
					// um eventuell mehrere Bündel gleichzeitig zu entsorgen.
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

			// bundle isn't in this list, add it
			fragment_list.push_back( bundle );

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

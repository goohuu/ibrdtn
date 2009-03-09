#include "core/SimpleBundleStorage.h"
#include "data/BundleFactory.h"
#include "utils/Utils.h"
#include "core/NodeEvent.h"
#include "core/StorageEvent.h"
#include "core/EventSwitch.h"
#include "core/BundleEvent.h"
#include "core/RouteEvent.h"

#include <iostream>



namespace dtn
{
	namespace core
	{
		SimpleBundleStorage::SimpleBundleStorage(BundleCore &core, unsigned int size, unsigned int bundle_maxsize, bool merge)
		 : Service("SimpleBundleStorage"), BundleStorage(), m_core(core), m_nextdeprecation(0), m_last_compress(0), m_size(size),
		 	m_bundle_maxsize(bundle_maxsize), m_currentsize(0), m_nocleanup(false), m_merge(merge), m_bundlewaiting(false)
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

			if (storage != NULL)
			{
				switch (storage->getAction())
				{
					case STORE_BUNDLE:
					{
						Bundle *ret = storeFragment(storage->getBundle());
						m_bundlewaiting = true;

						if (ret != NULL)
						{
							// route the joint bundle
							EventSwitch::raiseEvent( new RouteEvent(ret, ROUTE_FIND_SCHEDULE) );
						}

						break;
					}

					case STORE_SCHEDULE:
						store(storage->getSchedule());
						m_bundlewaiting = true;
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
			Bundle *bundle = schedule.getBundle();

			if ( remain < bundle->getLength() )
			{
				// Nicht genug Platz vorhanden
				throw NoSpaceLeftException();
			}
			else
			{
				m_currentsize += bundle->getLength();
			}

			// Storage ändert sich, aufräumen wieder erlaubt
			m_nocleanup = false;

			// Prüfe ob dieses Bündel vor dem nächsten geplanten Durchlauf abläuft
			// Wobei wir planen immer 5 Sekunden nach Ablauf des nächsten Bündel zu reagieren
			// um eventuell mehrere Bündel gleichzeitig zu entsorgen.
			unsigned int lifedtn = bundle->getInteger(LIFETIME) + bundle->getInteger(CREATION_TIMESTAMP);

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

			// Lösche alle Bundles
			list<BundleSchedule>::iterator iter = m_schedules.begin();

			while (iter != m_schedules.end())
			{
				delete (*iter).getBundle();
				iter++;
			}

			// delete all fragments
			list<list<Bundle*> >::iterator list_iter = m_fragments.begin();

			while (list_iter != m_fragments.end())
			{
				list<Bundle*>::iterator iter = (*list_iter).begin();

				while (iter != (*list_iter).end())
				{
					delete (*iter);
					iter++;
				}

				list_iter++;
			}
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
			// Aktuelle DTN Zeit holen
			unsigned int dtntime = BundleFactory::getDTNTime();

			if (m_neighbours.size() != 0)
			{
				// get the first neighbour
				Node &node = m_neighbours[0];

				try {
					BundleSchedule schedule = getSchedule( node.getURI() );
					EventSwitch::raiseEvent( new RouteEvent( schedule, node ) );
				} catch (exceptions::NoScheduleFoundException ex) {
					// remove the neighbour
					m_neighbours.erase(node.getURI());
				}

				// send timed schedules
				try {
					BundleSchedule schedule = getSchedule( dtntime );
					EventSwitch::raiseEvent( new RouteEvent( schedule ) );
				} catch (exceptions::NoScheduleFoundException ex) {

				}

				return;
			}

			// TODO: deleteDeprecated();

			// wait till the dtntime has changed or a new bundles is stored (m_bundlewaiting)
			while (true)
			{
				if ( (dtntime != BundleFactory::getDTNTime()) || m_bundlewaiting )
				{
					break;
				}
				usleep(1000);
			}

			// we react on signal bundlewaiting, set it to false
			m_bundlewaiting = false;
		}

		BundleSchedule SimpleBundleStorage::getSchedule(string destination)
		{
			MutexLock l(m_readlock);

			list<BundleSchedule>::iterator iter = m_schedules.begin();
			Bundle *b;
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
					b = (*iter).getBundle();

					// Prüfe ob der Schedule für die Destination ist.
					if (b->getDestination().find( destination, 0 ) == 0)
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
					m_currentsize -= schedule.getBundle()->getLength();

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
			m_currentsize -= ret.getBundle()->getLength();

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
				if ((*iter).getBundle()->getLength() < maxsize)
				if ((*iter).getBundle()->getReportTo() == "dtn:none")
				{
					if ( bundle == NULL )
					{
						// Dieses Bündel merken wir uns legen es in die marked-Liste
						marked.push_back( iter );
						bundle = schedule.getBundle();
						cursize += bundle->getLength();
					}
					else
					{
						Bundle *tmp = (*iter).getBundle();
						// Vergleiche das zuvor gefundene Bündel mit dem aktuellen
						if ( (cursize + tmp->getLength()) <= maxsize )
						if ( bundle->getSource() == tmp->getSource() )
						if ( bundle->getDestination() == tmp->getDestination() )
						if ( bundle->getCustodian() == tmp->getCustodian() )
						if ( bundle->getInteger(LIFETIME) == tmp->getInteger(LIFETIME) )
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
			Bundle *b;

			// Suche nach veralteten Bundles
			while (iter != m_schedules.end())
			{
				b = (*iter).getBundle();

				unsigned int lifedtn = b->getInteger(LIFETIME) + b->getInteger(CREATION_TIMESTAMP);

				// Prüfe ob die Lebenszeit für das Paket abgelaufen ist
				if (lifedtn < currenttime)
				{
					// Gesamtgröße verkleinern
					m_currentsize -= b->getLength();

					// announce bundle deleted event
					EventSwitch::raiseEvent( new BundleEvent(*b, BUNDLE_DELETED) );

					// delete the bundle
					delete b;

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

		Bundle* SimpleBundleStorage::storeFragment(const Bundle *bundle)
		{
			MutexLock l(m_fragmentslock);

			// iterate through the list of fragment-lists.
			list<list<Bundle*> >::iterator iter = m_fragments.begin();
			list<Bundle*> fragment_list;

			// search for a existing list for this fragment
			while (iter != m_fragments.end())
			{
				fragment_list = (*iter);

				// list found?
				if ( *bundle == (*(fragment_list.front())) )
				{
					m_fragments.erase( iter );
					break;
				}

				// next list
				iter++;
			}

			{	// check for duplicates. do this bundle already exists in the list?
				list<Bundle*>::const_iterator iter = fragment_list.begin();
				while (iter != fragment_list.end())
				{
					if ( (*iter)->getInteger(FRAGMENTATION_OFFSET) == bundle->getInteger(FRAGMENTATION_OFFSET) )
					{
						// bundle found, discard the current fragment.
						return NULL;
					}
					iter++;
				}
			}	// end check for duplicates

			// bundle isn't in this list, add it
			fragment_list.push_back( new Bundle(*bundle) );

			try {
				// try to merge the fragments
				Bundle *ret = BundleFactory::merge(fragment_list);

				// delete the bundle fragments
				list<Bundle*>::iterator iter = fragment_list.begin();

				while (iter != fragment_list.end())
				{
					delete (*iter);
					iter++;
				}

				return ret;
			} catch (dtn::exceptions::FragmentationException ex) {
				// merge not possible, store the list in front of the list of lists.
				m_fragments.push_front(fragment_list);
			}

			return NULL;
		}
	}
}

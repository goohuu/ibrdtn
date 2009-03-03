#include "core/SimpleBundleStorage.h"
#include "data/BundleFactory.h"
#include "utils/Utils.h"

#include <iostream>



namespace dtn
{
	namespace core
	{
		SimpleBundleStorage::SimpleBundleStorage(unsigned int size, unsigned int bundle_maxsize, bool merge)
		 : Service("SimpleBundleStorage"), BundleStorage(), m_nextdeprecation(0), m_nextcustodytimer(0), m_last_compress(0), m_size(size),
		 	m_bundle_maxsize(bundle_maxsize), m_currentsize(0), m_nocleanup(false), m_custodycb(NULL), m_merge(merge)
		{
		}

		SimpleBundleStorage::~SimpleBundleStorage()
		{
			MutexLock l(m_readlock);

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

		void SimpleBundleStorage::store(BundleSchedule schedule)
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

			m_schedules.clear();
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
			deleteDeprecated();
			checkCustodyTimer();

			usleep(5000);
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

					// TODO: Wenn StatusReport bei Löschung angefordert sind müsste
					// hier eigentlich ein solcher Report erstellt werden.
					// Callback an das Bündelprotokoll über abgelaufenes Bündel

					// Bundle löschen
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

		Bundle* SimpleBundleStorage::storeFragment(Bundle *bundle)
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
						delete bundle;
						return NULL;
					}
					iter++;
				}
			}	// end check for duplicates

			// bundle isn't in this list, add it
			fragment_list.push_back( bundle );

			try {
				// try to merge the fragments
				return BundleFactory::merge(fragment_list);
			} catch (dtn::exceptions::FragmentationException ex) {
				// merge not possible, store the list in front of the list of lists.
				m_fragments.push_front(fragment_list);
			}

			return NULL;
		}

		bool SimpleBundleStorage::timerAvailable()
		{
			MutexLock l(m_custodylock);

			if (m_custodytimer.size() < 200)
			{
				return true;
			}
			else
			{
				return false;
			}
		}

		void SimpleBundleStorage::setTimer(Node node, Bundle *bundle, unsigned int time, unsigned int attempt)
		{
			MutexLock l(m_custodylock);

			// Erstelle einen Timer
			CustodyTimer timer(node, bundle, BundleFactory::getDTNTime() + time, attempt);

			// Sortiert einfügen: Der als nächstes ablaufende Timer ist immer am Ende
			list<CustodyTimer>::iterator iter = m_custodytimer.begin();

			while (iter != m_custodytimer.end())
			{
				if ((*iter).getTime() <= timer.getTime())
				{
					break;
				}

				iter++;
			}

			m_custodytimer.insert(iter, timer);

			// Die Zeit zur nächsten Custody Kontrolle anpassen
			if ( m_nextcustodytimer > timer.getTime() )
			{
				m_nextcustodytimer = timer.getTime();
			}
		}

		Bundle* SimpleBundleStorage::removeTimer(string source, CustodySignalBlock *block)
		{
			MutexLock l(m_custodylock);

			// search for the timer match the signal
			list<CustodyTimer>::iterator iter = m_custodytimer.begin();

			while (iter != m_custodytimer.end())
			{
				Bundle *bundle = (*iter).getBundle();
				Node node = (*iter).getCustodyNode();

				if ( (node.getURI() == source) && block->match(bundle) )
				{
					// ... and remove it
					m_custodytimer.erase( iter );
					return bundle;
				}

				iter++;
			}

			return NULL;
		}

		void SimpleBundleStorage::setCallbackClass(CustodyManagerCallback *callback)
		{
			MutexLock l(m_custodylock);

			m_custodycb = callback;
		}

		void SimpleBundleStorage::checkCustodyTimer()
		{
			unsigned int currenttime = BundleFactory::getDTNTime();

			// wait till a timeout of a custody timer
			if ( m_nextcustodytimer > currenttime ) return;

			// wait a hour for a new check (or till a new timer is created)
			m_nextcustodytimer = currenttime + 3600;

			MutexLock l(m_custodylock);

			list<CustodyTimer> totrigger;

			while (!m_custodytimer.empty())
			{
				// the list is sorted, so only watch on the last item
				CustodyTimer &timer = m_custodytimer.back();

				if (timer.getTime() > BundleFactory::getDTNTime())
				{
					m_nextcustodytimer = timer.getTime();
					break;
				}

				// Timeout! Do a callback...
				totrigger.push_back(timer);

				// delete the timer
				m_custodytimer.pop_back();
			}

			m_custodylock.leaveMutex();

			list<CustodyTimer>::iterator iter = totrigger.begin();
			while (iter != totrigger.end())
			{
				m_custodycb->triggerCustodyTimeout( (*iter) );
				iter++;
			}
		}
	}
}

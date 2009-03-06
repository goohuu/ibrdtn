/*
 * CustodyManager.cpp
 *
 *  Created on: 06.03.2009
 *      Author: morgenro
 */

#include "core/CustodyManager.h"
#include "data/Bundle.h"
#include "utils/MutexLock.h"
#include "core/CustodyTimer.h"
#include "data/BundleFactory.h"

#include "core/EventSwitch.h"
#include "core/CustodyEvent.h"
#include "core/RouteEvent.h"

using namespace dtn::data;
using namespace dtn::utils;

namespace dtn
{
	namespace core
	{
		CustodyManager::CustodyManager() : Service("CustodyManager"), m_nextcustodytimer(0)
		{
			// register me for events
			EventSwitch::registerEventReceiver( CustodyEvent::className, this );
		}

		CustodyManager::~CustodyManager()
		{}

		void CustodyManager::raiseEvent(const Event *evt)
		{
			const CustodyEvent *custodyevent = dynamic_cast<const CustodyEvent*>(evt);

			if (custodyevent != NULL)
			{
				switch (custodyevent->getAction())
				{
					case CUSTODY_ACCEPTANCE:
					{
						// create a timer, if custody is requested
						// TODO: create a timer

						break;
					}

					case CUSTODY_REMOVE_TIMER:
					{
						const CustodySignalBlock *signal = custodyevent->getCustodySignal();

						// remove a timer
						Bundle *bundle = removeTimer(custodyevent->getEID(), *signal);

						if ( signal->isAccepted() )
						{
							delete bundle;
						}
						else
						{
							// custody was rejected - find a new route
							EventSwitch::raiseEvent( new RouteEvent( bundle, ROUTE_FIND_SCHEDULE ) );
						}

						break;
					}
				}
			}
		}

		void CustodyManager::tick()
		{
			checkCustodyTimer();
			usleep(5000);
		}

		bool CustodyManager::timerAvailable()
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

		void CustodyManager::setTimer(Node node, Bundle *bundle, unsigned int time, unsigned int attempt)
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

		Bundle* CustodyManager::removeTimer(string source, const CustodySignalBlock &block)
		{
			MutexLock l(m_custodylock);

			// search for the timer match the signal
			list<CustodyTimer>::iterator iter = m_custodytimer.begin();

			while (iter != m_custodytimer.end())
			{
				Bundle *bundle = (*iter).getBundle();
				Node node = (*iter).getCustodyNode();

				if ( (node.getURI() == source) && block.match(*bundle) )
				{
					// ... and remove it
					m_custodytimer.erase( iter );
					return bundle;
				}

				iter++;
			}

			return NULL;
		}

		void CustodyManager::checkCustodyTimer()
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
				EventSwitch::raiseEvent( new CustodyEvent(&(*iter)) );
				iter++;
			}
		}
	}
}

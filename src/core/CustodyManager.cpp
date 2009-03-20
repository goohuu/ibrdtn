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
#include "data/PayloadBlockFactory.h"

#include "core/TimeEvent.h"
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
			EventSwitch::registerEventReceiver( CustodyEvent::className, this );
			EventSwitch::registerEventReceiver( TimeEvent::className, this );
		}

		CustodyManager::~CustodyManager()
		{
			EventSwitch::unregisterEventReceiver( CustodyEvent::className, this );
			EventSwitch::unregisterEventReceiver( TimeEvent::className, this );
		}

		void CustodyManager::raiseEvent(const Event *evt)
		{
			const CustodyEvent *custody = dynamic_cast<const CustodyEvent*>(evt);
			const TimeEvent *time = dynamic_cast<const TimeEvent*>(evt);

			if (time != NULL)
			{
				if (time->getAction() == TIME_SECOND_TICK)
				{
					// the time has changed
					m_breakwait.signal();
				}
			}
			else if (custody == NULL)
			{
				switch (evt->getType())
				{
				case CUSTODY_ACCEPT:
					acceptCustody(custody->getBundle());
					break;
				case CUSTODY_REJECT:
					rejectCustody(custody->getBundle());
					break;
				}
			}
		}

		void CustodyManager::acceptCustody(const Bundle &bundle)
		{
			if (bundle.getPrimaryFlags().isCustodyRequested())
			{
				// send a custody signal with accept flag
				CustodySignalBlock *signal = PayloadBlockFactory::newCustodySignalBlock(true);
				signal->setMatch(bundle);

				Bundle *b = BundleFactory::getInstance().newBundle();
				b->appendBlock(signal);

				// raise the custody accepted event
				EventSwitch::raiseEvent(new RouteEvent(*b, ROUTE_PROCESS_BUNDLE));
				delete b;
			}
		}

		void CustodyManager::rejectCustody(const Bundle &bundle)
		{
			if (bundle.getPrimaryFlags().isCustodyRequested())
			{
				// send a custody signal with reject flag
				CustodySignalBlock *signal = PayloadBlockFactory::newCustodySignalBlock(false);
				signal->setMatch(bundle);

				Bundle *b = BundleFactory::getInstance().newBundle();
				b->appendBlock(signal);

				// raise the custody accepted event
				EventSwitch::raiseEvent(new RouteEvent(*b, ROUTE_PROCESS_BUNDLE));
				delete b;
			}
		}

		void CustodyManager::tick()
		{
			checkCustodyTimer();
			m_breakwait.wait();
		}

		void CustodyManager::terminate()
		{
			m_breakwait.signal();
		}

		void CustodyManager::setTimer(const Bundle &bundle, unsigned int time, unsigned int attempt)
		{
			MutexLock l(m_custodylock);

			// Erstelle einen Timer
			CustodyTimer timer(bundle, BundleFactory::getDTNTime() + time, attempt);

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

		const Bundle& CustodyManager::removeTimer(const CustodySignalBlock &block)
		{
			MutexLock l(m_custodylock);

			// search for the timer match the signal
			list<CustodyTimer>::iterator iter = m_custodytimer.begin();

			while (iter != m_custodytimer.end())
			{
				const Bundle &bundle = (*iter).getBundle();

				if ( block.match(bundle) )
				{
					// ... and remove it
					m_custodytimer.erase( iter );
					return bundle;
				}

				iter++;
			}

			throw NoTimerFoundException();
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
				retransmitBundle( (*iter).getBundle() );
				iter++;
			}
		}

		void CustodyManager::retransmitBundle(const Bundle &bundle)
		{
			// retransmit the bundle
			EventSwitch::raiseEvent(new RouteEvent(bundle, ROUTE_PROCESS_BUNDLE));
		}
	}
}

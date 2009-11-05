/*
 * CustodyManager.cpp
 *
 *  Created on: 06.03.2009
 *      Author: morgenro
 */

#include "ibrdtn/data/BLOBManager.h"
#include "core/CustodyManager.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/utils/Utils.h"
#include "core/CustodyTimer.h"
#include "core/BundleCore.h"

#include "core/TimeEvent.h"
#include "core/EventSwitch.h"
#include "core/CustodyEvent.h"
#include "core/RouteEvent.h"

using namespace dtn::blob;
using namespace dtn::data;
using namespace dtn::utils;

namespace dtn
{
	namespace core
	{
		CustodyManager::CustodyManager() : m_nextcustodytimer(0), _running(true)
		{
			EventSwitch::registerEventReceiver( CustodyEvent::className, this );
			EventSwitch::registerEventReceiver( TimeEvent::className, this );
		}

		CustodyManager::~CustodyManager()
		{
			_running = false;
			m_breakwait.go();
			join();

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
					m_breakwait.go();
				}
			}
			else if (custody != NULL)
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
			if (bundle._custodian == EID()) return;

			if (bundle._procflags & Bundle::CUSTODY_REQUESTED)
			{
				// send a custody signal with accept flag
				CustodySignalBlock *signal = new CustodySignalBlock();

				// set the bundle to match
				signal->setMatch(bundle);

				// set accepted
				if (!(signal->_status & 1)) signal->_status += 1;

				// create a new bundle
				Bundle b;
				b.addBlock(signal);

				b._destination = bundle._custodian;
				b._source = BundleCore::local;

				// raise the custody accepted event
				EventSwitch::raiseEvent(new RouteEvent(b, ROUTE_PROCESS_BUNDLE));
			}
		}

		void CustodyManager::rejectCustody(const Bundle &bundle)
		{
			if (bundle._custodian == EID()) return;

			if (bundle._procflags & Bundle::CUSTODY_REQUESTED)
			{
				// send a custody signal with accept flag
				CustodySignalBlock *signal = new CustodySignalBlock();

				// set the bundle to match
				signal->setMatch(bundle);

				// create a new bundle
				Bundle b;
				b.addBlock(signal);

				b._destination = bundle._custodian;
				b._source = BundleCore::local;

				// raise the custody accepted event
				EventSwitch::raiseEvent(new RouteEvent(b, ROUTE_PROCESS_BUNDLE));
			}
		}

		void CustodyManager::run()
		{
			while (_running)
			{
				checkCustodyTimer();
				m_breakwait.wait();
				yield();
			}
		}

		void CustodyManager::setTimer(const Bundle &bundle, unsigned int time, unsigned int attempt)
		{
			dtn::utils::MutexLock l(m_custodylock);

			// create a new timer
			CustodyTimer timer(bundle, Utils::get_current_dtn_time() + time, attempt);

			// sorted insert: the next expiring timer is at the end
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

			// modify the next time to check
			if ( m_nextcustodytimer > timer.getTime() )
			{
				m_nextcustodytimer = timer.getTime();
			}
		}

		const Bundle CustodyManager::removeTimer(const CustodySignalBlock &block)
		{
			dtn::utils::MutexLock l(m_custodylock);

			// search for the timer match the signal
			list<CustodyTimer>::iterator iter = m_custodytimer.begin();

			while (iter != m_custodytimer.end())
			{
				const Bundle &bundle = (*iter).getBundle();

				if ( block.match(bundle) )
				{
					Bundle b_copy = (*iter).getBundle();
					// ... and remove it
					m_custodytimer.erase( iter );
					return b_copy;
				}

				iter++;
			}

			throw NoTimerFoundException();
		}

		void CustodyManager::checkCustodyTimer()
		{
			unsigned int currenttime = Utils::get_current_dtn_time();

			// wait till a timeout of a custody timer
			if ( m_nextcustodytimer > currenttime ) return;

			// wait a hour for a new check (or till a new timer is created)
			m_nextcustodytimer = currenttime + 3600;

			m_custodylock.enter();

			list<CustodyTimer> totrigger;

			while (!m_custodytimer.empty())
			{
				// the list is sorted, so only watch on the last item
				CustodyTimer &timer = m_custodytimer.back();

				if (timer.getTime() > Utils::get_current_dtn_time())
				{
					m_nextcustodytimer = timer.getTime();
					break;
				}

				// Timeout! Do a callback...
				totrigger.push_back(timer);

				// delete the timer
				m_custodytimer.pop_back();
			}

			m_custodylock.leave();

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

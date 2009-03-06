/*
 * CustodyEvent.cpp
 *
 *  Created on: 06.03.2009
 *      Author: morgenro
 */

#include "core/CustodyEvent.h"

using namespace dtn::core;
using namespace std;

namespace dtn
{
	namespace core
	{
		CustodyEvent::CustodyEvent(Bundle *bundle, const EventCustodyAction action)
		: m_bundle(bundle), m_action(action), m_timer(NULL)
		{}

		CustodyEvent::CustodyEvent(string eid, const CustodySignalBlock *signal)
		: m_bundle(NULL), m_action(CUSTODY_REMOVE_TIMER), m_custody(signal), m_eid(eid), m_timer(NULL)
		{

		}

		CustodyEvent::CustodyEvent(CustodyTimer *timer)
		: m_bundle(NULL), m_action(CUSTODY_TIMEOUT), m_custody(NULL), m_eid(), m_timer(timer)
		{

		}

		CustodyEvent::~CustodyEvent()
		{}

		const CustodyTimer* CustodyEvent::getTimer() const
		{
			return m_timer;
		}

		const Bundle* CustodyEvent::getBundle() const
		{
			return m_bundle;
		}

		EventCustodyAction CustodyEvent::getAction() const
		{
			return m_action;
		}

		const CustodySignalBlock* CustodyEvent::getCustodySignal() const
		{
			return m_custody;
		}

		string CustodyEvent::getEID() const
		{
			return m_eid;
		}

		const string CustodyEvent::getName() const
		{
			return CustodyEvent::className;
		}

#ifdef DO_DEBUG_OUTPUT
		string CustodyEvent::toString()
		{
			return className;
		}
#endif

		const string CustodyEvent::className = "CustodyEvent";
	}
}

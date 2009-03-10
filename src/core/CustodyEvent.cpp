/*
 * CustodyEvent.cpp
 *
 *  Created on: 06.03.2009
 *      Author: morgenro
 */

#include "core/CustodyEvent.h"
#include "data/Exceptions.h"

using namespace dtn::core;
using namespace std;

namespace dtn
{
	namespace core
	{
		CustodyEvent::CustodyEvent(const Bundle &bundle, const EventCustodyAction action)
		: m_bundle(NULL), m_action(action), m_timer(NULL)
		{
			m_bundle = new Bundle(bundle);
		}

		CustodyEvent::CustodyEvent(string eid, const CustodySignalBlock &signal)
		: m_bundle(NULL), m_action(CUSTODY_REMOVE_TIMER), m_custody(NULL), m_eid(eid), m_timer(NULL)
		{
			m_custody = new CustodySignalBlock(signal);
		}

		CustodyEvent::CustodyEvent(CustodyTimer &timer)
		: m_bundle(NULL), m_action(CUSTODY_TIMEOUT), m_custody(NULL), m_eid(), m_timer(NULL)
		{
			m_timer = new CustodyTimer (timer);
		}

		CustodyEvent::~CustodyEvent()
		{
			if (m_timer != NULL) delete m_timer;
			if (m_custody != NULL) delete m_custody;
			if (m_bundle != NULL) delete m_bundle;
		}

		const CustodyTimer& CustodyEvent::getTimer() const
		{
			return *m_timer;
		}

		const Bundle& CustodyEvent::getBundle() const
		{
			if (m_bundle == NULL) throw exceptions::MissingObjectException();
			return *m_bundle;
		}

		EventCustodyAction CustodyEvent::getAction() const
		{
			return m_action;
		}

		const CustodySignalBlock& CustodyEvent::getCustodySignal() const
		{
			return *m_custody;
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

/*
 * CustodyEvent.cpp
 *
 *  Created on: 06.03.2009
 *      Author: morgenro
 */

#include "core/CustodyEvent.h"
#include "data/Exceptions.h"
#include "data/BundleFactory.h"

using namespace dtn::data;
using namespace dtn::core;
using namespace std;

namespace dtn
{
	namespace core
	{
		CustodyEvent::CustodyEvent(const Bundle &bundle, const EventCustodyAction action)
		: m_bundle(bundle), m_action(action)
		{
		}

		CustodyEvent::~CustodyEvent()
		{
		}

		const EventType CustodyEvent::getType() const
		{
			return EVENT_SYNC;
		}

		const Bundle& CustodyEvent::getBundle() const
		{
			return m_bundle;
		}

		EventCustodyAction CustodyEvent::getAction() const
		{
			return m_action;
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

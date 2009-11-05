/*
 * RouteEvent.cpp
 *
 *  Created on: 06.03.2009
 *      Author: morgenro
 */

#include "core/RouteEvent.h"
#include "ibrdtn/data/Exceptions.h"

using namespace dtn::core;
using namespace std;

namespace dtn
{
	namespace core
	{
		RouteEvent::RouteEvent(const Bundle &bundle, const EventRouteAction action)
		: m_bundle(bundle), m_action(action)
		{
		}

		RouteEvent::~RouteEvent()
		{
		}

		const EventType RouteEvent::getType() const
		{
			return EVENT_SYNC;
		}

		const Bundle& RouteEvent::getBundle() const
		{
			return m_bundle;
		}

		EventRouteAction RouteEvent::getAction() const
		{
			return m_action;
		}

		const string RouteEvent::getName() const
		{
			return RouteEvent::className;
		}

#ifdef DO_DEBUG_OUTPUT
		string RouteEvent::toString()
		{
			return className;
		}
#endif

		const string RouteEvent::className = "RouteEvent";
	}
}

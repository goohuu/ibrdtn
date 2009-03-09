/*
 * RouteEvent.cpp
 *
 *  Created on: 06.03.2009
 *      Author: morgenro
 */

#include "core/RouteEvent.h"

using namespace dtn::core;
using namespace std;

namespace dtn
{
	namespace core
	{
		RouteEvent::RouteEvent(BundleSchedule &schedule)
		: m_bundle(NULL), m_action(ROUTE_TRANSMIT_BUNDLE), m_schedule(schedule)
		{

		}
		RouteEvent::RouteEvent(BundleSchedule &schedule, Node &node)
		: m_action(ROUTE_TRANSMIT_BUNDLE), m_schedule(schedule)
		{

		}

		RouteEvent::RouteEvent(Bundle *bundle, const EventRouteAction action)
		: m_bundle(bundle), m_action(action), m_schedule(NULL, 0, "dtn:none")
		{}

		RouteEvent::~RouteEvent()
		{}

		BundleSchedule& RouteEvent::getSchedule()
		{
			return m_schedule;
		}

		Bundle* RouteEvent::getBundle() const
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

/*
 * RouteEvent.cpp
 *
 *  Created on: 06.03.2009
 *      Author: morgenro
 */

#include "core/RouteEvent.h"
#include "data/Exceptions.h"

using namespace dtn::core;
using namespace std;

namespace dtn
{
	namespace core
	{
		RouteEvent::RouteEvent(const BundleSchedule &schedule)
		: m_bundle(NULL), m_action(ROUTE_TRANSMIT_BUNDLE), m_schedule(schedule)
		{

		}
		RouteEvent::RouteEvent(const BundleSchedule &schedule, const Node &node)
		: m_bundle(NULL), m_action(ROUTE_TRANSMIT_BUNDLE), m_schedule(schedule), m_node(node)
		{

		}

		RouteEvent::RouteEvent(const Bundle &bundle, const EventRouteAction action)
		: m_bundle(NULL), m_action(action), m_schedule()
		{
			m_bundle = new Bundle(bundle);
		}

		RouteEvent::~RouteEvent()
		{
			if (m_bundle != NULL) delete m_bundle;
		}

		const BundleSchedule& RouteEvent::getSchedule() const
		{
			return m_schedule;
		}

		const Bundle& RouteEvent::getBundle() const
		{
			if (m_bundle == NULL) return m_schedule.getBundle();
			return *m_bundle;
		}

		EventRouteAction RouteEvent::getAction() const
		{
			return m_action;
		}

		const string RouteEvent::getName() const
		{
			return RouteEvent::className;
		}

		const Node& RouteEvent::getNode() const
		{
			return m_node;
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

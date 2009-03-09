/*
 * RouteEvent.h
 *
 *  Created on: 06.03.2009
 *      Author: morgenro
 */

#ifndef ROUTEEVENT_H_
#define ROUTEEVENT_H_

#include <string>
#include "data/Bundle.h"
#include "core/Event.h"
#include "core/BundleSchedule.h"
#include "core/Node.h"

using namespace dtn::data;
using namespace dtn::core;
using namespace std;

namespace dtn
{
	namespace core
	{
		enum EventRouteAction
		{
			ROUTE_FIND_SCHEDULE = 0,
			ROUTE_LOCAL_BUNDLE = 1,
			ROUTE_TRANSMIT_BUNDLE = 2
		};

		class RouteEvent : public Event
		{
		public:
			RouteEvent(BundleSchedule &schedule);
			RouteEvent(BundleSchedule &schedule, Node &node);
			RouteEvent(Bundle *bundle, const EventRouteAction action);
			~RouteEvent();

			EventRouteAction getAction() const;
			Bundle* getBundle() const;
			BundleSchedule& getSchedule();
			const string getName() const;

#ifdef DO_DEBUG_OUTPUT
			string toString();
#endif

			static const string className;

		private:
			Bundle *m_bundle;
			const EventRouteAction m_action;
			BundleSchedule m_schedule;
		};
	}
}

#endif /* ROUTEEVENT_H_ */

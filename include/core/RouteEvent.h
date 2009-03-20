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
			ROUTE_PROCESS_BUNDLE = 0
		};

		class RouteEvent : public Event
		{
		public:
			RouteEvent(const Bundle &bundle, const EventRouteAction action);
			~RouteEvent();

			EventRouteAction getAction() const;
			const Bundle& getBundle() const;
			const string getName() const;
			const EventType getType() const;

#ifdef DO_DEBUG_OUTPUT
			string toString();
#endif

			static const string className;

		private:
			const Bundle &m_bundle;
			const EventRouteAction m_action;
		};
	}
}

#endif /* ROUTEEVENT_H_ */

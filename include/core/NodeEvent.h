/*
 * NodeEvent.h
 *
 *  Created on: 05.03.2009
 *      Author: morgenro
 */

#ifndef NODEEVENT_H_
#define NODEEVENT_H_

#include <string>
#include "core/Node.h"
#include "core/Event.h"

using namespace dtn::core;
using namespace std;

namespace dtn
{
	namespace core
	{
		enum EventNodeAction
		{
			NODE_UNAVAILABLE = 0,
			NODE_AVAILABLE = 1,
			NODE_INFO_UPDATED = 2
		};

		class NodeEvent : public Event
		{
		public:
			NodeEvent(const Node &n, const EventNodeAction action);
			~NodeEvent();

			EventNodeAction getAction() const;
			const Node& getNode() const;
			const string getName() const;
			const EventType getType() const;

#ifdef DO_DEBUG_OUTPUT
			string toString();
#endif

			static const string className;

		private:
			const Node m_node;
			const EventNodeAction m_action;
		};
	}
}

#endif /* NODEEVENT_H_ */

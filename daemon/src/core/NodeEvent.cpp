/*
 * NodeEvent.cpp
 *
 *  Created on: 05.03.2009
 *      Author: morgenro
 */

#include "core/NodeEvent.h"

using namespace dtn::core;
using namespace std;

namespace dtn
{
	namespace core
	{
		NodeEvent::NodeEvent(const Node &n, const EventNodeAction action)
		: m_node(n), m_action(action)
		{}

		void NodeEvent::raise(const Node &n, const EventNodeAction action)
		{
			raiseEvent( new NodeEvent(n, action) );
		}

		NodeEvent::~NodeEvent()
		{}

		const Node& NodeEvent::getNode() const
		{
			return m_node;
		}

		EventNodeAction NodeEvent::getAction() const
		{
			return m_action;
		}

		const string NodeEvent::getName() const
		{
			return NodeEvent::className;
		}

#ifdef DO_DEBUG_OUTPUT
		string NodeEvent::toString() const
		{
			return className;
		}
#endif

		const string NodeEvent::className = "NodeEvent";
	}
}
/*
 * EventDebugger.cpp
 *
 *  Created on: 06.03.2009
 *      Author: morgenro
 */

#include "core/EventDebugger.h"
#include "core/EventSwitch.h"
#include "core/NodeEvent.h"
#include "core/RouteEvent.h"
#include "core/StorageEvent.h"
#include "core/CustodyEvent.h"
#include <iostream>

using namespace std;

namespace dtn
{
	namespace core
	{
		EventDebugger::EventDebugger()
		{
			EventSwitch::registerEventReceiver( NodeEvent::className, this );
			EventSwitch::registerEventReceiver( RouteEvent::className, this );
			EventSwitch::registerEventReceiver( StorageEvent::className, this );
			EventSwitch::registerEventReceiver( CustodyEvent::className, this );
		}

		EventDebugger::~EventDebugger()
		{
		}

		void EventDebugger::raiseEvent(const Event *evt)
		{
			cout << "Event<" << evt->getName() << ">: ";

			const NodeEvent *node = dynamic_cast<const NodeEvent*>(evt);
			const RouteEvent *route = dynamic_cast<const RouteEvent*>(evt);

			if (route != NULL)
			{
				switch (route->getAction())
				{
				case ROUTE_FIND_SCHEDULE:
					cout << "Find a schedule for " << route->getBundle()->toString();
					break;
				case ROUTE_LOCAL_BUNDLE:
					cout << "Received local bundle " << route->getBundle()->toString();
					break;
				}
			}
			if (node != NULL)
			{
				switch (node->getAction())
				{
				case NODE_INFO_UPDATED:
					cout << "Info updated for " << node->getNode().getURI();
					break;
				case NODE_AVAILABLE:
					cout << "Node " << node->getNode().getURI() << " available";
					break;
				case NODE_UNAVAILABLE:
					cout << "Node " << node->getNode().getURI() << " unavailable";
					break;
				}
			}

			cout << endl;
		}
	}
}

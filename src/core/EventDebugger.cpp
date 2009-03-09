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
			EventSwitch::unregisterEventReceiver( NodeEvent::className, this );
			EventSwitch::unregisterEventReceiver( RouteEvent::className, this );
			EventSwitch::unregisterEventReceiver( StorageEvent::className, this );
			EventSwitch::unregisterEventReceiver( CustodyEvent::className, this );
		}

		void EventDebugger::raiseEvent(const Event *evt)
		{
			cout << "Event<" << evt->getName() << ">: ";

			const NodeEvent *node = dynamic_cast<const NodeEvent*>(evt);
			const RouteEvent *route = dynamic_cast<const RouteEvent*>(evt);
			const StorageEvent *store = dynamic_cast<const StorageEvent*>(evt);

			if (store != NULL)
			{
				switch (store->getAction())
				{
				case STORE_BUNDLE:
					cout << "Store the bundle " << store->getBundle()->toString() << " in the storage." << endl;
					break;
				case STORE_SCHEDULE:
					BundleSchedule sched = store->getSchedule();
					cout << "Store a schedule for bundle " << sched.getBundle()->toString() << ", next hop: " << sched.getEID() << endl;
					break;
				}
			}
			if (route != NULL)
			{
				switch (route->getAction())
				{
				case ROUTE_FIND_SCHEDULE:
					cout << "Find a schedule for " << route->getBundle()->toString() << endl;
					break;
				case ROUTE_LOCAL_BUNDLE:
					cout << "Received local bundle " << route->getBundle()->toString() << endl;
					break;
				}
			}
			if (node != NULL)
			{
				switch (node->getAction())
				{
				case NODE_INFO_UPDATED:
					cout << "Info updated for " << node->getNode().getURI() << endl;
					break;
				case NODE_AVAILABLE:
					cout << "Node " << node->getNode().getURI() << " available" << endl;
					break;
				case NODE_UNAVAILABLE:
					cout << "Node " << node->getNode().getURI() << " unavailable" << endl;
					break;
				}
			}
		}
	}
}

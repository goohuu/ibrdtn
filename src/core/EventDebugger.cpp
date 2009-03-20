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
#include "core/BundleEvent.h"
#include "core/TimeEvent.h"
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
			EventSwitch::registerEventReceiver( BundleEvent::className, this );
			EventSwitch::registerEventReceiver( TimeEvent::className, this );
		}

		EventDebugger::~EventDebugger()
		{
			EventSwitch::unregisterEventReceiver( NodeEvent::className, this );
			EventSwitch::unregisterEventReceiver( RouteEvent::className, this );
			EventSwitch::unregisterEventReceiver( StorageEvent::className, this );
			EventSwitch::unregisterEventReceiver( CustodyEvent::className, this );
			EventSwitch::unregisterEventReceiver( BundleEvent::className, this );
			EventSwitch::unregisterEventReceiver( TimeEvent::className, this );
		}

		void EventDebugger::raiseEvent(const Event *evt)
		{
			cout << evt->getName() << ": ";

			const NodeEvent *node = dynamic_cast<const NodeEvent*>(evt);
			const RouteEvent *route = dynamic_cast<const RouteEvent*>(evt);
			const StorageEvent *store = dynamic_cast<const StorageEvent*>(evt);
			const BundleEvent *bundle = dynamic_cast<const BundleEvent*>(evt);
			const CustodyEvent *custody = dynamic_cast<const CustodyEvent*>(evt);
			const TimeEvent *time = dynamic_cast<const TimeEvent*>(evt);

			if (time != NULL)
			{
				switch (time->getAction())
				{
					case TIME_SECOND_TICK:
						cout << "time changed to " << time->getTimestamp() << endl;
						break;
				};
			}
			if (custody != NULL)
			{
				switch (custody->getAction())
				{
					case CUSTODY_ACCEPT:
						cout << "custody acceptance" << endl;
						break;

					case CUSTODY_REJECT:
						cout << "custody reject" << endl;
						break;
				};
			}
			if (bundle != NULL)
			{
				switch (bundle->getAction())
				{
				case BUNDLE_DELETED:
					cout << "bundle " << bundle->getBundle().toString() << " deleted" << endl;
					break;
				case BUNDLE_CUSTODY_ACCEPTED:
					cout << "custody accepted for " << bundle->getBundle().toString() << endl;
					break;
				case BUNDLE_FORWARDED:
					cout << "bundle " << bundle->getBundle().toString() << " forwarded" << endl;
					break;
				case BUNDLE_DELIVERED:
					cout << "bundle " << bundle->getBundle().toString() << " delivered" << endl;
					break;
				case BUNDLE_RECEIVED:
					cout << "bundle " << bundle->getBundle().toString() << " received" << endl;
					break;
				default:
					cout << "unknown" << endl;
					break;
				}
			}
			if (store != NULL)
			{
				switch (store->getAction())
				{
				case STORE_BUNDLE:
					cout << "Store the bundle " << store->getBundle().toString() << " in the storage." << endl;
					break;
				case STORE_SCHEDULE:
				{
					BundleSchedule sched = store->getSchedule();
					cout << "Store a schedule for bundle " << sched.getBundle().toString() << ", next hop: " << sched.getEID() << endl;
					break;
				}
				default:
					cout << "unknown" << endl;
					break;
				}
			}
			if (route != NULL)
			{
				switch (route->getAction())
				{
				case ROUTE_PROCESS_BUNDLE:
					cout << "process bundle " << route->getBundle().toString() << endl;
					break;
				default:
					cout << "unknown" << endl;
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
				default:
					cout << "unknown" << endl;
					break;
				}
			}
		}
	}
}

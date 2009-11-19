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
#include "core/CustodyEvent.h"
#include "core/BundleEvent.h"
#include <iostream>

using namespace std;
using namespace dtn::utils;

namespace dtn
{
	namespace core
	{
		EventDebugger::EventDebugger()
		{
			EventSwitch::registerEventReceiver( NodeEvent::className, this );
			EventSwitch::registerEventReceiver( RouteEvent::className, this );
			EventSwitch::registerEventReceiver( CustodyEvent::className, this );
			EventSwitch::registerEventReceiver( BundleEvent::className, this );
		}

		EventDebugger::~EventDebugger()
		{
			EventSwitch::unregisterEventReceiver( NodeEvent::className, this );
			EventSwitch::unregisterEventReceiver( RouteEvent::className, this );
			EventSwitch::unregisterEventReceiver( CustodyEvent::className, this );
			EventSwitch::unregisterEventReceiver( BundleEvent::className, this );
		}

		void EventDebugger::raiseEvent(const Event *evt)
		{
			const NodeEvent *node = dynamic_cast<const NodeEvent*>(evt);
			const RouteEvent *route = dynamic_cast<const RouteEvent*>(evt);
			const BundleEvent *bundle = dynamic_cast<const BundleEvent*>(evt);
			const CustodyEvent *custody = dynamic_cast<const CustodyEvent*>(evt);

			if (custody != NULL)
			{
				switch (custody->getAction())
				{
					case CUSTODY_ACCEPT:
						if (custody->getBundle()._procflags & Bundle::CUSTODY_REQUESTED)
						{
							dtn::utils::slog << SYSLOG_INFO << evt->getName() << ": custody acceptance" << endl;
						}
						break;

					case CUSTODY_REJECT:
						if (custody->getBundle()._procflags & Bundle::CUSTODY_REQUESTED)
						{
							dtn::utils::slog << SYSLOG_INFO << evt->getName() << ": custody reject" << endl;
						}
						break;
				};
			}
			if (bundle != NULL)
			{
				switch (bundle->getAction())
				{
				case BUNDLE_DELETED:
					dtn::utils::slog << SYSLOG_INFO << evt->getName() << ": bundle " << bundle->getBundle().toString() << " deleted" << endl;
					break;
				case BUNDLE_CUSTODY_ACCEPTED:
					dtn::utils::slog << SYSLOG_INFO << evt->getName() << ": custody accepted for " << bundle->getBundle().toString() << endl;
					break;
				case BUNDLE_FORWARDED:
					dtn::utils::slog << SYSLOG_INFO << evt->getName() << ": bundle " << bundle->getBundle().toString() << " forwarded" << endl;
					break;
				case BUNDLE_DELIVERED:
					dtn::utils::slog << SYSLOG_INFO << evt->getName() << ": bundle " << bundle->getBundle().toString() << " delivered" << endl;
					break;
				case BUNDLE_RECEIVED:
					dtn::utils::slog << SYSLOG_INFO << evt->getName() << ": bundle " << bundle->getBundle().toString() << " received" << endl;
					break;
				default:
					dtn::utils::slog << SYSLOG_INFO << evt->getName() << ": unknown" << endl;
					break;
				}
			}
			if (route != NULL)
			{
				switch (route->getAction())
				{
				case ROUTE_PROCESS_BUNDLE:
					dtn::utils::slog << SYSLOG_INFO << evt->getName() << ": process bundle " << route->getBundle().toString() << endl;
					break;
				default:
					dtn::utils::slog << SYSLOG_INFO << evt->getName() << ": unknown" << endl;
					break;
				}
			}
			if (node != NULL)
			{
				switch (node->getAction())
				{
				case NODE_INFO_UPDATED:
					//dtn::utils::slog << evt->getName() << ": Info updated for " << node->getNode().getURI() << endl;
					break;
				case NODE_AVAILABLE:
					dtn::utils::slog << SYSLOG_INFO << evt->getName() << ": Node " << node->getNode().getURI() << " available over ";

					if (node->getNode().getProtocol() == dtn::core::UDP_CONNECTION)
					{
						dtn::utils::slog << "UDP";
					}
					else if (node->getNode().getProtocol() == dtn::core::TCP_CONNECTION)
					{
						dtn::utils::slog << "TCP";
					}
					else
					{
						dtn::utils::slog << "a unsupported connection";
					}
					dtn::utils::slog << endl;
					break;
				case NODE_UNAVAILABLE:
					dtn::utils::slog << SYSLOG_INFO << evt->getName() << ": Node " << node->getNode().getURI() << " unavailable" << endl;
					break;
				default:
					dtn::utils::slog << SYSLOG_INFO << evt->getName() << ": unknown" << endl;
					break;
				}
			}
		}
	}
}

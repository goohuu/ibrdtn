/*
 * EventDebugger.cpp
 *
 *  Created on: 06.03.2009
 *      Author: morgenro
 */

#include "core/EventDebugger.h"
#include "core/NodeEvent.h"
#include "core/CustodyEvent.h"
#include "core/BundleEvent.h"
#include "core/TimeEvent.h"
#include <iostream>

using namespace std;
using namespace ibrcommon;

namespace dtn
{
	namespace core
	{
		EventDebugger::EventDebugger()
		{
		}

		EventDebugger::~EventDebugger()
		{
		}

		void EventDebugger::raiseEvent(const Event *evt)
		{
			const NodeEvent *node = dynamic_cast<const NodeEvent*>(evt);
			const BundleEvent *bundle = dynamic_cast<const BundleEvent*>(evt);
			const CustodyEvent *custody = dynamic_cast<const CustodyEvent*>(evt);
			const TimeEvent *time = dynamic_cast<const TimeEvent*>(evt);

			if (custody != NULL)
			{
				switch (custody->getAction())
				{
					case CUSTODY_ACCEPT:
						if (custody->getBundle()._procflags & Bundle::CUSTODY_REQUESTED)
						{
							ibrcommon::slog << SYSLOG_INFO << evt->getName() << ": custody acceptance" << endl;
						}
						break;

					case CUSTODY_REJECT:
						if (custody->getBundle()._procflags & Bundle::CUSTODY_REQUESTED)
						{
							ibrcommon::slog << SYSLOG_INFO << evt->getName() << ": custody reject" << endl;
						}
						break;
				};
			}
			else if (bundle != NULL)
			{
				switch (bundle->getAction())
				{
				case BUNDLE_DELETED:
					ibrcommon::slog << SYSLOG_INFO << evt->getName() << ": bundle " << bundle->getBundle().toString() << " deleted" << endl;
					break;
				case BUNDLE_CUSTODY_ACCEPTED:
					ibrcommon::slog << SYSLOG_INFO << evt->getName() << ": custody accepted for " << bundle->getBundle().toString() << endl;
					break;
				case BUNDLE_FORWARDED:
					ibrcommon::slog << SYSLOG_INFO << evt->getName() << ": bundle " << bundle->getBundle().toString() << " forwarded" << endl;
					break;
				case BUNDLE_DELIVERED:
					ibrcommon::slog << SYSLOG_INFO << evt->getName() << ": bundle " << bundle->getBundle().toString() << " delivered" << endl;
					break;
				case BUNDLE_RECEIVED:
					ibrcommon::slog << SYSLOG_INFO << evt->getName() << ": bundle " << bundle->getBundle().toString() << " received" << endl;
					break;
				default:
					ibrcommon::slog << SYSLOG_INFO << evt->getName() << ": unknown" << endl;
					break;
				}
			}
			else if (node != NULL)
			{
				switch (node->getAction())
				{
				case NODE_INFO_UPDATED:
					//ibrcommon::slog << evt->getName() << ": Info updated for " << node->getNode().getURI() << endl;
					break;
				case NODE_AVAILABLE:
					ibrcommon::slog << SYSLOG_INFO << evt->getName() << ": Node " << node->getNode().getURI() << " available over ";

					if (node->getNode().getProtocol() == dtn::core::UDP_CONNECTION)
					{
						ibrcommon::slog << "UDP";
					}
					else if (node->getNode().getProtocol() == dtn::core::TCP_CONNECTION)
					{
						ibrcommon::slog << "TCP";
					}
					else
					{
						ibrcommon::slog << "a unsupported connection";
					}
					ibrcommon::slog << endl;
					break;
				case NODE_UNAVAILABLE:
					ibrcommon::slog << SYSLOG_INFO << evt->getName() << ": Node " << node->getNode().getURI() << " unavailable" << endl;
					break;
				default:
					ibrcommon::slog << SYSLOG_INFO << evt->getName() << ": unknown" << endl;
					break;
				}
			}
			else if (time != NULL)
			{
				// do not debug print time events
			}
			else
			{
				// unknown event
				ibrcommon::slog << SYSLOG_INFO << evt->toString() << endl;
			}
		}
	}
}

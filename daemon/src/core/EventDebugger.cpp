/*
 * EventDebugger.cpp
 *
 *  Created on: 06.03.2009
 *      Author: morgenro
 */

#include "config.h"

#ifdef DO_DEBUG_OUTPUT

#include "core/EventDebugger.h"
#include "core/NodeEvent.h"
#include "core/CustodyEvent.h"
#include "core/BundleEvent.h"
#include "core/TimeEvent.h"
#include <ibrcommon/Logger.h>
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
							IBRCOMMON_LOGGER(notice) << evt->getName() << ": custody acceptance" << IBRCOMMON_LOGGER_ENDL;
						}
						break;

					case CUSTODY_REJECT:
						if (custody->getBundle()._procflags & Bundle::CUSTODY_REQUESTED)
						{
							IBRCOMMON_LOGGER(notice) << evt->getName() << ": custody reject" << IBRCOMMON_LOGGER_ENDL;
						}
						break;
				};
			}
			else if (bundle != NULL)
			{
				switch (bundle->getAction())
				{
				case BUNDLE_DELETED:
					IBRCOMMON_LOGGER(notice) << evt->getName() << ": bundle " << bundle->getBundle().toString() << " deleted" << IBRCOMMON_LOGGER_ENDL;
					break;
				case BUNDLE_CUSTODY_ACCEPTED:
					IBRCOMMON_LOGGER(notice) << evt->getName() << ": custody accepted for " << bundle->getBundle().toString() << IBRCOMMON_LOGGER_ENDL;
					break;
				case BUNDLE_FORWARDED:
					IBRCOMMON_LOGGER(notice) << evt->getName() << ": bundle " << bundle->getBundle().toString() << " forwarded" << IBRCOMMON_LOGGER_ENDL;
					break;
				case BUNDLE_DELIVERED:
					IBRCOMMON_LOGGER(notice) << evt->getName() << ": bundle " << bundle->getBundle().toString() << " delivered" << IBRCOMMON_LOGGER_ENDL;
					break;
				case BUNDLE_RECEIVED:
					IBRCOMMON_LOGGER(notice) << evt->getName() << ": bundle " << bundle->getBundle().toString() << " received" << IBRCOMMON_LOGGER_ENDL;
					break;
				default:
					IBRCOMMON_LOGGER(notice) << evt->getName() << ": unknown" << IBRCOMMON_LOGGER_ENDL;
					break;
				}
			}
			else if (node != NULL)
			{
				switch (node->getAction())
				{
				case NODE_INFO_UPDATED:
					IBRCOMMON_LOGGER_DEBUG(10) << evt->getName() << ": Info updated for " << node->getNode().getURI() << IBRCOMMON_LOGGER_ENDL;
					break;
				case NODE_AVAILABLE:
				{
					std::string proto = "a unsupported connection";

					if (node->getNode().getProtocol() == dtn::core::UDP_CONNECTION)
					{
						proto = "UDP";
					}
					else if (node->getNode().getProtocol() == dtn::core::TCP_CONNECTION)
					{
						proto = "TCP";
					}

					IBRCOMMON_LOGGER(notice) << evt->getName() << ": Node " << node->getNode().getURI() << " available over " << proto << IBRCOMMON_LOGGER_ENDL;
					break;
				}
				case NODE_UNAVAILABLE:
					IBRCOMMON_LOGGER(notice) << evt->getName() << ": Node " << node->getNode().getURI() << " unavailable" << IBRCOMMON_LOGGER_ENDL;
					break;
				default:
					IBRCOMMON_LOGGER(notice) << evt->getName() << ": unknown" << IBRCOMMON_LOGGER_ENDL;
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
				IBRCOMMON_LOGGER(notice) << evt->toString() << IBRCOMMON_LOGGER_ENDL;
			}
		}
	}
}

#endif


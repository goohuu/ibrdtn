/*
 * Notifier.cpp
 *
 *  Created on: 11.12.2009
 *      Author: morgenro
 */

#include "daemon/Notifier.h"
#include "core/EventSwitch.h"
#include "core/NodeEvent.h"
#include <stdlib.h>

namespace dtn
{
	namespace daemon
	{
		using namespace dtn::core;

		Notifier::Notifier(std::string cmd) : _cmd(cmd)
		{
			EventSwitch::registerEventReceiver( NodeEvent::className, this );
		}

		Notifier::~Notifier()
		{
			EventSwitch::unregisterEventReceiver( NodeEvent::className, this );
		}

		void Notifier::raiseEvent(const dtn::core::Event *evt)
		{
			const NodeEvent *node = dynamic_cast<const NodeEvent*>(evt);

			if (node != NULL)
			{
				stringstream msg;

				switch (node->getAction())
				{
				case NODE_AVAILABLE:
					msg << "Node is available: " << node->getNode().getURI();
					notify("IBR-DTN", msg.str());
					break;

				case NODE_UNAVAILABLE:
					msg << "Node is unavailable: " << node->getNode().getURI();
					notify("IBR-DTN", msg.str());
					break;
				default:
					break;
				}
			}
		}

		void Notifier::notify(string title, string msg)
		{
			stringstream notifycmd;
			notifycmd << _cmd;
			notifycmd << " \"" << title << "\" \"" << msg << "\"";

			int ret = system(notifycmd.str().c_str());
		}
	}
}

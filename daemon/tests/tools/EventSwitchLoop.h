/*
 * EventSwitchLoop.h
 *
 *  Created on: 02.11.2010
 *      Author: morgenro
 */

#include "src/core/EventSwitch.h"
#include "src/core/GlobalEvent.h"
#include <ibrcommon/thread/Thread.h>

#ifndef EVENTSWITCHLOOP_H_
#define EVENTSWITCHLOOP_H_

namespace ibrtest
{
	class EventSwitchLoop : public ibrcommon::JoinableThread
	{
	public:
		EventSwitchLoop()
		{
			dtn::core::EventSwitch &es = dtn::core::EventSwitch::getInstance();
			es.clear();
		};

		virtual ~EventSwitchLoop()
		{
			dtn::core::GlobalEvent::raise(dtn::core::GlobalEvent::GLOBAL_SHUTDOWN);
			join();
		};

	protected:
		void run()
		{
			dtn::core::EventSwitch &es = dtn::core::EventSwitch::getInstance();
			es.loop();
		}

		bool __cancellation()
		{
			dtn::core::GlobalEvent::raise(dtn::core::GlobalEvent::GLOBAL_SHUTDOWN);
			return true;
		}
	};
}

#endif /* EVENTSWITCHLOOP_H_ */

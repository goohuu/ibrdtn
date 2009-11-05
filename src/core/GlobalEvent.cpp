/*
 * GlobalEvent.h
 *
 *  Created on: 30.10.2009
 *      Author: morgenro
 */

#include "core/GlobalEvent.h"


namespace dtn
{
	namespace core
	{
		GlobalEvent::GlobalEvent(Action a)
		 : _action(a)
		{
		}

		GlobalEvent::~GlobalEvent()
		{
		}

		const GlobalEvent::Action GlobalEvent::getAction() const
		{
			return _action;
		}

		const EventType GlobalEvent::getType() const
		{
			return EVENT_SYNC;
		}

		const string GlobalEvent::getName() const
		{
			return GlobalEvent::className;
		}

#ifdef DO_DEBUG_OUTPUT
		string GlobalEvent::toString()
		{
			return className;
		}
#endif

		const string GlobalEvent::className = "GlobalEvent";
	}
}

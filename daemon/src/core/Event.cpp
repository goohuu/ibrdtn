/*
 * Event.cpp
 *
 *  Created on: 16.02.2010
 *      Author: morgenro
 */

#include "core/Event.h"
#include "core/EventSwitch.h"

namespace dtn
{
	namespace core
	{
		Event::~Event() {};

		void Event::raiseEvent(Event *evt)
		{
			// raise the new event
			dtn::core::EventSwitch::raiseEvent( evt );
		}
	}
}

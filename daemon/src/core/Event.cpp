/*
 * Event.cpp
 *
 *  Created on: 16.02.2010
 *      Author: morgenro
 */

#include "core/Event.h"
#include "core/EventSwitch.h"
#include <ibrcommon/thread/MutexLock.h>

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

		void Event::set_ref_count(size_t c)
		{
			ibrcommon::MutexLock l(_ref_count_mutex);
			_ref_count = c;
		}

		bool Event::decrement_ref_count()
		{
			ibrcommon::MutexLock l(_ref_count_mutex);
			if (_ref_count == 0)
			{
				throw ibrcommon::Exception("This reference count is already zero!");
			}

			_ref_count--;

			if (_ref_count == 0) return true;

			return false;
		}
	}
}

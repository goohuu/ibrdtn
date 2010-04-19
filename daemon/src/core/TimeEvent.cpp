/*
 * TimeEvent.cpp
 *
 *  Created on: 16.03.2009
 *      Author: morgenro
 */

#include "core/TimeEvent.h"

namespace dtn
{
	namespace core
	{
		TimeEvent::TimeEvent(const size_t timestamp, const TimeEventAction action)
		: m_timestamp(timestamp), m_action(action)
		{

		}

		TimeEvent::~TimeEvent()
		{

		}

		TimeEventAction TimeEvent::getAction() const
		{
			return m_action;
		}

		size_t TimeEvent::getTimestamp() const
		{
			return m_timestamp;
		}

		const std::string TimeEvent::getName() const
		{
			return TimeEvent::className;
		}

		void TimeEvent::raise(const size_t timestamp, const TimeEventAction action)
		{
			// raise the new event
			raiseEvent( new TimeEvent(timestamp, action) );
		}

#ifdef DO_DEBUG_OUTPUT
		std::string TimeEvent::toString() const
		{

		}
#endif

		const std::string TimeEvent::className = "TimeEvent";
	}
}

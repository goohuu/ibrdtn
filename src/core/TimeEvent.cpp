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

		const string TimeEvent::getName() const
		{
			return TimeEvent::className;
		}

		const EventType TimeEvent::getType() const
		{
			EVENT_ASYNC;
		}

#ifdef DO_DEBUG_OUTPUT
		string TimeEvent::toString()
		{

		}
#endif

		const string TimeEvent::className = "TimeEvent";
	}
}

/*
 * TimeEvent.h
 *
 *  Created on: 16.03.2009
 *      Author: morgenro
 */

#ifndef TIMEEVENT_H_
#define TIMEEVENT_H_

#include "ibrdtn/default.h"
#include "core/Event.h"

namespace dtn
{
	namespace core
	{
		enum TimeEventAction
		{
			TIME_SECOND_TICK = 0
		};

		class TimeEvent : public Event
		{
		public:
			TimeEvent(const size_t timestamp, const TimeEventAction action);
			virtual ~TimeEvent();

			TimeEventAction getAction() const;
			size_t getTimestamp() const;
			const string getName() const;
			const EventType getType() const;

#ifdef DO_DEBUG_OUTPUT
			string toString();
#endif

			static const string className;

		private:
			const size_t m_timestamp;
			const TimeEventAction m_action;
		};
	}
}

#endif /* TIMEEVENT_H_ */

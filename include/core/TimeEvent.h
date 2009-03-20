/*
 * TimeEvent.h
 *
 *  Created on: 16.03.2009
 *      Author: morgenro
 */

#include "config.h"

#ifndef TIMEEVENT_H_
#define TIMEEVENT_H_


#include <string>
#include "core/Event.h"

using namespace std;

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
			~TimeEvent();

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

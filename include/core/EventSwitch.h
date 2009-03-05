/*
 * EventSwitch.h
 *
 *  Created on: 05.03.2009
 *      Author: morgenro
 */

#ifndef EVENTSWITCH_H_
#define EVENTSWITCH_H_

#include "data/Exceptions.h"
#include "core/Event.h"
#include "core/EventReceiver.h"

#include <map>
#include <list>

using namespace std;
using namespace dtn::exceptions;

namespace dtn
{
	namespace exceptions
	{
		class EventException : public Exception
		{

		};

		class NoSuchEventException : public EventException
		{

		};
	}

	namespace core
	{
		class EventSwitch
		{
		private:
			EventSwitch();
			~EventSwitch();
			static EventSwitch& getInstance();

			map<string,list<EventReceiver*> > m_list;

		public:
			static void registerEventReceiver(string eventName, EventReceiver *receiver);
			static void raiseEvent(Event *evt);
		};
	}
}

#endif /* EVENTSWITCH_H_ */

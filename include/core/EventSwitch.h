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
#include "utils/Service.h"
#include "utils/Mutex.h"
#include "utils/MutexLock.h"

#include <map>
#include <list>
#include <queue>

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
		class EventSwitch : public dtn::utils::Service
		{
		private:
			EventSwitch();
			~EventSwitch();
			static EventSwitch& getInstance();

			map<string,list<EventReceiver*> > m_list;
			queue<Event*> m_queue;
			dtn::utils::Mutex m_queuelock;

			void push(Event *evt);

		protected:
			void tick();

		public:
			static void registerEventReceiver(string eventName, EventReceiver *receiver);
			static void unregisterEventReceiver(string eventName, EventReceiver *receiver);
			static void raiseEvent(Event *evt);
		};
	}
}

#endif /* EVENTSWITCH_H_ */

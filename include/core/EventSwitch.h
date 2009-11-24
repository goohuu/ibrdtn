/*
 * EventSwitch.h
 *
 *  Created on: 05.03.2009
 *      Author: morgenro
 */

#ifndef EVENTSWITCH_H_
#define EVENTSWITCH_H_

#include "ibrdtn/default.h"
#include "ibrdtn/data/Exceptions.h"
#include "core/Event.h"
#include "core/EventReceiver.h"

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

		class NoReceiverFoundException : public Exception
		{

		};
	}

	namespace core
	{
		class EventSwitch
		{
		private:
			EventSwitch();
			virtual ~EventSwitch();
			static EventSwitch& getInstance();

			map<string,list<EventReceiver*> > m_list;
			queue<Event*> m_queue;
			dtn::utils::Mutex m_queuelock;

			void direct(const Event *evt);

			const list<EventReceiver*>& getReceivers(string eventName) const;
			list<EventReceiver*>& getReceivers(string eventName);

		public:
			static void registerEventReceiver(string eventName, EventReceiver *receiver);
			static void unregisterEventReceiver(string eventName, EventReceiver *receiver);
			static void raiseEvent(Event *evt);
		};
	}
}

#endif /* EVENTSWITCH_H_ */

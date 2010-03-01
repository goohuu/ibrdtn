/*
 * EventSwitch.h
 *
 *  Created on: 05.03.2009
 *      Author: morgenro
 */

#ifndef EVENTSWITCH_H_
#define EVENTSWITCH_H_

#include "ibrdtn/default.h"
#include "ibrcommon/Exceptions.h"
#include "core/Event.h"
#include "core/EventReceiver.h"
#include "ibrcommon/thread/Thread.h"

#ifdef DO_DEBUG_OUTPUT
#include "core/EventDebugger.h"
#endif

namespace dtn
{
	namespace core
	{
		class EventException : public ibrcommon::Exception
		{

		};

		class NoSuchEventException : public EventException
		{

		};

		class NoReceiverFoundException : public ibrcommon::Exception
		{

		};

		class EventSwitch : public ibrcommon::JoinableThread
		{
		private:
			EventSwitch();
			virtual ~EventSwitch();
			static EventSwitch& getInstance();

			ibrcommon::Mutex _receiverlock;
			map<string,list<EventReceiver*> > _list;
			queue<Event*> _queue;
			ibrcommon::Conditional _cond_queue;
			bool _running;

#ifdef DO_DEBUG_OUTPUT
			// create event debugger
			EventDebugger _debugger;
#endif

			const list<EventReceiver*>& getReceivers(string eventName) const;
		protected:
			void run();

			friend class Event;
			friend class EventReceiver;

			static void registerEventReceiver(string eventName, EventReceiver *receiver);
			static void unregisterEventReceiver(string eventName, EventReceiver *receiver);
			static void raiseEvent(Event *evt);

		public:
			static void stop();
		};
	}
}

#endif /* EVENTSWITCH_H_ */

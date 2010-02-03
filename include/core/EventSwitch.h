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
#include "ibrcommon/thread/Thread.h"

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

			const list<EventReceiver*>& getReceivers(string eventName) const;
		protected:
			void run();

		public:
			static void registerEventReceiver(string eventName, EventReceiver *receiver);
			static void unregisterEventReceiver(string eventName, EventReceiver *receiver);
			static void raiseEvent(Event *evt);
			static void stop();
		};
	}
}

#endif /* EVENTSWITCH_H_ */

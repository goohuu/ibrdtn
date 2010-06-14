/*
 * EventSwitch.h
 *
 *  Created on: 05.03.2009
 *      Author: morgenro
 */

#ifndef EVENTSWITCH_H_
#define EVENTSWITCH_H_

#include "core/Event.h"
#include "core/EventReceiver.h"
#include "Component.h"
#include <ibrcommon/Exceptions.h>
#include <ibrcommon/thread/Mutex.h>
#include <ibrcommon/thread/Conditional.h>
#include <ibrcommon/thread/ThreadSafeQueue.h>
#include "core/EventDebugger.h"

#include <list>
#include <map>
#include <queue>

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

		class EventSwitch : public dtn::daemon::IndependentComponent
		{
		private:
			EventSwitch();
			virtual ~EventSwitch();

			ibrcommon::Mutex _receiverlock;
			std::map<std::string,std::list<EventReceiver*> > _list;
			ibrcommon::ThreadSafeQueue<dtn::core::Event*> _queue;
			bool _running;

			// create event debugger
			EventDebugger _debugger;

			const std::list<EventReceiver*>& getReceivers(std::string eventName) const;

		protected:
			virtual void componentUp();
			virtual void componentRun();
			virtual void componentDown();

			friend class Event;
			friend class EventReceiver;

			static void registerEventReceiver(string eventName, EventReceiver *receiver);
			static void unregisterEventReceiver(string eventName, EventReceiver *receiver);
			static void raiseEvent(Event *evt);

		public:
			static EventSwitch& getInstance();
		};
	}
}

#endif /* EVENTSWITCH_H_ */

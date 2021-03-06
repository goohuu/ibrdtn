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
#include "core/EventDebugger.h"

#include <list>
#include <map>

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

		class EventSwitch : public dtn::daemon::IntegratedComponent
		{
		private:
			EventSwitch();
			virtual ~EventSwitch();

			ibrcommon::Mutex _receiverlock;
			std::map<std::string, std::list<EventReceiver*> > _list;

			bool _running;

			// create event debugger
			EventDebugger _debugger;

			const std::list<EventReceiver*>& getReceivers(std::string eventName) const;

			/**
			 * @see Component::getName()
			 */
			virtual const std::string getName() const;

			class Task
			{
			public:
				Task();
				Task(EventReceiver *er, dtn::core::Event *evt);
				~Task();

				EventReceiver *receiver;
				dtn::core::Event *event;
			};

			class Worker : public ibrcommon::JoinableThread
			{
			public:
				Worker(EventSwitch &sw);
				~Worker();

			protected:
				void run();

			private:
				EventSwitch &_switch;
			};

			ibrcommon::Conditional _queue_cond;
			std::list<Task*> _queue;
			std::list<Task*> _prio_queue;
			std::list<Task*> _low_queue;

			ibrcommon::Conditional _active_cond;
			size_t _active_worker;
			bool _pause;

			void process();

			void pause();
			void unpause();

			void unregister(std::string eventName, EventReceiver *receiver);

		protected:
			virtual void componentUp();
			virtual void componentDown();

			friend class Event;
			friend class EventReceiver;

			static void registerEventReceiver(string eventName, EventReceiver *receiver);
			static void unregisterEventReceiver(string eventName, EventReceiver *receiver);
			static void raiseEvent(Event *evt);

		public:
			static EventSwitch& getInstance();
			void loop(size_t threads = 0);
			void clear();

			friend class Worker;
		};
	}
}

#endif /* EVENTSWITCH_H_ */

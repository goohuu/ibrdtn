/*
 * EventSwitch.cpp
 *
 *  Created on: 05.03.2009
 *      Author: morgenro
 */

#include "core/EventSwitch.h"

#include <ibrcommon/thread/MutexLock.h>
#include "core/GlobalEvent.h"
#include <stdexcept>
#include <iostream>
#include <typeinfo>

namespace dtn
{
	namespace core
	{
		EventSwitch::EventSwitch()
		 : _running(true)
		{
		}

		EventSwitch::~EventSwitch()
		{
			componentDown();
		}

		void EventSwitch::componentUp()
		{
		}

		void EventSwitch::componentDown()
		{
			try {
				ibrcommon::MutexLock l(_queue_cond);
				_running = false;

				// wait until the queue is empty
				while (!_queue.empty())
				{
					_queue_cond.wait();
				}

				_queue_cond.abort();
			} catch (const ibrcommon::Conditional::ConditionalAbortException&) {};
		}

		void EventSwitch::process()
		{
			EventSwitch::Task *t = NULL;

			// just look for an event to process
			{
				ibrcommon::MutexLock l(_queue_cond);
				while (_queue.empty())
				{
					_queue_cond.wait();
				}

				t = _queue.front();
				_queue.pop();
			}

			try {
				// execute the event
				t->receiver->raiseEvent(t->event);
			} catch (...) {};

			// delete the Task
			delete t;
		}

		void EventSwitch::loop(size_t threads)
		{
			// create worker threads
			std::list<Worker*> wlist;

			for (size_t i = 0; i < threads; i++)
			{
				Worker *w = new Worker(*this);
				w->start();
				wlist.push_back(w);
			}

			try {
				while (_running)
				{
					process();
				}
			} catch (const ibrcommon::Conditional::ConditionalAbortException&) { };

			for (std::list<Worker*>::iterator iter = wlist.begin(); iter != wlist.end(); iter++)
			{
				Worker *w = (*iter);
				w->join();
				delete w;
			}
		}

		const list<EventReceiver*>& EventSwitch::getReceivers(string eventName) const
		{
			map<string,list<EventReceiver*> >::const_iterator iter = _list.find(eventName);

			if (iter == _list.end())
			{
				throw NoReceiverFoundException();
			}

			return iter->second;
		}

		void EventSwitch::registerEventReceiver(string eventName, EventReceiver *receiver)
		{
			// get the list for this event
			EventSwitch &s = EventSwitch::getInstance();
			ibrcommon::MutexLock l(s._receiverlock);
			s._list[eventName].push_back(receiver);
		}

		void EventSwitch::unregisterEventReceiver(string eventName, EventReceiver *receiver)
		{
			// get the list for this event
			EventSwitch &s = EventSwitch::getInstance();
			ibrcommon::MutexLock l(s._receiverlock);
			s._list[eventName].remove( receiver );
		}

		void EventSwitch::raiseEvent(Event *evt)
		{
			EventSwitch &s = EventSwitch::getInstance();

			// do not process any event if the system is going down
			{
				ibrcommon::MutexLock l(s._queue_cond);
				if (!s._running) return;
			}

			// forward to debugger
			s._debugger.raiseEvent(evt);

			try {
				dtn::core::GlobalEvent &global = dynamic_cast<dtn::core::GlobalEvent&>(*evt);

				if (global.getAction() == dtn::core::GlobalEvent::GLOBAL_SHUTDOWN)
				{
					// stop receiving events
					s.componentDown();
				}
			} catch (const std::bad_cast&) { }

			try {
				ibrcommon::MutexLock reglock(s._receiverlock);

				// get the list for this event
				const std::list<EventReceiver*> receivers = s.getReceivers(evt->getName());
				evt->set_ref_count(receivers.size());

				for (list<EventReceiver*>::const_iterator iter = receivers.begin(); iter != receivers.end(); iter++)
				{
					Task *t = new Task(*iter, evt);
					ibrcommon::MutexLock l(s._queue_cond);
					s._queue.push(t);
					s._queue_cond.signal();
				}
			} catch (const NoReceiverFoundException&) {
				// No receiver available!
			}
		}

		EventSwitch& EventSwitch::getInstance()
		{
			static EventSwitch instance;
			return instance;
		}

		const std::string EventSwitch::getName() const
		{
			return "EventSwitch";
		}

		void EventSwitch::clear()
		{
			ibrcommon::MutexLock l(_receiverlock);
			_list.clear();
		}

		EventSwitch::Task::Task()
		 : receiver(NULL), event(NULL)
		{
		}

		EventSwitch::Task::Task(EventReceiver *er, dtn::core::Event *evt)
		 : receiver(er), event(evt)
		{
		}

		EventSwitch::Task::~Task()
		{
			if (event != NULL)
			{
				if (event->decrement_ref_count())
				{
					delete event;
				}
			}
		}

		EventSwitch::Worker::Worker(EventSwitch &sw)
		 : _switch(sw)
		{}

		EventSwitch::Worker::~Worker()
		{}

		void EventSwitch::Worker::run()
		{
			try {
				while (true)
					_switch.process();
			} catch (const ibrcommon::Conditional::ConditionalAbortException&) { };
		}
	}
}


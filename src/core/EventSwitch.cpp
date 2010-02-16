/*
 * EventSwitch.cpp
 *
 *  Created on: 05.03.2009
 *      Author: morgenro
 */

#include "core/EventSwitch.h"

#include <stdexcept>
#include <iostream>
using namespace std;

namespace dtn
{
	namespace core
	{
		EventSwitch::EventSwitch()
		 : _running(false)
		{

		}

		EventSwitch::~EventSwitch()
		{
			{
				ibrcommon::MutexLock l(_cond_queue);
				_running = false;
				_cond_queue.signal(true);
			}
			join();

			// cleanup queue
			while (!_queue.empty())
			{
				Event *evt = _queue.front();
				_queue.pop();
				delete evt;
			}
		}

		void EventSwitch::run()
		{
			while (_running)
			{
				Event *evt = NULL;

				{
					ibrcommon::MutexLock l(_cond_queue);
					if (_queue.empty()) _cond_queue.wait();
					if (!_running) return;

					// get item out of the queue
					evt = _queue.front();
					_queue.pop();
				}

#ifdef DO_DEBUG_OUTPUT
				// forward to debugger
				_debugger.raiseEvent(evt);
#endif

				try {
					// get the list for this event
					const list<EventReceiver*> receivers = getReceivers(evt->getName());

					for (list<EventReceiver*>::const_iterator iter = receivers.begin(); iter != receivers.end(); iter++)
					{
						(*iter)->raiseEvent(evt);
					}
				} catch (NoReceiverFoundException ex) {
					// No receiver available!
				}

				// delete the event
				delete evt;

				// spend some extra time
				yield();
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

			ibrcommon::MutexLock l(s._cond_queue);

			s._queue.push(evt);
			s._cond_queue.signal(true);
		}

		EventSwitch& EventSwitch::getInstance()
		{
			static EventSwitch instance;
			if (!instance._running)
			{
				instance._running = true;
				instance.start();
			}
			return instance;
		}

		void EventSwitch::stop()
		{
			EventSwitch &s = EventSwitch::getInstance();
			s._running = false;
			s.join();
		}
	}
}

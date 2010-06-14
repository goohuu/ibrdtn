/*
 * EventSwitch.cpp
 *
 *  Created on: 05.03.2009
 *      Author: morgenro
 */

#include "core/EventSwitch.h"

#include <ibrcommon/thread/MutexLock.h>
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
			componentDown();
		}

		void EventSwitch::componentUp()
		{
		}

		void EventSwitch::componentDown()
		{
			try {
				while (!_queue.empty())
				{
					delete _queue.frontpop();
				}
			} catch (ibrcommon::Exception) {

			}

			_running = false;
			_queue.unblock();
		}

		void EventSwitch::loop()
		{
			_running = true;

			try {
				while (_running)
				{
					Event *evt = _queue.blockingpop();

					{
						ibrcommon::MutexLock reglock(_receiverlock);

						// forward to debugger
						_debugger.raiseEvent(evt);

						try {
							// get the list for this event
							const list<EventReceiver*> receivers = getReceivers(evt->getName());

							for (list<EventReceiver*>::const_iterator iter = receivers.begin(); iter != receivers.end(); iter++)
							{
								try {
									(*iter)->raiseEvent(evt);
								} catch (...) {
									// An error occurred during event raising
								}
							}
						} catch (NoReceiverFoundException ex) {
							// No receiver available!
						}
					}

					delete evt;
				}
			} catch (ibrcommon::Exception) {
				_running = false;
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
			s._queue.push(evt);
		}

		EventSwitch& EventSwitch::getInstance()
		{
			static EventSwitch instance;
			return instance;
		}
	}
}

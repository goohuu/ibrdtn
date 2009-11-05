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
		{

		}

		EventSwitch::~EventSwitch()
		{
		}

		void EventSwitch::registerEventReceiver(string eventName, EventReceiver *receiver)
		{
			// get the list for this event
			EventSwitch &s = EventSwitch::getInstance();

			try {
				list<EventReceiver*> &receivers = s.m_list.at(eventName);
				receivers.push_back(receiver);
			} catch (std::out_of_range ex) {
				list<EventReceiver*> receivers;
				receivers.push_back(receiver);
				s.m_list[eventName] = receivers;
			}
		}

		void EventSwitch::unregisterEventReceiver(string eventName, EventReceiver *receiver)
		{
			// get the list for this event
			EventSwitch &s = EventSwitch::getInstance();

			try {
				list<EventReceiver*> &receivers = s.m_list.at(eventName);
				receivers.remove( receiver );
			} catch (std::out_of_range ex) {
			}
		}

		void EventSwitch::raiseEvent(Event *evt)
		{
			EventSwitch &s = EventSwitch::getInstance();

			s.direct(evt);
			delete evt;
		}

		void EventSwitch::direct(const Event *evt)
		{
			try {
				// get the list for this event
				const list<EventReceiver*> receivers = m_list.at(evt->getName());
				list<EventReceiver*>::const_iterator iter = receivers.begin();

				while (iter != receivers.end())
				{
					(*iter)->raiseEvent(evt);
					iter++;
				}
			} catch (std::out_of_range ex) {
				// No receiver available!
			}
		}

		EventSwitch& EventSwitch::getInstance()
		{
			static EventSwitch instance;
			//if (!instance.isRunning()) instance.start();
			return instance;
		}
	}
}

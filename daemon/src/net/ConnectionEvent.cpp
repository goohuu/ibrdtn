/*
 * ConnectionEvent.cpp
 *
 *  Created on: 16.02.2010
 *      Author: morgenro
 */

#include "net/ConnectionEvent.h"

namespace dtn
{
	namespace net
	{
		ConnectionEvent::ConnectionEvent(State s, const dtn::data::EID &p)
		 : state(s), peer(p)
		{

		}

		ConnectionEvent::~ConnectionEvent()
		{

		}

		void ConnectionEvent::raise(State s, const dtn::data::EID &p)
		{
			// raise the new event
			dtn::core::Event::raiseEvent( new ConnectionEvent(s, p) );
		}

		const string ConnectionEvent::getName() const
		{
			return ConnectionEvent::className;
		}


		std::string ConnectionEvent::toString() const
		{
			switch (state)
			{
			case CONNECTION_UP:
				return className + ": connection up " + peer.getString();
			case CONNECTION_DOWN:
				return className + ": connection down " + peer.getString();
			case CONNECTION_TIMEOUT:
				return className + ": connection timeout " + peer.getString();
			case CONNECTION_SETUP:
				return className + ": connection setup " + peer.getString();
			}

		}

		const string ConnectionEvent::className = "ConnectionEvent";
	}
}

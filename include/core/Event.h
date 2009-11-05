/*
 * Event.h
 *
 *  Created on: 05.03.2009
 *      Author: morgenro
 */

#ifndef EVENT_H_
#define EVENT_H_

#include "ibrdtn/default.h"

namespace dtn
{
	namespace core
	{
		enum EventType {
			EVENT_ASYNC = 0,
			EVENT_SYNC = 1
		};

		class Event
		{
		public:
			virtual ~Event() {};
			virtual const string getName() const = 0;
			virtual const EventType getType() const = 0;

#ifdef DO_DEBUG_OUTPUT
			virtual string toString() = 0;
#endif
		};
	}
}

#endif /* EVENT_H_ */

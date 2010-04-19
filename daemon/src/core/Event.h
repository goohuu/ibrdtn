/*
 * Event.h
 *
 *  Created on: 05.03.2009
 *      Author: morgenro
 */

#ifndef EVENT_H_
#define EVENT_H_

#include "config.h"
#include "core/EventReceiver.h"

namespace dtn
{
	namespace core
	{
		class Event
		{
		public:
			virtual ~Event() = 0;
			virtual const std::string getName() const = 0;

#ifdef DO_DEBUG_OUTPUT
			virtual std::string toString() const = 0;
#endif

		protected:
			static void raiseEvent(Event *evt);
		};
	}
}

#endif /* EVENT_H_ */

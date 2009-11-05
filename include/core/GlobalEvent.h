/*
 * GlobalEvent.h
 *
 *  Created on: 30.10.2009
 *      Author: morgenro
 */

#ifndef GLOBALEVENT_H_
#define GLOBALEVENT_H_

#include "ibrdtn/default.h"
#include "core/Event.h"

namespace dtn
{
	namespace core
	{
		class GlobalEvent : public Event
		{
		public:
			enum Action
			{
				GLOBAL_SHUTDOWN = 0
			};

			GlobalEvent(Action a);
			virtual ~GlobalEvent();

			const string getName() const;
			const EventType getType() const;

			const Action getAction() const;

#ifdef DO_DEBUG_OUTPUT
			string toString();
#endif

			static const string className;

		private:
			Action _action;
		};
	}
}

#endif /* GLOBALEVENT_H_ */

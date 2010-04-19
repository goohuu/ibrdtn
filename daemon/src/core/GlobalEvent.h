/*
 * GlobalEvent.h
 *
 *  Created on: 30.10.2009
 *      Author: morgenro
 */

#ifndef GLOBALEVENT_H_
#define GLOBALEVENT_H_

#include "config.h"
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
				GLOBAL_SHUTDOWN = 0,
				GLOBAL_RELOAD = 1
			};

			virtual ~GlobalEvent();

			const std::string getName() const;

			const Action getAction() const;

			static void raise(const Action a);

#ifdef DO_DEBUG_OUTPUT
			std::string toString() const;
#endif

			static const std::string className;

		private:
			GlobalEvent(const Action a);
			Action _action;
		};
	}
}

#endif /* GLOBALEVENT_H_ */

/*
 * Clock.h
 *
 *  Created on: 17.07.2009
 *      Author: morgenro
 */

#ifndef WALLCLOCK_H_
#define WALLCLOCK_H_

#include <ibrcommon/thread/Thread.h>
#include <ibrcommon/thread/WaitForConditional.h>
#include "Component.h"

namespace dtn
{
	namespace core
	{
		class WallClock : public ibrcommon::WaitForConditional, public dtn::daemon::IndependentComponent
		{
		public:
			/**
			 * Constructor for the global Clock
			 * @param frequency Specify the frequency for the clock tick in seconds.
			 */
			WallClock(size_t frequency);
			virtual ~WallClock();

			/**
			 * Blocks until the next clock tick happens.
			 */
			void sync();

		protected:
			virtual void componentUp();
			virtual void componentRun();
			virtual void componentDown();

		private:
			size_t _frequency;
			size_t _next;
			bool _running;

		};
	}
}

#endif /* WALLCLOCK_H_ */

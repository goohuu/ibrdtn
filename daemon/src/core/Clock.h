/*
 * Clock.h
 *
 *  Created on: 17.07.2009
 *      Author: morgenro
 */

#ifndef CLOCK_H_
#define CLOCK_H_

#include <ibrcommon/thread/Thread.h>
#include <ibrcommon/thread/WaitForConditional.h>
#include "Component.h"

namespace dtn
{
	namespace core
	{
		class Clock : public ibrcommon::WaitForConditional, public dtn::daemon::IndependentComponent
		{
		public:
			/**
			 * Constructor for the global Clock
			 * @param frequency Specify the frequency for the clock tick in seconds.
			 */
			Clock(size_t frequency);
			virtual ~Clock();

			/**
			 * Blocks until the next clock tick happens.
			 */
			void sync();

			/**
			 * @return the DTN time in seconds since year 2000
			 */
			static size_t getTime();

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

#endif /* CLOCK_H_ */

/*
 * TimerCallback.h
 *
 *  Created on: 29.09.2009
 *      Author: morgenro
 */

#ifndef TIMERCALLBACK_H_
#define TIMERCALLBACK_H_

#include "ibrdtn/default.h"


namespace dtn
{
	namespace utils
	{
		class Timer;

		class TimerCallback
		{
		public:
			/**
			 * This method will be called if the timer timed out.
			 * @param timer The reference to the timer which timed out.
			 * @return True, if the timer should be reset and start again.
			 */
			virtual bool timeout(Timer *timer) = 0;
		};
	}
}

#endif /* TIMERCALLBACK_H_ */

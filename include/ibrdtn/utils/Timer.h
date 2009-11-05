/*
 * Timer.h
 *
 *  Created on: 29.09.2009
 *      Author: morgenro
 */

#ifndef TIMER_H_
#define TIMER_H_

#include "ibrdtn/default.h"
#include "ibrdtn/utils/TimerCallback.h"
#include "ibrdtn/utils/Thread.h"
#include "ibrdtn/utils/Mutex.h"

namespace dtn
{
	namespace utils
	{
		class Timer : public JoinableThread
		{
		public:
			/**
			 * Constructor for this timer
			 * @param timeout Timeout in seconds
			 */
			Timer(TimerCallback &callback, size_t timeout);

			virtual ~Timer();

			void set(size_t timeout);

			/**
			 * This method resets the timer.
			 */
			void reset();

			void stop();

		protected:
			void run();

		private:
			TimerCallback &_callback;
			bool _running;
			size_t _countdown;
			size_t _timeout;
			dtn::utils::Mutex _writelock;
		};
	}
}

#endif /* TIMER_H_ */

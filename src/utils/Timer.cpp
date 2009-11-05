/*
 * Timer.cpp
 *
 *  Created on: 29.09.2009
 *      Author: morgenro
 */

#include "ibrdtn/utils/Timer.h"
#include "ibrdtn/utils/MutexLock.h"

namespace dtn
{
	namespace utils
	{
		Timer::Timer(TimerCallback &callback, size_t timeout)
		 : _callback(callback), _running(true), _timeout(timeout), _countdown(timeout)
		{
		}

		Timer::~Timer()
		{
			stop();
		}

		void Timer::stop()
		{
			_running = false;
			join();
		}

		void Timer::set(size_t timeout)
		{
			MutexLock l(_writelock);
			_timeout = timeout;
			_countdown = timeout;
		}

		void Timer::reset()
		{
			MutexLock l(_writelock);
			_countdown = _timeout;
		}

		void Timer::run()
		{
			if (_timeout == 0) return;

			while (_running)
			{
				sleep(1000);
				yield();

				if (_countdown == 0)
				{
					if (_callback.timeout(this))
					{
						reset();
					}
					else
					{
						return;
					}
				}

				MutexLock l(_writelock);
				_countdown--;
			}
		}
	}
}

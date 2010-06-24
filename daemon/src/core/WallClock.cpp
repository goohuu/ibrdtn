/*
 * Clock.cpp
 *
 *  Created on: 17.07.2009
 *      Author: morgenro
 */

#include "core/WallClock.h"
#include "core/TimeEvent.h"
#include <ibrdtn/utils/Clock.h>

namespace dtn
{
	namespace core
	{
		WallClock::WallClock(size_t frequency) : _frequency(frequency), _next(0), _running(false)
		{
		}

		WallClock::~WallClock()
		{
			if (isRunning())
			{
				componentDown();
			}
		}

		void WallClock::sync()
		{
			wait();
		}

		void WallClock::componentUp()
		{
		}

		void WallClock::componentDown()
		{
			_running = false;
			join();
		}

		void WallClock::componentRun()
		{
			_running = true;

			while (_running)
			{
				size_t dtntime = dtn::utils::Clock::getTime();
				if (_next <= dtntime)
				{
					TimeEvent::raise(dtntime, TIME_SECOND_TICK);
					go();
					_next = dtntime + _frequency;
				}

				this->sleep(500);
				yield();
			}
		}
	}
}

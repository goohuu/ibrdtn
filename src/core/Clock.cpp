/*
 * Clock.cpp
 *
 *  Created on: 17.07.2009
 *      Author: morgenro
 */

#include "core/Clock.h"

#include "core/EventSwitch.h"
#include "core/TimeEvent.h"

#include <math.h>
#include <time.h>

namespace dtn
{
	namespace core
	{
		Clock::Clock(size_t frequency) : _frequency(frequency), _next(0)
		{
			_running = true;
		}

		Clock::~Clock()
		{
			_running = false;
			join();
		}

		void Clock::sync()
		{
			wait();
		}

		size_t Clock::getTime()
		{
			time_t rawtime = time(NULL);
			tm * ptm;

			ptm = gmtime ( &rawtime );

			return mktime(ptm) - 946681200;
		}

		void Clock::run()
		{
			while (_running)
			{
				size_t dtntime = getTime();
				if (_next <= dtntime)
				{
					EventSwitch::raiseEvent( new TimeEvent(dtntime, TIME_SECOND_TICK) );
					go();
					_next = dtntime + _frequency;
				}

				this->sleep(500);
				yield();
			}
		}
	}
}

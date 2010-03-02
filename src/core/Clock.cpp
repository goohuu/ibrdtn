/*
 * Clock.cpp
 *
 *  Created on: 17.07.2009
 *      Author: morgenro
 */

#include "core/Clock.h"
#include "core/TimeEvent.h"
#include "ibrdtn/utils/Utils.h"

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
			return dtn::utils::Utils::get_current_dtn_time();
		}

		void Clock::run()
		{
			while (_running)
			{
				size_t dtntime = getTime();
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

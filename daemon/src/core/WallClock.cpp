/*
 * Clock.cpp
 *
 *  Created on: 17.07.2009
 *      Author: morgenro
 */

#include "core/WallClock.h"
#include "core/TimeEvent.h"
#include <ibrdtn/utils/Clock.h>
#include <ibrcommon/thread/MutexLock.h>

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
			try {
				ibrcommon::MutexLock l(*this);
				wait();
			} catch (const ibrcommon::Conditional::ConditionalAbortException &ex) {

			}
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

				if (dtntime == 0)
				{
					ibrcommon::MutexLock l(*this);
					TimeEvent::raise(dtntime, TIME_SECOND_TICK);
					signal(true);
				}
				else if (_next <= dtntime)
				{
					ibrcommon::MutexLock l(*this);
					TimeEvent::raise(dtntime, TIME_SECOND_TICK);
					signal(true);
					_next = dtntime + _frequency;
				}

				this->sleep(500);
				yield();
			}
		}
	}
}

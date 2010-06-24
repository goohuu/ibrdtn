/*
 * Clock.h
 *
 *  Created on: 24.06.2010
 *      Author: morgenro
 */

#ifndef CLOCK_H_
#define CLOCK_H_

#include <sys/types.h>

namespace dtn
{
	namespace utils
	{
		class Clock
		{
		public:
			Clock();
			virtual ~Clock();

			static size_t getTime();

			static bool isExpired(size_t timestamp, size_t lifetime = 0);

			static size_t getExpireTime(size_t timestamp, size_t lifetime);

			static int timezone;

			/**
			 * Defines an estimation about the precision of the local time. If the clock is definitely wrong
			 * the value is zero and one when we have a perfect time sync. Everything between one and zero gives
			 * an abstract knowledge about the quality of time.
			 */
			static float quality;
		};
	}
}

#endif /* CLOCK_H_ */

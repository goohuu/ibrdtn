/*
 * Clock.h
 *
 *  Created on: 24.06.2010
 *      Author: morgenro
 */

#ifndef CLOCK_H_
#define CLOCK_H_

#include <sys/types.h>
#include "ibrdtn/data/Bundle.h"

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

			static bool isExpired(const dtn::data::Bundle &b);

			/**
			 * This method is deprecated because it does not recognize the AgeBlock
			 * as alternative age verification.
			 */
			static bool isExpired(size_t timestamp, size_t lifetime = 0) __attribute__ ((deprecated));

			static size_t getExpireTime(const dtn::data::Bundle &b);

			/**
			 * This method is deprecated because it does not recognize the AgeBlock
			 * as alternative age verification.
			 */
			static size_t getExpireTime(size_t timestamp, size_t lifetime) __attribute__ ((deprecated));

			static int timezone;

			static u_int32_t TIMEVAL_CONVERSION;

			/**
			 * Defines an estimation about the precision of the local time. If the clock is definitely wrong
			 * the value is zero and one when we have a perfect time sync. Everything between one and zero gives
			 * an abstract knowledge about the quality of time.
			 */
			static float quality;

			/**
			 * If set to true, all time based functions assume a bad clock and try to use other mechanisms
			 * to detect expiration.
			 */
			static bool badclock;

		private:
			static bool __isExpired(size_t timestamp, size_t lifetime = 0);
			static size_t __getExpireTime(size_t timestamp, size_t lifetime);

		};
	}
}

#endif /* CLOCK_H_ */

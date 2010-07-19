/*
 * Clock.cpp
 *
 *  Created on: 24.06.2010
 *      Author: morgenro
 */

#include "ibrdtn/utils/Clock.h"
#include <sys/time.h>

namespace dtn
{
	namespace utils
	{
		int Clock::timezone = 0;
		float Clock::quality = 0;

		/**
		 * The number of seconds between 1/1/1970 and 1/1/2000.
		 */
		u_int32_t Clock::TIMEVAL_CONVERSION = 946684800;

		Clock::Clock()
		{
		}

		Clock::~Clock()
		{
		}

		size_t Clock::getExpireTime(size_t timestamp, size_t lifetime)
		{
			// if the quality of time is zero, return standard expire time
			if (Clock::quality == 0) return timestamp + lifetime;

			// calculate sigma based on the quality of time and the original lifetime
			size_t sigma = lifetime * (1 - Clock::quality);

			// expiration adjusted by quality of time
			return timestamp + lifetime + sigma;
		}

		bool Clock::isExpired(size_t timestamp, size_t lifetime)
		{
			// if the quality of time is zero, then never expire a bundle
			if (Clock::quality == 0) return false;

			if (lifetime > 0)
			{
				// calculate sigma based on the quality of time and the original lifetime
				size_t sigma = lifetime * (1 - Clock::quality);

				// expiration adjusted by quality of time
				if ( Clock::getTime() > (timestamp + sigma)) return true;
			}
			else
			{
				// expiration based on the timestamp only
				if ( Clock::getTime() > timestamp) return true;
			}

			return false;
		}

		size_t Clock::getTime()
		{
			struct timeval now;
			::gettimeofday(&now, 0);

			// timezone
			int offset = Clock::timezone * 3600;

			// do we believe we are before the year 2000?
			if ((u_int)now.tv_sec < TIMEVAL_CONVERSION)
			{
				return 0;
			}

			return (now.tv_sec - TIMEVAL_CONVERSION) + offset;
		}
	}
}

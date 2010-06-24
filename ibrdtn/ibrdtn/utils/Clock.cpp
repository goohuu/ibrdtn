/*
 * Clock.cpp
 *
 *  Created on: 24.06.2010
 *      Author: morgenro
 */

#include "ibrdtn/utils/Clock.h"
#include <time.h>

namespace dtn
{
	namespace utils
	{
		int Clock::timezone = 0;
		float Clock::quality = 0;

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
			time_t rawtime = time(NULL);
			tm * ptm;

			ptm = gmtime ( &rawtime );

			// timezone
			int offset = Clock::timezone * 3600;

			// current local UNIX timestamp
			time_t utime = mktime(ptm);

			// do we believe we are before the year 2000?
			if (utime < 946681200)
			{
				return 0;
			}

			return (utime - 946681200) + offset;
		}
	}
}

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

		Clock::Clock()
		{
		}

		Clock::~Clock()
		{
		}

		size_t Clock::getTime()
		{
			time_t rawtime = time(NULL);
			tm * ptm;

			ptm = gmtime ( &rawtime );

			// timezone
			int offset = Clock::timezone * 3600;

			return (mktime(ptm) - 946681200) + offset;
		}
	}
}

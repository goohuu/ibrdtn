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

			static int timezone;
			static float quality;
		};
	}
}

#endif /* CLOCK_H_ */

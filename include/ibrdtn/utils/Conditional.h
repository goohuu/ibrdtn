#ifndef CONDITIONAL_H_
#define CONDITIONAL_H_

#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include "ibrdtn/utils/Mutex.h"

namespace dtn
{
	namespace utils
	{
		class Conditional : public dtn::utils::Mutex
		{
			public:
				Conditional();
				virtual ~Conditional();

				void signal (bool broadcast = false);
				void wait();
				bool wait(size_t timeout);
				bool wait(struct timespec *ts);

			protected:
				/**
				 * Convert a millisecond timeout into use for high resolution
				 * conditional timers.
				 * @param timeout to convert.
				 * @param hires timespec representation to fill.
				 */
				static void gettimeout(size_t timeout, struct timespec *hires);

			private:
				class attribute
				{
				public:
					pthread_condattr_t attr;
					attribute();
				};

				pthread_cond_t cond;

				static attribute attr;
		};
	}
}

#endif /*CONDITIONAL_H_*/

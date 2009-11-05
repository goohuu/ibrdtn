#ifndef SEMAPHORE_H_
#define SEMAPHORE_H_

#include "ibrdtn/config.h"
#include <semaphore.h>

namespace dtn
{
	namespace utils
	{
		class Semaphore
		{
			public:
				Semaphore(unsigned int value = 0);
				virtual ~Semaphore();

				void wait();
				void post();

			private:
				sem_t count_sem;
		};
	}
}
#endif /*SEMAPHORE_H_*/

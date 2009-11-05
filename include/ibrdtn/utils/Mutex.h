#ifndef MUTEX_H_
#define MUTEX_H_

#include "ibrdtn/config.h"
#include "pthread.h"

namespace dtn
{
	namespace utils
	{
		class Mutex
		{
			public:
				Mutex();
				virtual ~Mutex();

				void enter();
				void leave();

			protected:
				pthread_mutex_t m_mutex;
		};
	}
}

#endif /*MUTEX_H_*/

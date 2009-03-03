#ifndef MUTEX_H_
#define MUTEX_H_

#include "config.h"

#ifdef HAVE_LIBCOMMONCPP
#include <cc++/thread.h>
#else
#include "pthread.h"
#endif

namespace dtn
{
	namespace utils
	{
	#ifdef HAVE_LIBCOMMONCPP
		class Mutex : public ost::Mutex
		{
			public:
				Mutex();
				~Mutex();

				void lock();
				void unlock();
		};
	#else
		class Mutex
		{
			public:
				Mutex();
				~Mutex();

				bool tryLock();
				void lock();
				void unlock();
				void enterMutex();
				void leaveMutex();

			private:
				pthread_mutex_t *m_mutex;
		};
	#endif
	}
}

#endif /*MUTEX_H_*/

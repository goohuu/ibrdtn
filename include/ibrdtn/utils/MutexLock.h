#ifndef MUTEXLOCK_H_
#define MUTEXLOCK_H_

#include "ibrdtn/config.h"
#include "ibrdtn/utils/Mutex.h"

namespace dtn
{
	namespace utils
	{
		class MutexLock
		{
			public:
				MutexLock(Mutex &m);
				virtual ~MutexLock();

			private:
				Mutex &m_mutex;
		};
	}
};

#endif /*MUTEXLOCK_H_*/

#ifndef MUTEXLOCK_H_
#define MUTEXLOCK_H_

#include "config.h"

#include "Mutex.h"

#ifdef HAVE_LIBCOMMONCPP
#include <cc++/thread.h>
#endif

namespace dtn
{
namespace utils
{
#ifdef HAVE_LIBCOMMONCPP
	class MutexLock : public ost::MutexLock
	{
		public:
			MutexLock(dtn::Mutex &m);
			~MutexLock();
	};
#else
	class MutexLock
	{
		public:
			MutexLock(Mutex &m);
			~MutexLock();

			void unlock();

			private:
			Mutex &m_mutex;
			bool m_locked;
	};
#endif
}
};

#endif /*MUTEXLOCK_H_*/

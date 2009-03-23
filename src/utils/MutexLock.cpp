#include "utils/MutexLock.h"

namespace dtn
{
	namespace utils
	{
	#ifdef HAVE_LIBCOMMONCPP
		MutexLock::MutexLock(dtn::Mutex &m) : ost::MutexLock(m)
		{}

		MutexLock::~MutexLock()
		{}
	#else
		MutexLock::MutexLock(Mutex &m)
			: m_mutex(m), m_locked(true)
		{
			m_mutex.enterMutex();
		}

		MutexLock::~MutexLock()
		{
			if (m_locked) m_mutex.leaveMutex();
		}

		void MutexLock::unlock()
		{
			if (m_locked)
			{
				m_locked = false;
				m_mutex.leaveMutex();
			}
		}
	#endif
	}
}

#include "ibrdtn/utils/MutexLock.h"

namespace dtn
{
	namespace utils
	{
		MutexLock::MutexLock(Mutex &m) : m_mutex(m)
		{
			m_mutex.enter();
		}

		MutexLock::~MutexLock()
		{
			m_mutex.leave();
		}
	}
}

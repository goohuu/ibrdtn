#include "utils/Mutex.h"

namespace dtn
{
	namespace utils
	{
	#ifdef HAVE_LIBCOMMONCPP
		Mutex::Mutex() : ost::Mutex() {}
		Mutex::~Mutex() {}
	#else
		Mutex::Mutex()
		{
			m_mutex = new pthread_mutex_t();
			pthread_mutex_init(m_mutex, NULL);
		}

		Mutex::~Mutex()
		{
			pthread_mutex_destroy( m_mutex );
			delete m_mutex;
		}

		bool Mutex::tryLock()
		{
			if ( pthread_mutex_trylock( m_mutex ) < 0 )
			{
				return false;
			}
			else
			{
				return true;
			}
		}

		void Mutex::enterMutex()
		{
			pthread_mutex_lock( m_mutex );
		}

		void Mutex::leaveMutex()
		{
			pthread_mutex_unlock( m_mutex );
		}

	#endif

		void Mutex::lock()
		{
			enterMutex();
		}

		void Mutex::unlock()
		{
			leaveMutex();
		}
	}
}

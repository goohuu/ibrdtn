#include "ibrdtn/utils/Mutex.h"

namespace dtn
{
	namespace utils
	{
		Mutex::Mutex()
		{
			pthread_mutex_init(&m_mutex, NULL);
		}

		Mutex::~Mutex()
		{
			pthread_mutex_destroy( &m_mutex );
		}

		void Mutex::enter()
		{
			pthread_mutex_lock( &m_mutex );
		}

		void Mutex::leave()
		{
			pthread_mutex_unlock( &m_mutex );
		}
	}
}

#include "utils/Conditional.h"

namespace dtn
{
namespace utils
{
#ifdef HAVE_LIBCOMMONCPP
	Conditional::Conditional() : ost::Conditional()
	{}

	Conditional::~Conditional()
	{}
#else
	Conditional::Conditional()
	{
		m_mutex = new pthread_mutex_t();
		pthread_mutex_init(m_mutex, NULL);

		m_cond = new pthread_cond_t();
		pthread_cond_init(m_cond, NULL);
	}

	Conditional::~Conditional()
	{
		pthread_mutex_lock( m_mutex );
		pthread_cond_destroy(m_cond);
		delete m_cond;
		pthread_mutex_unlock( m_mutex );
		pthread_mutex_destroy( m_mutex );
		delete m_mutex;
	}

	void Conditional::signal (bool broadcast)
	{
		pthread_mutex_lock( m_mutex );

		if (broadcast)
			pthread_cond_broadcast( m_cond );
		else
			pthread_cond_signal( m_cond );

		pthread_mutex_unlock( m_mutex );
	}

	void Conditional::wait ()
	{
		pthread_mutex_lock( m_mutex );
		pthread_cond_wait( m_cond, m_mutex );
		pthread_mutex_unlock( m_mutex );
	}
#endif
}
}


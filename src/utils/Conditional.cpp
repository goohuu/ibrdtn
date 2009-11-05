#include "ibrdtn/utils/Conditional.h"
#include "ibrdtn/utils/Thread.h"
#include <sys/time.h>

namespace dtn
{
	namespace utils
	{
		Conditional::Conditional()
		{
			pthread_cond_init(&cond, &attr.attr);
		}

		Conditional::~Conditional()
		{
			signal(true);
			pthread_cond_destroy(&cond);
		}

		void Conditional::signal (bool broadcast)
		{
			if (broadcast)
				pthread_cond_broadcast( &cond );
			else
				pthread_cond_signal( &cond );
		}

		void Conditional::wait()
		{
			pthread_cond_wait( &cond, &m_mutex );
		}

		bool Conditional::wait(size_t timeout)
		{
			struct timespec ts;
			gettimeout(timeout, &ts);
			return wait(&ts);
		}

		bool Conditional::wait(struct timespec *ts)
		{
			if(pthread_cond_timedwait(&cond, &m_mutex, ts) == ETIMEDOUT)
			{
				return false;
			}

			return true;
		}

		Conditional::attribute Conditional::attr;

		Conditional::attribute::attribute()
		{
			pthread_condattr_init(&attr);
		}

		void Conditional::gettimeout(size_t msec, struct timespec *ts)
		{
		#if defined(HAVE_PTHREAD_CONDATTR_SETCLOCK) && defined(_POSIX_MONOTONIC_CLOCK)
			clock_gettime(CLOCK_MONOTONIC, ts);
		#elif _POSIX_TIMERS > 0
			clock_gettime(CLOCK_REALTIME, ts);
		#else
			timeval tv;
			::gettimeofday(&tv, NULL);
			ts->tv_sec = tv.tv_sec;
			ts->tv_nsec = tv.tv_usec * 1000l;
		#endif
			ts->tv_sec += msec / 1000;
			ts->tv_nsec += (msec % 1000) * 1000000l;
			while(ts->tv_nsec > 1000000000l) {
				++ts->tv_sec;
				ts->tv_nsec -= 1000000000l;
			}
		}
	}
}


#ifndef CONDITIONAL_H_
#define CONDITIONAL_H_

#include "config.h"

#ifdef HAVE_LIBCOMMONCPP
#include <cc++/thread.h>
#else
#include <pthread.h>
#endif

namespace dtn
{
namespace utils
{
#ifdef HAVE_LIBCOMMONCPP
	class Conditional : public ost::Conditional
	{
		public:
			Conditional();
			~Conditional();
	};
#else
	class Conditional
	{
		public:
			Conditional();
			~Conditional();

			void signal (bool broadcast = false);
			void wait ();

		private:
			pthread_cond_t *m_cond;
			pthread_mutex_t *m_mutex;
	};
#endif
}
}

#endif /*CONDITIONAL_H_*/

#ifndef SEMAPHORE_H_
#define SEMAPHORE_H_

#ifdef HAVE_LIBCOMMONCPP
#include <cc++/thread.h>
#else
#include "semaphore.h"
#endif

namespace dtn
{
namespace utils
{
#ifdef HAVE_LIBCOMMONCPP
	class Semaphore : public ost::Semaphore
	{
		public:
			Semaphore(unsigned int value = 0);
			~Semaphore();
	};
#else
	class Semaphore
	{
		public:
			Semaphore(unsigned int value = 0);
			~Semaphore();

			void wait();
			void post();

		private:
			sem_t count_sem;
	};
#endif
}
}
#endif /*SEMAPHORE_H_*/

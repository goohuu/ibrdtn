#include "utils/Semaphore.h"
#include <iostream>

using namespace std;

namespace dtn
{
namespace utils
{
#ifdef HAVE_LIBCOMMONCPP
	Semaphore::Semaphore(unsigned int value) : ost::Semaphore(value) {}
	Semaphore::~Semaphore() {}
#else
	Semaphore::Semaphore(unsigned int value)
	{
		sem_init(&count_sem, 0, value);

	}

	Semaphore::~Semaphore()
	{
		sem_destroy(&count_sem);
	}

	void Semaphore::wait()
	{
		sem_wait(&count_sem);
	}

	void Semaphore::post()
	{
		sem_post(&count_sem);
	}
#endif
}
}

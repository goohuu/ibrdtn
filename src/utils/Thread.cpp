/*
 * Thread.cpp
 *
 *  Created on: 30.07.2009
 *      Author: morgenro
 */

#include "ibrdtn/utils/Thread.h"
#include "pthread.h"
#include <sys/times.h>
#include <stdio.h>
#include <unistd.h>


namespace dtn
{
	namespace utils
	{
		static void *exec_thread(void *obj)
		{
			if (obj == NULL) return NULL;

			Thread *th = static_cast<Thread *>(obj);
			th->setPriority();
			th->run();
			th->exit();
			return NULL;
		}

		Thread::Thread(size_t size)
		{
			stack = size;
			priority = 0;
		}

		void Thread::setPriority(void)
		{};

		void Thread::yield(void)
		{
		#if defined(HAVE_PTHREAD_YIELD_NP)
			pthread_yield_np();
		#elif defined(HAVE_PTHREAD_YIELD)
			pthread_yield();
		#else
			sched_yield();
		#endif
		}

		void Thread::concurrency(int level)
		{
		#if defined(HAVE_PTHREAD_SETCONCURRENCY)
			pthread_setconcurrency(level);
		#endif
		}

		void Thread::policy(int polid)
		{
		}

		void Thread::sleep(size_t timeout)
		{
			timespec ts;
			ts.tv_sec = timeout / 1000l;
			ts.tv_nsec = (timeout % 1000l) * 1000000l;
		#if defined(HAVE_PTHREAD_DELAY)
			pthread_delay(&ts);
		#elif defined(HAVE_PTHREAD_DELAY_NP)
			pthread_delay_np(&ts);
		#else
			usleep(timeout * 1000);
		#endif
		}

		Thread::~Thread()
		{
		}

		void Thread::exit(void)
		{
			pthread_exit(NULL);
		}

		bool Thread::equal(pthread_t t1, pthread_t t2)
		{
			return t1 == t2;
		}

		JoinableThread::JoinableThread(size_t size)
		{
			running = false;
			stack = size;
		}

		JoinableThread::~JoinableThread()
		{
			join();
		}

		void JoinableThread::start(int adj)
		{
			int result;

			if(running)
				return;

			priority = adj;

			pthread_attr_t attr;
			pthread_attr_init(&attr);
			pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
			pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED);

		// we typically use "stack 1" for min stack...
		#ifdef	PTHREAD_STACK_MIN
			if(stack && stack < PTHREAD_STACK_MIN)
				stack = PTHREAD_STACK_MIN;
		#else
			if(stack && stack < 2)
				stack = 0;
		#endif

			if(stack)
				pthread_attr_setstacksize(&attr, stack);
			result = pthread_create(&tid, &attr, &exec_thread, this);
			pthread_attr_destroy(&attr);
			if(!result)
				running = true;
		}

		void JoinableThread::join(void)
		{
			pthread_t self = pthread_self();

			if(running && equal(tid, self))
				Thread::exit();

			// already joined, so we ignore...
			if(!running)
				return;

			if(!pthread_join(tid, NULL))
				running = false;
		}

		void JoinableThread::waitFor()
		{
			join();
		}
	}
}

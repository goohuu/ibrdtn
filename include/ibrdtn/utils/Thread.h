/*
 * Thread.h
 *
 *  Created on: 30.07.2009
 *      Author: morgenro
 */

#ifndef THREAD_H_
#define THREAD_H_

#include "ibrdtn/config.h"
#include "pthread.h"
#include <sys/types.h>

namespace dtn
{
	namespace utils
	{
		/**
		 * An abstract class for defining classes that operate as a thread.  A derived
		 * thread class has a run method that is invoked with the newly created
		 * thread context, and can use the derived object to store all member data
		 * that needs to be associated with that context.  This means the derived
		 * object can safely hold thread-specific data that is managed with the life
		 * of the object, rather than having to use the clumsy thread-specific data
		 * management and access functions found in thread support libraries.
		 * @author David Sugar <dyfet@gnutelephony.org>
		 */
		class Thread
		{
		protected:
			pthread_t tid;
			size_t stack;
			int priority;

			/**
			 * Create a thread object that will have a preset stack size.  If 0
			 * is used, then the stack size is os defined/default.
			 * @param stack size to use or 0 for default.
			 */
			Thread(size_t stack = 0);

		public:
			/**
			 * Set thread priority without disrupting scheduling if possible.
			 * Based on scheduling policy.  It is recommended that the process
			 * is set for realtime scheduling, and this method is actually for
			 * internal use.
			 */
			void setPriority(void);

			/**
			 * Yield execution context of the current thread. This is a static
			 * and may be used anywhere.
			 */
			static void yield(void);

			/**
			 * Sleep current thread for a specified time period.
			 * @param timeout to sleep for in milliseconds.
			 */
			static void sleep(size_t timeout);

			/**
			 * Abstract interface for thread context run method.
			 */
			virtual void run(void) = 0;

			/**
			 * Destroy thread object, thread-specific data, and execution context.
			 */
			virtual ~Thread();

			/**
			 * Exit the thread context.  This function should NO LONGER be called
			 * directly to exit a running thread.  Instead this method will only be
			 * used to modify the behavior of the thread context at thread exit,
			 * including detached threads which by default delete themselves.  This
			 * documented usage was changed to support Mozilla NSPR exit behavior
			 * in case we support NSPR as an alternate thread runtime in the future.
			 */
			virtual void exit(void);

			/**
			 * Used to specify scheduling policy for threads above priority "0".
			 * Normally we apply static realtime policy SCHED_FIFO (default) or
			 * SCHED_RR.  However, we could apply SCHED_OTHER, etc.
			 */
			static void policy(int polid);

			/**
			 * Set concurrency level of process.  This is essentially a portable
			 * wrapper for pthread_setconcurrency.
			 */
			static void concurrency(int level);

			/**
			 * Determine if two thread identifiers refer to the same thread.
			 * @param thread1 to test.
			 * @param thread2 to test.
			 * @return true if both are the same context.
			 */
			static bool equal(pthread_t thread1, pthread_t thread2);

			/**
			 * Get current thread id.
			 * @return thread id.
			 */
			inline static pthread_t self(void)
				{return pthread_self();};
		};

		/**
		 * A child thread object that may be joined by parent.  A child thread is
		 * a type of thread in which the parent thread (or process main thread) can
		 * then wait for the child thread to complete and then delete the child object.
		 * The parent thread can wait for the child thread to complete either by
		 * calling join, or performing a "delete" of the derived child object.  In
		 * either case the parent thread will suspend execution until the child thread
		 * exits.
		 * @author David Sugar <dyfet@gnutelephony.org>
		 */
		class JoinableThread : protected Thread
		{
		private:
			volatile bool running;

		protected:
			/**
			 * Create a joinable thread with a known context stack size.
			 * @param size of stack for thread context or 0 for default.
			 */
			JoinableThread(size_t size = 0);

			/**
			 * Delete child thread.  Parent thread suspends until child thread
			 * run method completes or child thread calls it's exit method.
			 */
			virtual ~JoinableThread();

			/**
			 * Join thread with parent.  Calling from a child thread to exit is
			 * now depreciated behavior and in the future will not be supported.
			 * Threads should always return through their run() method.
			 */
			void join(void);

		public:
			/**
			 * Test if thread is currently running.
			 * @return true while thread is running.
			 */
			inline bool isRunning(void)
				{return running;};

			/**
			 * Start execution of child context.  This must be called after the
			 * child object is created (perhaps with "new") and before it can be
			 * joined.  This method actually begins the new thread context, which
			 * then calls the object's run method.  Optionally raise the priority
			 * of the thread when it starts under realtime priority.
			 * @param priority of child thread.
			 */
			void start(int priority = 0);

			/**
			 * Start execution of child context as background thread.  This is
			 * assumed to be off main thread, with a priority lowered by one.
			 */
			inline void background(void)
				{start(-1);};

			void waitFor();
		};
	}
}

#endif /* THREAD_H_ */

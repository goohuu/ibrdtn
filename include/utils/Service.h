#ifndef SERVICE_H_
#define SERVICE_H_

#include "pthread.h"
#include <string>

/**
 * A child of the class Service is possible to run as a thread.
 *
 * The tick() method is called in a loop until the abort() method is called. To do
 * your own stuff you have to overload this method.
 *
 * Additionally the methods initialize() and terminate() could be overloaded to to things
 * before the first tick() call and do stuff after the last call of tick().
 */

using namespace std;

namespace dtn
{
	namespace utils
	{
		class Service
		{
			public:
				/**
				 * constructor with a name to identify the service or make debugging easier.
				 */
				Service(string name);

				/**
				 * destructor
				 */
				virtual ~Service();

				void start();
				void abort();
				void waitFor();
				bool isRunning();
				string getName();

			protected:
				/**
				 * overload this method to do your own stuff.
				 */
				virtual void tick() = 0;

				void finished();

				virtual void initialize()
				{};

				virtual void terminate()
				{};

			private:
				void run();
				static void * entryPoint(void*);

				bool m_running;
				bool m_started;

				pthread_t m_thread;
				string m_name;
		};
	}
}

#endif /*SERVICE_H_*/

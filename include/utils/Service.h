#ifndef SERVICE_H_
#define SERVICE_H_

#include "pthread.h"
#include <string>

/**
 * Wird eine Klasse von Service abgeleitet ist es möglich diese in einen Zyklus von
 * Ablaufen einzuordnen in denen jede Klasse nach einander agieren darf. Dieser Mechanismus
 * wird verwendet um auf Threads verzichten zu können.
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
				 * Service Konstruktor
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
				 * Führt einen tick() aus. Dieses Model ermöglicht einen getakteten Ablauf ohne
				 * Threads. Dabei wird von allen Service Klassen regelmäßig die Funktion tick()
				 * aufgerufen.
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

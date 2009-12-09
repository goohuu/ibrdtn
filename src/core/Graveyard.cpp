/*
 * Graveyard.cpp
 *
 *  Created on: 28.10.2009
 *      Author: morgenro
 */

#include "core/Graveyard.h"


namespace dtn
{
	namespace core
	{
		Graveyard::Zombie::Zombie() : _buried(false)
		{};

		Graveyard::Zombie::~Zombie()
		{};

		void Graveyard::Zombie::bury()
		{
			if (_buried) return;
			_buried = true;
			dtn::core::Graveyard::bury(this);
		}

		Graveyard::Graveyard()
		 : _running(false)
		{
		}

		Graveyard::~Graveyard()
		{
			// wait until all zombies are cleared
			while (!_zombies.empty()) sleep(100);

			_wait.enter();
			_running = false;
			_wait.signal(true);
			_wait.leave();
			join();
		}

		void Graveyard::bury(Zombie *z)
		{
			static Graveyard yard;
			static ibrcommon::Mutex m;

			ibrcommon::MutexLock l(m);

			// if not running, call the gravedigger
			if (!yard._running) yard.start();

			yard.newJob(z);
		}

		void Graveyard::newJob(Zombie *z)
		{
#ifdef DO_EXTENDED_DEBUG_OUTPUT
			cout << "Graveyard: new Zombie" << endl;
#endif
			ibrcommon::MutexLock l(_wait);
			_zombies.push(z);
			_wait.signal(true);
#ifdef DO_EXTENDED_DEBUG_OUTPUT
			cout << "Graveyard: new Zombie signaled" << endl;
#endif
		}

		bool Graveyard::wait()
		{
			ibrcommon::MutexLock l(_wait);
			if (!_zombies.empty()) return true;

			// wait for a new zombie
			_wait.wait();

			if (_zombies.empty())
			{
				return false;
			}

			return true;
		}


		void Graveyard::run()
		{
			_running = true;

			while (_running)
			{
				if (wait())
				{
					Zombie *z = _zombies.front();
					_zombies.pop();

					// embalm the zombie
					z->embalm();

#ifdef DO_EXTENDED_DEBUG_OUTPUT
					cout << "Graveyard: delete Zombie" << endl;
#endif

					// bury him
					delete z;

#ifdef DO_EXTENDED_DEBUG_OUTPUT
					cout << "Graveyard: Zombie deleted" << endl;
#endif
				}

				yield();
			}
		}
	}
}

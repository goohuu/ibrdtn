/*
 * GenericServer.h
 *
 *  Created on: 23.04.2010
 *      Author: morgenro
 */

#ifndef GENERICSERVER_H_
#define GENERICSERVER_H_

#include "Component.h"
#include <ibrcommon/thread/MutexLock.h>
#include <ibrcommon/thread/Thread.h>
#include <ibrcommon/thread/Mutex.h>
#include <ibrcommon/thread/Conditional.h>
#include <list>

namespace dtn
{
	namespace net
	{
		class GenericConnection
		{
		public:
			virtual ~GenericConnection() { };
			virtual void shutdown() = 0;
			virtual bool free() = 0;
		};

		template <class T>
		class GenericServer : public dtn::daemon::IndependentComponent
		{
		public:
			GenericServer()
			 : _running(false), _cleaner(*this, _clients)
			{ }

			virtual ~GenericServer()
			{ }

			void add(T *obj)
			{
				ibrcommon::MutexLock l(_cleaner);
				_clients.push_back(obj);
				connectionUp(obj);
			}

		protected:
			virtual T* accept() = 0;
			virtual void listen() = 0;
			virtual void shutdown() = 0;

			virtual void connectionUp(T *conn) = 0;
			virtual void connectionDown(T *conn) = 0;

			void shutdownAll()
			{
				ibrcommon::MutexLock l(_cleaner);

				std::list<GenericConnection*>::iterator iter = _clients.begin();
				while (iter != _clients.end())
				{
					if ( (*iter)->free() )
					{
						delete (*iter);
						_clients.erase(iter++);
					}
					else
					{
						(*iter)->shutdown();
						iter++;
					}
				}
			}

			void componentUp()
			{
				_cleaner.start();
				listen();
			}

			void componentRun()
			{
				_running = true;

				while (_running)
				{
					try {
						T* obj = accept();
						if (_running && (obj != NULL))
						{
							add( obj );
						}
					} catch (ibrcommon::Exception) {
						// ignore all errors
					} catch (std::exception) { }

					{
						ibrcommon::MutexLock l(_cleaner);
						_cleaner.signal(true);
					}

					// breakpoint
					ibrcommon::Thread::yield();
				}
			}

			void componentDown()
			{
				_running = false;
				shutdown();
			}

		private:
			class ClientCleaner : public ibrcommon::JoinableThread, public ibrcommon::Conditional
			{
			public:
				ClientCleaner(GenericServer &callback, std::list<GenericConnection*> &clients)
				 : _running(false), _clients(clients), _callback(callback)
				{
				}

				virtual ~ClientCleaner()
				{
					{
						ibrcommon::MutexLock l(*this);
						_running = false;
					}

					join();
				}

			protected:
				void run()
				{
					ibrcommon::MutexLock l(*this);
					_running = true;

					while (_running)
					{
						std::list<GenericConnection*>::iterator iter = _clients.begin();
						while (iter != _clients.end())
						{
							if ( (*iter)->free() )
							{
								_callback.__connectionDown(*iter);

								// free the object
								delete (*iter);

								_clients.erase(iter++);
							}
							else
							{
								iter++;
							}
						}

						ibrcommon::Thread::yield();
						(*this).wait(500); // wait max. 0.5 seconds
					}
				}

			private:
				bool _running;
				std::list<GenericConnection*> &_clients;
				GenericServer &_callback;
			};

			void __connectionDown(GenericConnection *conn)
			{
				T *tconn = dynamic_cast<T*>(conn);
				if (tconn != NULL)
					connectionDown(tconn);
			}

			std::list<GenericConnection*> _clients;
			bool _running;
			ClientCleaner _cleaner;
		};
	}
}

#endif /* GENERICSERVER_H_ */

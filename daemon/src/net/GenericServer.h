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
#include <ibrcommon/thread/ThreadSafeQueue.h>
#include <list>

namespace dtn
{
	namespace net
	{
		class GenericConnectionInterface
		{
		public:
			virtual ~GenericConnectionInterface() { };
			virtual void shutdown() = 0;
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

			void setFree(T *conn)
			{
				_cleaner.freequeue.push(conn);
			}

		protected:
			virtual T* accept() = 0;
			virtual void listen() = 0;
			virtual void shutdown() = 0;

			virtual void connectionUp(T *conn) = 0;
			virtual void connectionDown(T *conn) = 0;

			void componentUp()
			{
				_running = true;
				_cleaner.start();
				listen();
			}

			void componentRun()
			{
				while (_running)
				{
					try {
						T* obj = accept();
						if (_running && (obj != NULL))
						{
							add( obj );
						}
					} catch (std::exception) {
						// ignore all errors
					}

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
				_cleaner.stop();
			}

		private:
			class ClientCleaner : public ibrcommon::JoinableThread, public ibrcommon::Conditional
			{
			public:
				ClientCleaner(GenericServer &callback, std::list<T*> &clients)
				 : _running(true), _clients(clients), _callback(callback)
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

				ibrcommon::ThreadSafeQueue<T*> freequeue;

			protected:
				void finally()
				{
					ibrcommon::MutexLock l(*this);

					typename std::list< T* >::iterator iter = _clients.begin();
					while (iter != _clients.end())
					{
						delete (*iter);
						_clients.erase(iter++);
					}
				}

				void run()
				{
					try {
						while (_running)
						{
							T *conn = freequeue.blockingpop();
							bool delobj = false;

							// delete in list
							{
								ibrcommon::MutexLock l(*this);
								typename std::list<T* >::iterator iter = _clients.begin();
								while (iter != _clients.end())
								{
									if ( (*iter) == conn )
									{
										delobj = true;
										_clients.erase(iter++);
									}
									else
									{
										iter++;
									}
								}
							}

							if (delobj)
							{
								_callback.connectionDown(conn);
								delete conn;
							}

							ibrcommon::Thread::yield();
							ibrcommon::Thread::testcancel();
						}
					} catch (ibrcommon::Exception) {

					}
				}

			private:
				bool _running;
				std::list< T* > &_clients;
				GenericServer &_callback;
			};

			std::list< T* > _clients;
			bool _running;
			ClientCleaner _cleaner;
		};

		template <class T>
		class GenericConnection : public GenericConnectionInterface
		{
		public:
			GenericConnection(GenericServer<T> &server) : _server(server) { };
			virtual ~GenericConnection() { };

			void free()
			{
				T *obj = dynamic_cast<T*>(this);
				_server.setFree(obj);
			};

		private:
			GenericServer<T> &_server;
		};
	}
}

#endif /* GENERICSERVER_H_ */

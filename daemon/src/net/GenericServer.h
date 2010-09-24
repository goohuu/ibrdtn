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
#include <algorithm>

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
			{ }

			virtual ~GenericServer()
			{ }

			void add(T *obj)
			{
				{
					ibrcommon::MutexLock l(_lock);
					_clients.push_back(obj);
				}

				connectionUp(obj);
			}

			void remove(T *obj)
			{
				{
					ibrcommon::MutexLock l(_lock);
					_clients.erase( std::remove( _clients.begin(), _clients.end(), obj), _clients.end() );
				}

				connectionDown(obj);
			}

		protected:
			virtual T* accept() = 0;
			virtual void listen() = 0;
			virtual void shutdown() = 0;

			virtual void connectionUp(T *conn) = 0;
			virtual void connectionDown(T *conn) = 0;

			void componentUp()
			{
				listen();
			}

			void componentRun()
			{
				while (true)
				{
					try {
						T* obj = accept();
						if (obj != NULL)
						{
							add( obj );
						}
					} catch (std::exception) {
						// ignore all errors
					}

					// breakpoint
					ibrcommon::Thread::yield();
				}
			}

			void componentDown()
			{
				while (true)
				{
					T* client = NULL;
					{
						ibrcommon::MutexLock l(_lock);
						if (_clients.empty()) break;
						client = _clients.front();
						_clients.remove(client);
					}

					client->shutdown();
				}

				shutdown();
			}

			ibrcommon::Mutex _lock;
			std::list< T* > _clients;
		};

		template <class T>
		class GenericConnection : public GenericConnectionInterface
		{
		public:
			GenericConnection(GenericServer<T> &server) : _server(server) { };

		protected:
			virtual ~GenericConnection() { };
			GenericServer<T> &_server;
		};
	}
}

#endif /* GENERICSERVER_H_ */

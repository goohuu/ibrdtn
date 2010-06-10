/*
 * ApiServer.cpp
 *
 *  Created on: 24.06.2009
 *      Author: morgenro
 */

#include "ApiServer.h"
#include "core/BundleCore.h"
#include "core/EventSwitch.h"
#include "net/BundleReceivedEvent.h"
#include "ClientHandler.h"
#include <typeinfo>

using namespace dtn::data;
using namespace dtn::core;
using namespace dtn::streams;
using namespace std;

namespace dtn
{
	namespace daemon
	{
		ApiServer::ApiServer(ibrcommon::NetInterface net)
		 : dtn::net::GenericServer<ClientHandler>(), _tcpsrv(net), _dist(_connections, _connection_lock)
		{
			_dist.start();
		}

		ApiServer::~ApiServer()
		{
			if (isRunning())
			{
				componentDown();
			}
		}

		ClientHandler* ApiServer::accept()
		{
			try {
				// create a new ClientHandler
				ClientHandler *handler = new ClientHandler(_tcpsrv.accept());

				// start the ClientHandler (service)
				handler->start();

				return handler;
			} catch (ibrcommon::tcpserver::SocketException ex) {
				// socket is closed
				return NULL;
			}
		}

		void ApiServer::listen()
		{

		}

		void ApiServer::shutdown()
		{
			shutdownAll();
		}

		void ApiServer::connectionUp(ClientHandler *conn)
		{
			ibrcommon::MutexLock l(_connection_lock);
			_connections.push_back(conn);
		}

		void ApiServer::connectionDown(ClientHandler *conn)
		{
			ibrcommon::MutexLock l(_connection_lock);
			for (std::list<ClientHandler*>::iterator iter = _connections.begin(); iter != _connections.end(); iter++)
			{
				if (conn == (*iter))
				{
					_connections.erase(iter);
					return;
				}
			}
		}

		ApiServer::Distributor::Distributor(std::list<ClientHandler*> &connections, ibrcommon::Mutex &lock)
		 : _running(true), _connections(connections), _lock(lock)
		{
			bindEvent(dtn::net::BundleReceivedEvent::className);
		}

		ApiServer::Distributor::~Distributor()
		{
			unbindEvent(dtn::net::BundleReceivedEvent::className);
			shutdown();
		}

		/**
		 * @see ibrcommon::JoinableThread::run()
		 */
		void ApiServer::Distributor::run()
		{
			try
			{
				while (_running)
				{
					// get the next element in the queue
					dtn::routing::MetaBundle mb = _received.blockingpop();

					// search for the receiver of this bundle
					{
						ibrcommon::MutexLock l(_lock);

						for (std::list<ClientHandler*>::iterator iter = _connections.begin(); iter != _connections.end(); iter++)
						{
							ClientHandler *handler = (*iter);
							if (handler->getPeer() == mb.destination)
							{
								try {
									BundleStorage &storage = BundleCore::getInstance().getStorage();
									dtn::data::Bundle bundle = storage.get( mb );

									// send the bundle
									(*handler) << bundle;

									storage.remove(bundle);
								} catch (dtn::core::BundleStorage::NoBundleFoundException ex) {
									std::cerr << "API: NoBundleFoundException" << std::endl;
								} catch (ibrcommon::IOException ex) {
									std::cerr << "API: IOException" << std::endl;
									handler->shutdown();
								} catch (dtn::InvalidDataException ex) {
									std::cerr << "API: InvalidDataException" << std::endl;
									handler->shutdown();
								} catch (...) {
									std::cerr << "unexpected API error!" << std::endl;
									handler->shutdown();
								}
							}
						}
					}
				}
			} catch (...) {
				std::cerr << "unexpected error or shutdown" << std::endl;
			}
		}

		/**
		 * Stop the running daemon
		 */
		void ApiServer::Distributor::shutdown()
		{
			{
				ibrcommon::MutexLock l(_received);
				_running = false;
				_received.signal(true);
			}
			join();
		}

		/**
		 * @see dtn::core::EventReceiver::raiseEvent()
		 */
		void ApiServer::Distributor::raiseEvent(const Event *evt)
		{
			try {
				const dtn::net::BundleReceivedEvent &received = dynamic_cast<const dtn::net::BundleReceivedEvent&>(*evt);
				_received.push(received.getBundle());
			} catch (std::bad_cast ex) {

			}
		}
	}
}

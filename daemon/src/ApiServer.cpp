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
			ibrcommon::MutexLock l(_received_cond);

			while (_running)
			{
				// wait for the next signal
				_received_cond.wait();

				// stop the thread on shutdown
				if (!_running) return;

				// no bundles available
				if (_received.empty()) continue;

				// get the next element in the queue
				dtn::routing::MetaBundle &mb = _received.front();

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
								(*handler) << bundle;
								storage.remove(bundle);
							} catch (dtn::exceptions::NoBundleFoundException ex) {

							} catch (dtn::exceptions::IOException ex) {
								handler->shutdown();
							} catch (dtn::exceptions::InvalidDataException ex) {
								handler->shutdown();
							} catch (dtn::exceptions::InvalidBundleData ex) {
								handler->shutdown();
							}
						}
					}
				}

				// delete the element
				_received.pop();
			}
		}

		/**
		 * Stop the running daemon
		 */
		void ApiServer::Distributor::shutdown()
		{
			{
				ibrcommon::MutexLock l(_received_cond);
				_running = false;
				_received_cond.signal(true);
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
				ibrcommon::MutexLock l(_received_cond);
				_received.push(received.getBundle());
				_received_cond.signal();
			} catch (std::bad_cast ex) {

			}
		}
	}
}

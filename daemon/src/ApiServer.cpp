/*
 * ApiServer.cpp
 *
 *  Created on: 24.06.2009
 *      Author: morgenro
 */

#include "ApiServer.h"
#include "core/BundleCore.h"
#include "core/EventSwitch.h"
#include "routing/QueueBundleEvent.h"
#include "ClientHandler.h"
#include <ibrcommon/Logger.h>
#include <typeinfo>
#include <algorithm>

using namespace dtn::data;
using namespace dtn::core;
using namespace dtn::streams;
using namespace std;

namespace dtn
{
	namespace daemon
	{
		ApiServer::ApiServer(ibrcommon::NetInterface net, int port)
		 : dtn::net::GenericServer<ClientHandler>(), _tcpsrv(net, port), _dist(_connections, _list_lock)
		{
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
				ClientHandler *handler = new ClientHandler((dtn::net::GenericServer<ClientHandler>&)*this, _tcpsrv.accept());

				// start the ClientHandler (service)
				handler->start();

				return handler;
			} catch (ibrcommon::SocketException ex) {
				// socket is closed
				return NULL;
			}
		}

		void ApiServer::listen()
		{
			_dist.start();
		}

		void ApiServer::shutdown()
		{
			_dist.stop();
		}

		void ApiServer::connectionUp(ClientHandler *conn)
		{
			ibrcommon::MutexLock l(_list_lock);
			_connections.push_back(conn);
			IBRCOMMON_LOGGER_DEBUG(5) << "Client connection up" << IBRCOMMON_LOGGER_ENDL;
		}

		void ApiServer::connectionDown(ClientHandler *conn)
		{
			ibrcommon::MutexLock l(_list_lock);
			_connections.erase( std::remove(_connections.begin(), _connections.end(), conn) );
			IBRCOMMON_LOGGER_DEBUG(5) << "Client connection down" << IBRCOMMON_LOGGER_ENDL;
		}

		ApiServer::Distributor::Distributor(std::list<ClientHandler*> &connections, ibrcommon::Mutex &list)
		 : _list_lock(list), _connections(connections)
		{
			bindEvent(dtn::routing::QueueBundleEvent::className);
		}

		ApiServer::Distributor::~Distributor()
		{
			unbindEvent(dtn::routing::QueueBundleEvent::className);
			join();
		}

		/**
		 * @see ibrcommon::JoinableThread::run()
		 */
		void ApiServer::Distributor::run()
		{
			try
			{
				while (true)
				{
					// get the next element in the queue
					dtn::data::MetaBundle mb = _received.blockingpop();

					ibrcommon::MutexLock l(_list_lock);

					// search for the receiver of this bundle
					std::queue<ClientHandler*> receivers;

					for (std::list<ClientHandler*>::iterator iter = _connections.begin(); iter != _connections.end(); iter++)
					{
						ClientHandler *handler = (*iter);
						if (handler->getPeer() == mb.destination)
						{
							receivers.push(handler);
						}
					}

					if (!receivers.empty())
					{
						try {
							BundleStorage &storage = BundleCore::getInstance().getStorage();
							dtn::data::Bundle bundle = storage.get( mb );

							while (!receivers.empty())
							{
								ClientHandler *handler = receivers.front();
								IBRCOMMON_LOGGER_DEBUG(5) << "Transfer bundle " << mb.toString() << " to client " << handler->getPeer().getString() << IBRCOMMON_LOGGER_ENDL;

								// send the bundle
								(*handler).queue(bundle);

								receivers.pop();
							}

							// bundle has been delivered, delete it
							storage.remove( mb );
						} catch (dtn::core::BundleStorage::NoBundleFoundException ex) {
							IBRCOMMON_LOGGER_DEBUG(10) << "API: NoBundleFoundException; BundleID: " << mb.toString() << IBRCOMMON_LOGGER_ENDL;
						}
					}
				}
			} catch (std::exception) {
				IBRCOMMON_LOGGER_DEBUG(10) << "unexpected error or shutdown" << IBRCOMMON_LOGGER_ENDL;
			}
		}

		/**
		 * @see dtn::core::EventReceiver::raiseEvent()
		 */
		void ApiServer::Distributor::raiseEvent(const Event *evt)
		{
			try {
				const dtn::routing::QueueBundleEvent &queued = dynamic_cast<const dtn::routing::QueueBundleEvent&>(*evt);
				_received.push(queued.bundle);
			} catch (std::bad_cast ex) {

			}
		}
	}
}

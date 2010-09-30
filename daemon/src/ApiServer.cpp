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
		 : dtn::net::GenericServer<ClientHandler>(), _tcpsrv(net, port), _dist(_clients, mutex())
		{
		}

		ApiServer::~ApiServer()
		{
			if (isRunning())
			{
				componentDown();
			}

			join();
		}

		ClientHandler* ApiServer::accept()
		{
			try {
				// create a new ClientHandler
				return new ClientHandler(*this, _tcpsrv.accept());
			} catch (ibrcommon::SocketException ex) {
				// socket is closed
				return NULL;
			}
		}

		void ApiServer::listen()
		{
			try {
				_dist.start();
			} catch (const ibrcommon::ThreadException &ex) {
				IBRCOMMON_LOGGER(error) << "failed to start ApiServer" << IBRCOMMON_LOGGER_ENDL;
			}
		}

		void ApiServer::shutdown()
		{
			_dist.stop();
			_tcpsrv.close();
		}

		void ApiServer::connectionUp(ClientHandler*)
		{
			IBRCOMMON_LOGGER_DEBUG(5) << "client connection up" << IBRCOMMON_LOGGER_ENDL;
			IBRCOMMON_LOGGER_DEBUG(60) << "current open connections " << _clients.size() << IBRCOMMON_LOGGER_ENDL;
		}

		void ApiServer::connectionDown(ClientHandler*)
		{
			IBRCOMMON_LOGGER_DEBUG(5) << "client connection down" << IBRCOMMON_LOGGER_ENDL;
			IBRCOMMON_LOGGER_DEBUG(60) << "current open connections " << _clients.size() << IBRCOMMON_LOGGER_ENDL;
		}

		ApiServer::Distributor::Distributor(std::list<ClientHandler*> &connections, ibrcommon::Mutex &list)
		 : _lock(list), _connections(connections)
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
					dtn::data::MetaBundle mb = _received.getnpop(true);

					ibrcommon::MutexLock l(_lock);

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

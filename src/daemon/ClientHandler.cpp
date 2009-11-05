/*
 * ClientHandler.cpp
 *
 *  Created on: 24.06.2009
 *      Author: morgenro
 */

#include "daemon/ClientHandler.h"
#include "ibrdtn/data/BLOBManager.h"
#include "core/EventSwitch.h"
#include "core/GlobalEvent.h"
#include "core/BundleCore.h"
#include <iostream>

using namespace dtn::blob;
using namespace dtn::data;
using namespace dtn::streams;
using namespace dtn::core;

namespace dtn
{
	namespace daemon
	{
		ClientHandler::ClientHandler(iostream &stream)
		 : AbstractWorker(), _connection(stream, 30), _connected(false)
		{
			EventSwitch::registerEventReceiver(GlobalEvent::className, this);
		}

		ClientHandler::ClientHandler(int socket)
		 : _tcpstream(socket, tcpstream::STREAM_INCOMING), AbstractWorker(), _connection(_tcpstream, 0), _connected(false)
		{
			EventSwitch::registerEventReceiver(GlobalEvent::className, this);
		}

		ClientHandler::~ClientHandler()
		{
			_connection.waitFor();
			EventSwitch::unregisterEventReceiver(GlobalEvent::className, this);
			join();
		}

		void ClientHandler::embalm()
		{
		}

		void ClientHandler::raiseEvent(const dtn::core::Event *evt)
		{
			static dtn::utils::Mutex mutex;
			dtn::utils::MutexLock l(mutex);

			const GlobalEvent *global = dynamic_cast<const GlobalEvent*>(evt);

			if (global != NULL)
			{
				if (global->getAction() == GlobalEvent::GLOBAL_SHUTDOWN)
				{
					shutdown();
				}
			}
		}

		bool ClientHandler::isConnected()
		{
			return _connected;
		}

		void ClientHandler::shutdown()
		{
#ifdef DO_EXTENDED_DEBUG_OUTPUT
			cout << "Client disconnected: " << _local.getString() << endl;
			cout << "shutdown: ClientHandler" << endl;
#endif

			dtn::core::BundleCore &core = dtn::core::BundleCore::getInstance();
			core.unregisterSubNode( _local );

			_tcpstream.close();
			_connection.shutdown();

			bury();
		}

		void ClientHandler::run()
		{
			try {
				// receive the header
				_connection >> _contact;
				received(_contact);

				// set to connected
				_connected = true;

				// start the receiver thread
				_connection.start();

				while (isConnected())
				{
					Bundle b;
					_connection >> b;

					// create a new sequence number
					b.relabel();

					// set the source
					b._source = _local;

					// forward the bundle
					dtn::core::AbstractWorker::transmit(b);

					yield();
				}
			} catch (IOException ex) {

			} catch (dtn::exceptions::InvalidBundleData ex) {

			}

			shutdown();
		}

		dtn::net::TransmitReport ClientHandler::callbackBundleReceived(const Bundle &b)
		{
			_connection << b;
			_connection.flush();

			return dtn::net::BUNDLE_ACCEPTED;
		}

		void ClientHandler::received(StreamContactHeader &h)
		{
			_contact = h;

			// register at bundle core
			dtn::core::BundleCore &core = dtn::core::BundleCore::getInstance();
			_local = dtn::core::BundleCore::local + _contact._localeid.getApplication();
			core.registerSubNode( _local, (AbstractWorker*)this );

#ifdef DO_EXTENDED_DEBUG_OUTPUT
			cout << "New client connected: " << _local.getString() << endl;
#endif

			// reply with own header
			StreamContactHeader srv_header(dtn::core::BundleCore::local);
			_connection << srv_header;
		}
	}
}


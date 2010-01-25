/*
 * ClientHandler.cpp
 *
 *  Created on: 24.06.2009
 *      Author: morgenro
 */

#include "daemon/ClientHandler.h"
#include "core/EventSwitch.h"
#include "core/GlobalEvent.h"
#include "core/BundleCore.h"
#include <iostream>

using namespace dtn::data;
using namespace dtn::streams;
using namespace dtn::core;

namespace dtn
{
	namespace daemon
	{
		ClientHandler::ClientHandler(iostream &stream)
		 : _connection(stream), _connected(false)
		{
			EventSwitch::registerEventReceiver(GlobalEvent::className, this);
		}

		ClientHandler::ClientHandler(int socket)
		 : _tcpstream(socket, ibrcommon::tcpstream::STREAM_INCOMING), _connection(_tcpstream), _connected(false)
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
			static ibrcommon::Mutex mutex;
			ibrcommon::MutexLock l(mutex);

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
			cout << "Client disconnected: " << _eid.getString() << endl;
			cout << "shutdown: ClientHandler" << endl;
#endif

			_tcpstream.close();
			_connection.shutdown();
			AbstractWorker::shutdown();

			bury();
		}

		void ClientHandler::run()
		{
			try {
				// receive the header
				_connection >> _contact;
				received(_contact);

				// reply with own header
				StreamContactHeader srv_header(dtn::core::BundleCore::local);
				_connection << srv_header;

				// initialize the AbstractWorker
				// deactivate the thread for receiving bundles is bit 0x80 is set
				AbstractWorker::initialize(_contact._localeid.getApplication(), !(_contact._flags & 0x80));

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
					b._source = _eid;

					// forward the bundle
					dtn::core::AbstractWorker::transmit(b);

					yield();
				}
			} catch (IOException ex) {

			} catch (dtn::exceptions::InvalidBundleData ex) {

			} catch (dtn::exceptions::InvalidDataException ex) {

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

#ifdef DO_EXTENDED_DEBUG_OUTPUT
			cout << "New client connected: " << _contact.getEID().getString() << endl;
#endif

		}
	}
}


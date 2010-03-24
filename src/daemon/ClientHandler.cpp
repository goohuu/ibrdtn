/*
 * ClientHandler.cpp
 *
 *  Created on: 24.06.2009
 *      Author: morgenro
 */

#include "daemon/ClientHandler.h"
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
		ClientHandler::ClientHandler(ibrcommon::tcpstream *stream)
		 : _stream(stream), _connection(*this, *_stream), _connected(false)
		{
			bindEvent(GlobalEvent::className);
		}

		ClientHandler::~ClientHandler()
		{
			unbindEvent(GlobalEvent::className);

			AbstractWorker::shutdown();
			(*_stream).close();

			join();
		}

		void ClientHandler::eventShutdown()
		{
			// shutdown message received
		}

		void ClientHandler::eventTimeout()
		{
			_connected = false;
		}

		void ClientHandler::eventConnectionUp(const StreamContactHeader &header)
		{
			// initialize the AbstractWorker
			// deactivate the thread for receiving bundles is bit 0x80 is set
			AbstractWorker::initialize(header._localeid.getApplication(), !(header._flags & 0x80));

			// contact received event
			received(header);
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
			return _connection.isConnected();
		}

		void ClientHandler::shutdown()
		{
#ifdef DO_EXTENDED_DEBUG_OUTPUT
			cout << "Client disconnected: " << _eid.getString() << endl;
#endif
			_connection.wait();
			_connection.close();
			bury();
		}

		void ClientHandler::run()
		{
			try {
				// do the handshake
				_connection.handshake(dtn::core::BundleCore::local, 10);

				// set to connected
				_connected = true;

				while (_connection.good())
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
			} catch (dtn::exceptions::IOException ex) {

			} catch (dtn::exceptions::InvalidBundleData ex) {

			} catch (dtn::exceptions::InvalidDataException ex) {

			}

			shutdown();
		}

		void ClientHandler::callbackBundleReceived(const Bundle &b)
		{
			_connection << b;
			_connection.flush();
		}

		void ClientHandler::received(const StreamContactHeader &h)
		{
			_contact = h;

#ifdef DO_EXTENDED_DEBUG_OUTPUT
			cout << "New client connected: " << _contact.getEID().getString() << endl;
#endif

		}
	}
}


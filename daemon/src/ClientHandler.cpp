/*
 * ClientHandler.cpp
 *
 *  Created on: 24.06.2009
 *      Author: morgenro
 */

#include "ClientHandler.h"
#include "core/GlobalEvent.h"
#include "core/BundleCore.h"
#include "net/BundleReceivedEvent.h"
#include <iostream>

using namespace dtn::data;
using namespace dtn::streams;
using namespace dtn::core;

namespace dtn
{
	namespace daemon
	{
		ClientHandler::ClientHandler(ibrcommon::tcpstream *stream)
		 : _free(false), _running(true), _stream(stream), _connection(*this, *_stream)
		{
		}

		ClientHandler::~ClientHandler()
		{
			shutdown();
			join();
		}

		const dtn::data::EID& ClientHandler::getPeer() const
		{
			return _eid;
		}

		void ClientHandler::iamfree()
		{
			_free = true;
		}

		bool ClientHandler::free()
		{
			ibrcommon::MutexLock l(_freemutex);
			return _free;
		}

		void ClientHandler::eventShutdown()
		{
			// shutdown message received
			(*_stream).done();
			(*_stream).close();
		}

		void ClientHandler::eventTimeout()
		{
			(*_stream).done();
			(*_stream).close();
		}

		void ClientHandler::eventConnectionUp(const StreamContactHeader &header)
		{
			// contact received event
			_eid = BundleCore::local + header._localeid.getApplication();
		}

		void ClientHandler::eventError()
		{
			(*_stream).done();
			(*_stream).close();
		}

		void ClientHandler::eventConnectionDown()
		{
			ibrcommon::MutexLock l(_freemutex);
			iamfree();
		}

		bool ClientHandler::isConnected()
		{
			return _connection.isConnected();
		}

		void ClientHandler::shutdown()
		{
			_connection.shutdown();
		}

		void ClientHandler::run()
		{
			try {
				char flags = 0;

				// request acknowledgements
				flags |= dtn::streams::StreamContactHeader::REQUEST_ACKNOWLEDGMENTS;

				// do the handshake
				_connection.handshake(dtn::core::BundleCore::local, 10, flags);

				BundleStorage &storage = BundleCore::getInstance().getStorage();

				while (_running)
				{
					try {
						dtn::data::Bundle b = storage.get( getPeer() );
						(*this) << b;
						storage.remove(b);
					} catch (dtn::exceptions::NoBundleFoundException ex) {
						break;
					}
				}

				while (_running)
				{
					dtn::data::Bundle bundle;
					_connection >> bundle;

					// create a new sequence number
					bundle.relabel();

					// set the source
					bundle._source = _eid;

					// raise default bundle received event
					dtn::net::BundleReceivedEvent::raise(EID(), bundle);

					yield();
				}

				_connection.shutdown();
			} catch (dtn::exceptions::IOException ex) {
				_connection.shutdown(StreamConnection::CONNECTION_SHUTDOWN_ERROR);
				_running = false;
			} catch (dtn::exceptions::InvalidDataException ex) {
				_connection.shutdown(StreamConnection::CONNECTION_SHUTDOWN_ERROR);
				_running = false;
			} catch (dtn::exceptions::InvalidBundleData ex) {
				_connection.shutdown(StreamConnection::CONNECTION_SHUTDOWN_ERROR);
				_running = false;
			}
		}

		ClientHandler& operator>>(ClientHandler &conn, dtn::data::Bundle &bundle)
		{
			conn._connection >> bundle;
			if (!conn._connection.good()) throw dtn::exceptions::IOException("read from stream failed");
		}

		ClientHandler& operator<<(ClientHandler &conn, const dtn::data::Bundle &bundle)
		{
			// reset the ACK value
			conn._connection.reset();

			// transmit the bundle
			conn._connection << bundle << std::flush;

			// wait until all segments are acknowledged.
			conn._connection.wait();
		}
	}
}


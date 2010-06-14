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
#include <ibrdtn/streams/StreamContactHeader.h>
#include <ibrdtn/data/Serializer.h>
#include <iostream>

using namespace dtn::data;
using namespace dtn::streams;
using namespace dtn::core;

namespace dtn
{
	namespace daemon
	{
		ClientHandler::ClientHandler(ibrcommon::tcpstream *stream)
		 : ibrcommon::JoinableThread(), _free(false), _running(true), _stream(stream), _connection(*this, *_stream)
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
			try {
				// wait until all segments are ACK'd with 10 seconds timeout
				_connection.wait(10000);
			} catch (...) {

			}

			// shutdown the connection
			_connection.shutdown();
		}

		void ClientHandler::run()
		{
			try {
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
							dtn::data::DefaultSerializer(_connection) << b; _connection << std::flush;
							storage.remove(b);
						} catch (dtn::core::BundleStorage::NoBundleFoundException ex) {
							break;
						}
					}

					while (_running)
					{
						dtn::data::Bundle bundle;
						dtn::data::DefaultDeserializer(_connection) >> bundle;

						// create a new sequence number
						bundle.relabel();

						// set the source
						bundle._source = _eid;

						// raise default bundle received event
						dtn::net::BundleReceivedEvent::raise(EID(), bundle);

						yield();
					}

					_connection.shutdown();
				} catch (ibrcommon::IOException ex) {
					_connection.shutdown(StreamConnection::CONNECTION_SHUTDOWN_ERROR);
					_running = false;
				} catch (dtn::InvalidDataException ex) {
					_connection.shutdown(StreamConnection::CONNECTION_SHUTDOWN_ERROR);
					_running = false;
				}
			} catch (...) {

			}
		}

		ClientHandler& operator>>(ClientHandler &conn, dtn::data::Bundle &bundle)
		{
			// get a bundle
			dtn::data::DefaultDeserializer(conn._connection) >> bundle;

			return conn;
		}

		ClientHandler& operator<<(ClientHandler &conn, const dtn::data::Bundle &bundle)
		{
			// transmit the bundle
			dtn::data::DefaultSerializer(conn._connection) << bundle;

			// mark the end of the bundle
			conn._connection << std::flush;

			// wait until all segments are ACK'd with 10 seconds timeout
			conn._connection.wait(10000);

			return conn;
		}
	}
}


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
#include "core/BundleEvent.h"
#include <ibrdtn/streams/StreamContactHeader.h>
#include <ibrdtn/data/Serializer.h>
#include <iostream>
#include <ibrcommon/Logger.h>

using namespace dtn::data;
using namespace dtn::streams;
using namespace dtn::core;

namespace dtn
{
	namespace daemon
	{
		ClientHandler::ClientHandler(dtn::net::GenericServer<ClientHandler> &srv, ibrcommon::tcpstream *stream)
		 : dtn::net::GenericConnection<ClientHandler>((dtn::net::GenericServer<ClientHandler>&)srv),
		   ibrcommon::JoinableThread(), _sender(*this), _stream(stream), _connection(*this, *_stream), _semaphore(0)
		{
			stream->enableNoDelay();
		}

		ClientHandler::~ClientHandler()
		{
		}

		const dtn::data::EID& ClientHandler::getPeer() const
		{
			return _eid;
		}

		void ClientHandler::eventShutdown()
		{
			// stop the receiver
			_sender.stop();
		}

		void ClientHandler::eventTimeout()
		{
			// stop the receiver
			_sender.stop();
		}

		void ClientHandler::eventError()
		{
			// stop the receiver
			_sender.stop();
		}

		void ClientHandler::eventConnectionUp(const StreamContactHeader &header)
		{
			if (BundleCore::local.getScheme() == dtn::data::EID::CBHE_SCHEME)
			{
				// This node is working with compressed addresses
				// generate a number
				_eid = BundleCore::local + "." + header._localeid.getNode();
			}
			else
			{
				// contact received event
				_eid = BundleCore::local + "/" + header._localeid.getNode();
			}
		}

		void ClientHandler::eventConnectionDown()
		{
		}

		void ClientHandler::eventBundleRefused()
		{
			try {
				const dtn::data::Bundle bundle = _sentqueue.frontpop();

				// set ACK to zero
				_lastack = 0;

			} catch (ibrcommon::Exception ex) {
				// pop on empty queue!
			}
		}

		void ClientHandler::eventBundleForwarded()
		{
			try {
				const dtn::data::Bundle bundle = _sentqueue.frontpop();

				// raise bundle event
				dtn::core::BundleEvent::raise(bundle, BUNDLE_DELIVERED);

				// set ACK to zero
				_lastack = 0;

			} catch (ibrcommon::Exception ex) {
				// pop on empty queue!
			}
		}

		void ClientHandler::eventBundleAck(size_t ack)
		{
			_lastack = ack;
		}

		bool ClientHandler::isConnected()
		{
			return _connection.isConnected();
		}

		void ClientHandler::shutdown()
		{
			// shutdown the sender thread
			_sender.stop();
		}

		void ClientHandler::finally()
		{
			try {
				// close the stream
				(*_stream).close();
			} catch (std::exception) {

			}

			_server.remove(this);

			// connection down -> connection to be deleted
			delete this;
		}

		void ClientHandler::run()
		{
			try {
				char flags = 0;

				// request acknowledgements
				flags |= dtn::streams::StreamContactHeader::REQUEST_ACKNOWLEDGMENTS;

				// do the handshake
				_connection.handshake(dtn::core::BundleCore::local, 10, flags);

				// start the sender thread
				_sender.start();

				while (true)
				{
					dtn::data::Bundle bundle;
					dtn::data::DefaultDeserializer(_connection) >> bundle;

					// create a new sequence number
					bundle.relabel();

					// check address fields for "api:me", this has to be replaced
					dtn::data::EID clienteid("api:me");

					// set the source address to the sending EID
					bundle._source = _eid;

					if (bundle._destination == clienteid) bundle._destination = _eid;
					if (bundle._reportto == clienteid) bundle._reportto = _eid;
					if (bundle._custodian == clienteid) bundle._custodian = _eid;

					// raise default bundle received event
					dtn::net::BundleReceivedEvent::raise(EID(), bundle);

					yield();
				}
			} catch (ibrcommon::IOException ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "ClientHandler::run(): IOException (" << ex.what() << ")" << IBRCOMMON_LOGGER_ENDL;
				_connection.abort();
				_connection.shutdown(StreamConnection::CONNECTION_SHUTDOWN_ERROR);
			} catch (dtn::InvalidDataException ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "ClientHandler::run(): InvalidDataException (" << ex.what() << ")" << IBRCOMMON_LOGGER_ENDL;
				_connection.abort();
				_connection.shutdown(StreamConnection::CONNECTION_SHUTDOWN_ERROR);
			} catch (std::exception ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "ClientHandler::run(): std::exception (" << ex.what() << ")" << IBRCOMMON_LOGGER_ENDL;
				_connection.abort();
				_connection.shutdown(StreamConnection::CONNECTION_SHUTDOWN_ERROR);
			}
		}

		ClientHandler& operator>>(ClientHandler &conn, dtn::data::Bundle &bundle)
		{
			// get a bundle
			dtn::data::DefaultDeserializer(conn._connection, dtn::core::BundleCore::getInstance()) >> bundle;

			return conn;
		}

		ClientHandler& operator<<(ClientHandler &conn, const dtn::data::Bundle &bundle)
		{
			ibrcommon::MutexLock l(conn._send_lock);

			// add bundle to the queue
			conn._sentqueue.push(bundle);

			// transmit the bundle
			dtn::data::DefaultSerializer(conn._connection) << bundle;

			// mark the end of the bundle
			conn._connection << std::flush;

			return conn;
		}

		ClientHandler::Sender::Sender(ClientHandler &client)
		 : _client(client)
		{
		}

		ClientHandler::Sender::~Sender()
		{
			join();
		}

		void ClientHandler::Sender::run()
		{
			try {
				try {
					BundleStorage &storage = BundleCore::getInstance().getStorage();

					while (true)
					{
						dtn::data::Bundle b = storage.get( _client.getPeer() );
						_client << b;
						storage.remove(b);

						// idle a little bit
						testcancel();
						yield();
					}
				} catch (dtn::core::BundleStorage::NoBundleFoundException ex) {
				}

				while (true)
				{
					dtn::data::Bundle bundle = blockingpop();

					// send bundle
					_client << bundle;

					// idle a little bit
					testcancel();
					yield();
				}
			} catch (ibrcommon::IOException ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "API: IOException" << IBRCOMMON_LOGGER_ENDL;
			} catch (dtn::InvalidDataException ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "API: InvalidDataException" << IBRCOMMON_LOGGER_ENDL;
			} catch (ibrcommon::QueueEmptyException) {
				IBRCOMMON_LOGGER_DEBUG(10) << "API: QueueEmptyException" << IBRCOMMON_LOGGER_ENDL;
			} catch (std::exception) {
				IBRCOMMON_LOGGER_DEBUG(10) << "unexpected API error!" << IBRCOMMON_LOGGER_ENDL;
			}
		}

		void ClientHandler::Sender::finally()
		{
			_client.stop();
		}

		void ClientHandler::queue(const dtn::data::Bundle &bundle)
		{
			_sender.push(bundle);
		}
	}
}

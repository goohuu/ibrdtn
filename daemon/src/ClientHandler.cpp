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
		 : dtn::net::GenericConnection<ClientHandler>((dtn::net::GenericServer<ClientHandler>&)srv), ibrcommon::DetachedThread(0),
		   _sender(*this), _stream(stream), _connection(*this, *_stream)
		{
			// stream->enableNoDelay();
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
		}

		void ClientHandler::eventTimeout()
		{
		}

		void ClientHandler::eventError()
		{
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
			IBRCOMMON_LOGGER_DEBUG(40) << "ClientHandler::eventConnectionDown()" << IBRCOMMON_LOGGER_ENDL;

			try {
				// stop the sender
				_sender.abort();
				_sender.stop();
			} catch (const ibrcommon::ThreadException &ex) {
				IBRCOMMON_LOGGER_DEBUG(50) << "ClientHandler::eventConnectionDown(): ThreadException (" << ex.what() << ")" << IBRCOMMON_LOGGER_ENDL;
			}
		}

		void ClientHandler::eventBundleRefused()
		{
			try {
				const dtn::data::Bundle bundle = _sentqueue.getnpop();

				// set ACK to zero
				_lastack = 0;

			} catch (ibrcommon::QueueUnblockedException) {
				// pop on empty queue!
			}
		}

		void ClientHandler::eventBundleForwarded()
		{
			try {
				const dtn::data::Bundle bundle = _sentqueue.getnpop();

				// raise bundle event
				dtn::core::BundleEvent::raise(bundle, BUNDLE_DELIVERED);

				// set ACK to zero
				_lastack = 0;

			} catch (ibrcommon::QueueUnblockedException) {
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

		void ClientHandler::initialize()
		{
			try {
				// start the ClientHandler (service)
				start();
			} catch (const ibrcommon::ThreadException &ex) {
				IBRCOMMON_LOGGER(error) << "failed to start thread in ClientHandler\n" << ex.what() << IBRCOMMON_LOGGER_ENDL;
			}
		}

		void ClientHandler::shutdown()
		{
			// shutdown
			_connection.shutdown(StreamConnection::CONNECTION_SHUTDOWN_ERROR);

			try {
				// abort the connection thread
				this->stop();
			} catch (const ibrcommon::ThreadException &ex) {
				IBRCOMMON_LOGGER_DEBUG(50) << "ClientHandler::shutdown(): ThreadException (" << ex.what() << ")" << IBRCOMMON_LOGGER_ENDL;
			}
		}

		void ClientHandler::finally()
		{
			IBRCOMMON_LOGGER_DEBUG(60) << "ClientHandler down" << IBRCOMMON_LOGGER_ENDL;

			try {
				// close the stream
				(*_stream).close();
			} catch (std::exception) {

			}

			try {
				// shutdown the sender thread
				_sender.shutdown();
			} catch (std::exception) {

			}

			ibrcommon::MutexLock l(_server.mutex());
			_server.remove(this);
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
					dtn::net::BundleReceivedEvent::raise(dtn::core::BundleCore::local, bundle);

					yield();
				}
			} catch (const ibrcommon::ThreadException &ex) {
				IBRCOMMON_LOGGER(error) << "failed to start thread in ClientHandler\n" << ex.what() << IBRCOMMON_LOGGER_ENDL;
				_connection.shutdown(StreamConnection::CONNECTION_SHUTDOWN_ERROR);
			} catch (ibrcommon::IOException ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "ClientHandler::run(): IOException (" << ex.what() << ")" << IBRCOMMON_LOGGER_ENDL;
				_connection.shutdown(StreamConnection::CONNECTION_SHUTDOWN_ERROR);
			} catch (dtn::InvalidDataException ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "ClientHandler::run(): InvalidDataException (" << ex.what() << ")" << IBRCOMMON_LOGGER_ENDL;
				_connection.shutdown(StreamConnection::CONNECTION_SHUTDOWN_ERROR);
			} catch (std::exception ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "ClientHandler::run(): std::exception (" << ex.what() << ")" << IBRCOMMON_LOGGER_ENDL;
				_connection.shutdown(StreamConnection::CONNECTION_SHUTDOWN_ERROR);
			} catch (...) {
				IBRCOMMON_LOGGER_DEBUG(10) << "ClientHandler::run(): canceled" << IBRCOMMON_LOGGER_ENDL;
				throw;
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
			// add bundle to the queue
			conn._sentqueue.push(bundle);

			// transmit the bundle
			dtn::data::DefaultSerializer(conn._connection) << bundle;

			// mark the end of the bundle
			conn._connection << std::flush;

			return conn;
		}

		ClientHandler::Sender::Sender(ClientHandler &client)
		 : _abort(false), _client(client)
		{
		}

		ClientHandler::Sender::~Sender()
		{
			// enable this to force the dtnd to block on unsuccessful object free
			// shutdown();
		}

		void ClientHandler::Sender::shutdown()
		{
			try {
				this->stop();
			} catch (const ibrcommon::ThreadException &ex) {
				IBRCOMMON_LOGGER_DEBUG(50) << "ClientHandler::Sender::shutdown(): ThreadException (" << ex.what() << ")" << IBRCOMMON_LOGGER_ENDL;
			}

			this->join();
		}

		void ClientHandler::Sender::run()
		{
			try {
				try {
					BundleStorage &storage = BundleCore::getInstance().getStorage();

					while (!_abort)
					{
						dtn::data::Bundle b = storage.get( _client.getPeer() );
						_client << b;
						storage.remove(b);
					}
				} catch (const dtn::core::BundleStorage::NoBundleFoundException&) {
				}

				while (!_abort)
				{
					try {
						dtn::data::Bundle bundle = getnpop(true);

						// send bundle
						_client << bundle;
					} catch (ibrcommon::QueueUnblockedException) { }

					// idle a little bit
					yield();
				}

			} catch (const ibrcommon::IOException &ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "API: IOException says " << ex.what() << IBRCOMMON_LOGGER_ENDL;
			} catch (const dtn::InvalidDataException &ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "API: InvalidDataException says " << ex.what() << IBRCOMMON_LOGGER_ENDL;
			} catch (const std::exception &ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "unexpected API error! " << ex.what() << IBRCOMMON_LOGGER_ENDL;
			} catch (...) {
				IBRCOMMON_LOGGER_DEBUG(10) << "API: canceled" << IBRCOMMON_LOGGER_ENDL;
				throw;
			}

			try {
				_client.stop();
			} catch (const ibrcommon::ThreadException &ex) {
				IBRCOMMON_LOGGER_DEBUG(50) << "ClientHandler::Sender::run(): ThreadException (" << ex.what() << ") on termination" << IBRCOMMON_LOGGER_ENDL;
			}
		}

		void ClientHandler::queue(const dtn::data::Bundle &bundle)
		{
			_sender.push(bundle);
		}
	}
}

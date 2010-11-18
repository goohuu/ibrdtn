/*
 * ClientHandler.cpp
 *
 *  Created on: 24.06.2009
 *      Author: morgenro
 */

#include "ClientHandler.h"
#include "Configuration.h"
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
		ClientHandler::ClientHandler(ApiServerInterface &srv, ibrcommon::tcpstream *stream, size_t i)
		 : ibrcommon::DetachedThread(0), id(i), _srv(srv), _sender(*this), _stream(stream), _connection(*this, *_stream)
		{
			_connection.exceptions(std::ios::badbit | std::ios::eofbit);

			if ( dtn::daemon::Configuration::getInstance().getNetwork().getTCPOptionNoDelay() )
			{
				stream->enableNoDelay();
			}
		}

		ClientHandler::~ClientHandler()
		{
			_sender.join();
		}

		const dtn::data::EID& ClientHandler::getPeer() const
		{
			return _eid;
		}

		void ClientHandler::eventShutdown(StreamConnection::ConnectionShutdownCases)
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

			_srv.connectionUp(this);
		}

		void ClientHandler::eventConnectionDown()
		{
			IBRCOMMON_LOGGER_DEBUG(40) << "ClientHandler::eventConnectionDown()" << IBRCOMMON_LOGGER_ENDL;

			try {
				// stop the sender
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

			// remove the client from the list in ApiServer
			_srv.connectionDown(this);

			// close the stream
			(*_stream).close();

			try {
				// shutdown the sender thread
				_sender.stop();
			} catch (std::exception) { };
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

				while (!_connection.eof())
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
					dtn::net::BundleReceivedEvent::raise(dtn::core::BundleCore::local, bundle, true);

					yield();
				}
			} catch (const ibrcommon::ThreadException &ex) {
				IBRCOMMON_LOGGER(error) << "failed to start thread in ClientHandler\n" << ex.what() << IBRCOMMON_LOGGER_ENDL;
				_connection.shutdown(StreamConnection::CONNECTION_SHUTDOWN_ERROR);
			} catch (const dtn::SerializationFailedException &ex) {
				IBRCOMMON_LOGGER(error) << "ClientHandler::run(): SerializationFailedException (" << ex.what() << ")" << IBRCOMMON_LOGGER_ENDL;
				_connection.shutdown(StreamConnection::CONNECTION_SHUTDOWN_ERROR);
			} catch (const ibrcommon::IOException &ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "ClientHandler::run(): IOException (" << ex.what() << ")" << IBRCOMMON_LOGGER_ENDL;
				_connection.shutdown(StreamConnection::CONNECTION_SHUTDOWN_ERROR);
			} catch (const dtn::InvalidDataException &ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "ClientHandler::run(): InvalidDataException (" << ex.what() << ")" << IBRCOMMON_LOGGER_ENDL;
				_connection.shutdown(StreamConnection::CONNECTION_SHUTDOWN_ERROR);
			} catch (const std::exception &ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "ClientHandler::run(): std::exception (" << ex.what() << ")" << IBRCOMMON_LOGGER_ENDL;
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
		}

		bool ClientHandler::Sender::__cancellation()
		{
			// cancel the main thread in here
			this->abort();

			// return false, to signal that further cancel (the hardway) is needed
			return false;
		}

		void ClientHandler::Sender::run()
		{
			// The queue is not cancel-safe with uclibc, so we need to
			// disable cancel here
			int oldstate;
			ibrcommon::Thread::disableCancel(oldstate);

			try {
				while (true)
				{
					dtn::data::Bundle bundle = getnpop(true);

					// enable cancellation during transmission
					ibrcommon::Thread::CancelProtector cprotect(true);

					// enable cancellation during transmission
					{
						ibrcommon::Thread::CancelProtector cprotect(true);

						// send bundle
						_client << bundle;
					}

					// idle a little bit
					yield();
				}

			} catch (const ibrcommon::QueueUnblockedException &ex) {
				IBRCOMMON_LOGGER_DEBUG(40) << "ClientHandler::Sender::run(): aborted" << IBRCOMMON_LOGGER_ENDL;
				return;
			} catch (const ibrcommon::IOException &ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "API: IOException says " << ex.what() << IBRCOMMON_LOGGER_ENDL;
			} catch (const dtn::InvalidDataException &ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "API: InvalidDataException says " << ex.what() << IBRCOMMON_LOGGER_ENDL;
			} catch (const std::exception &ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "unexpected API error! " << ex.what() << IBRCOMMON_LOGGER_ENDL;
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

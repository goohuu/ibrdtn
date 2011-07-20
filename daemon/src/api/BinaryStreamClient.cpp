/*
 * BinaryStreamClient.cpp
 *
 *  Created on: 19.07.2011
 *      Author: morgenro
 */

#include "config.h"
#include "Configuration.h"
#include "api/BinaryStreamClient.h"
#include "core/GlobalEvent.h"
#include "core/BundleCore.h"
#include "net/BundleReceivedEvent.h"
#include "core/BundleEvent.h"
#include <ibrdtn/streams/StreamContactHeader.h>
#include <ibrdtn/data/Serializer.h>
#include <iostream>
#include <ibrcommon/Logger.h>

namespace dtn
{
	namespace api
	{
		BinaryStreamClient::BinaryStreamClient(ClientHandler &client, ibrcommon::tcpstream &stream)
		 : ProtocolHandler(client, stream), _sender(*this), _connection(*this, _stream)
		{
		}

		BinaryStreamClient::~BinaryStreamClient()
		{
			_sender.join();
		}

		const dtn::data::EID& BinaryStreamClient::getPeer() const
		{
			return _eid;
		}

		void BinaryStreamClient::eventShutdown(dtn::streams::StreamConnection::ConnectionShutdownCases)
		{
		}

		void BinaryStreamClient::eventTimeout()
		{
		}

		void BinaryStreamClient::eventError()
		{
		}

		void BinaryStreamClient::eventConnectionUp(const dtn::streams::StreamContactHeader &header)
		{
			if (BundleCore::local.getScheme() == dtn::data::EID::CBHE_SCHEME)
			{
				// This node is working with compressed addresses
				// generate a number
				_eid = BundleCore::local + "." + header._localeid.getHost();
			}
			else
			{
				// contact received event
				_eid = BundleCore::local + "/" + header._localeid.getHost() + header._localeid.getApplication();
			}

			IBRCOMMON_LOGGER_DEBUG(20) << "new client connected: " << _eid.getString() << IBRCOMMON_LOGGER_ENDL;

			_client.getRegistration().subscribe(_eid);
		}

		void BinaryStreamClient::eventConnectionDown()
		{
			IBRCOMMON_LOGGER_DEBUG(40) << "BinaryStreamClient::eventConnectionDown()" << IBRCOMMON_LOGGER_ENDL;

			_client.getRegistration().unsubscribe(_eid);

			try {
				// stop the sender
				_sender.stop();
			} catch (const ibrcommon::ThreadException &ex) {
				IBRCOMMON_LOGGER_DEBUG(50) << "BinaryStreamClient::eventConnectionDown(): ThreadException (" << ex.what() << ")" << IBRCOMMON_LOGGER_ENDL;
			}
		}

		void BinaryStreamClient::eventBundleRefused()
		{
			try {
				const dtn::data::Bundle bundle = _sentqueue.getnpop();

				// set ACK to zero
				_lastack = 0;

			} catch (const ibrcommon::QueueUnblockedException&) {
				// pop on empty queue!
			}
		}

		void BinaryStreamClient::eventBundleForwarded()
		{
			try {
				const dtn::data::Bundle bundle = _sentqueue.getnpop();

				// notify bundle as delivered
				_client.getRegistration().delivered(bundle);

				// set ACK to zero
				_lastack = 0;
			} catch (const ibrcommon::QueueUnblockedException&) {
				// pop on empty queue!
			}
		}

		void BinaryStreamClient::eventBundleAck(size_t ack)
		{
			_lastack = ack;
		}

		bool BinaryStreamClient::__cancellation()
		{
			// shutdown
			_connection.shutdown(dtn::streams::StreamConnection::CONNECTION_SHUTDOWN_ERROR);

			// close the stream
			try {
				_stream.close();
			} catch (const ibrcommon::ConnectionClosedException&) { };

			return true;
		}

		void BinaryStreamClient::finally()
		{
			IBRCOMMON_LOGGER_DEBUG(60) << "BinaryStreamClient down" << IBRCOMMON_LOGGER_ENDL;

			// close the stream
			try {
				_stream.close();
			} catch (const ibrcommon::ConnectionClosedException&) { };

			try {
				// shutdown the sender thread
				_sender.stop();
			} catch (const std::exception&) { };
		}

		void BinaryStreamClient::run()
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

					// process the new bundle
					_client.getAPIServer().processIncomingBundle(_eid, bundle);
				}
			} catch (const ibrcommon::ThreadException &ex) {
				IBRCOMMON_LOGGER(error) << "failed to start thread in BinaryStreamClient\n" << ex.what() << IBRCOMMON_LOGGER_ENDL;
				_connection.shutdown(dtn::streams::StreamConnection::CONNECTION_SHUTDOWN_ERROR);
			} catch (const dtn::SerializationFailedException &ex) {
				IBRCOMMON_LOGGER(error) << "BinaryStreamClient::run(): SerializationFailedException (" << ex.what() << ")" << IBRCOMMON_LOGGER_ENDL;
				_connection.shutdown(dtn::streams::StreamConnection::CONNECTION_SHUTDOWN_ERROR);
			} catch (const ibrcommon::IOException &ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "BinaryStreamClient::run(): IOException (" << ex.what() << ")" << IBRCOMMON_LOGGER_ENDL;
				_connection.shutdown(dtn::streams::StreamConnection::CONNECTION_SHUTDOWN_ERROR);
			} catch (const dtn::InvalidDataException &ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "BinaryStreamClient::run(): InvalidDataException (" << ex.what() << ")" << IBRCOMMON_LOGGER_ENDL;
				_connection.shutdown(dtn::streams::StreamConnection::CONNECTION_SHUTDOWN_ERROR);
			} catch (const std::exception &ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "BinaryStreamClient::run(): std::exception (" << ex.what() << ")" << IBRCOMMON_LOGGER_ENDL;
				_connection.shutdown(dtn::streams::StreamConnection::CONNECTION_SHUTDOWN_ERROR);
			}
		}

		bool BinaryStreamClient::good() const
		{
			return _stream.good();
		}

		BinaryStreamClient::Sender::Sender(BinaryStreamClient &client)
		 : _client(client)
		{
		}

		BinaryStreamClient::Sender::~Sender()
		{
		}

		bool BinaryStreamClient::Sender::__cancellation()
		{
			// cancel the main thread in here
			this->abort();

			// abort all blocking calls on the registration object
			_client._client.getRegistration().abort();

			return true;
		}

		void BinaryStreamClient::Sender::run()
		{
			Registration &reg = _client._client.getRegistration();

			try {
				while (_client.good())
				{
					try {
						dtn::data::Bundle bundle = reg.receive();

						// process the bundle block (security, compression, ...)
						dtn::core::BundleCore::processBlocks(bundle);

						// add bundle to the queue
						_client._sentqueue.push(bundle);

						// transmit the bundle
						dtn::data::DefaultSerializer(_client._connection) << bundle;

						// mark the end of the bundle
						_client._connection << std::flush;
					} catch (const dtn::core::BundleStorage::NoBundleFoundException&) {
						reg.wait_for_bundle();
					}

					// idle a little bit
					yield();
				}
			} catch (const ibrcommon::QueueUnblockedException &ex) {
				IBRCOMMON_LOGGER_DEBUG(40) << "BinaryStreamClient::Sender::run(): aborted" << IBRCOMMON_LOGGER_ENDL;
				return;
			} catch (const ibrcommon::IOException &ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "API: IOException says " << ex.what() << IBRCOMMON_LOGGER_ENDL;
			} catch (const dtn::InvalidDataException &ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "API: InvalidDataException says " << ex.what() << IBRCOMMON_LOGGER_ENDL;
			} catch (const std::exception &ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "unexpected API error! " << ex.what() << IBRCOMMON_LOGGER_ENDL;
			}
		}

		void BinaryStreamClient::queue(const dtn::data::Bundle &bundle)
		{
			_sender.push(bundle);
		}
	}
}

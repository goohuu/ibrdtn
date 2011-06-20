/*
 * Client.cpp
 *
 *  Created on: 24.06.2009
 *      Author: morgenro
 */




#include "ibrdtn/api/Client.h"
#include "ibrdtn/api/Bundle.h"
#include "ibrdtn/data/SDNV.h"
#include "ibrdtn/data/Exceptions.h"
#include "ibrdtn/streams/StreamDataSegment.h"
#include "ibrdtn/streams/StreamContactHeader.h"

#include <ibrcommon/net/tcpstream.h>
#include <ibrcommon/Logger.h>

using namespace dtn::data;
using namespace dtn::streams;

namespace dtn
{
	namespace api
	{
		Client::AsyncReceiver::AsyncReceiver(Client &client)
		 : _client(client), _running(true)
		{
			// enable exception throwing on stream events
			_client.exceptions(std::ios::badbit | std::ios::eofbit);
		}

		Client::AsyncReceiver::~AsyncReceiver()
		{
		}

		bool Client::AsyncReceiver::__cancellation()
		{
			_running = false;
			return true;
		}

		void Client::AsyncReceiver::run()
		{
			try {
				while (!_client.eof() && _running)
				{
					dtn::api::Bundle b;
					_client >> b;
					_client.received(b);
					yield();
				}
			} catch (const dtn::api::ConnectionException &ex) {
				IBRCOMMON_LOGGER(error) << "Client::AsyncReceiver - ConnectionException: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
				_client.shutdown(CONNECTION_SHUTDOWN_ERROR);
			} catch (const dtn::streams::StreamConnection::StreamErrorException &ex) {
				IBRCOMMON_LOGGER(error) << "Client::AsyncReceiver - StreamErrorException: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
				_client.shutdown(CONNECTION_SHUTDOWN_ERROR);
			} catch (const ibrcommon::IOException &ex) {
				IBRCOMMON_LOGGER(error) << "Client::AsyncReceiver - IOException: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
				_client.shutdown(CONNECTION_SHUTDOWN_ERROR);
			} catch (const dtn::InvalidDataException &ex) {
				IBRCOMMON_LOGGER(error) << "Client::AsyncReceiver - InvalidDataException: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
				_client.shutdown(CONNECTION_SHUTDOWN_ERROR);
			} catch (const std::exception&) {
				IBRCOMMON_LOGGER(error) << "error" << IBRCOMMON_LOGGER_ENDL;
				_client.shutdown(CONNECTION_SHUTDOWN_ERROR);
			}
		}

		Client::Client(const std::string &app, ibrcommon::tcpstream &stream, const COMMUNICATION_MODE mode)
		  : StreamConnection(*this, stream), _stream(stream), _mode(mode), _app(app), _receiver(*this)
		{
		}

		Client::~Client()
		{
			try {
				// stop the receiver
				_receiver.stop();
			} catch (const ibrcommon::ThreadException &ex) {
				IBRCOMMON_LOGGER_DEBUG(20) << "ThreadException in Client destructor: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
			}
			
			// Close the stream. This releases all reading or writing threads.
			_stream.close();

			// wait until the async thread has been finished
			_receiver.join();
		}

		void Client::connect()
		{
			// do a handshake
			EID localeid(EID("api:" + _app));

			// connection flags
			char flags = 0;

			// request acknowledgements
			flags |= dtn::streams::StreamContactHeader::REQUEST_ACKNOWLEDGMENTS;

			// set comm. mode
			if (_mode == MODE_SENDONLY) flags |= HANDSHAKE_SENDONLY;

			// do the handshake (no timeout, no keepalive)
			handshake(localeid, 0, flags);

			try {
				// run the receiver
				_receiver.start();
			} catch (const ibrcommon::ThreadException &ex) {
				IBRCOMMON_LOGGER(error) << "failed to start Client::Receiver\n" << ex.what() << IBRCOMMON_LOGGER_ENDL;
			}
		}

		void Client::close()
		{
			// shutdown the bundle stream connection
			shutdown(StreamConnection::CONNECTION_SHUTDOWN_SIMPLE_SHUTDOWN);
		}

		void Client::eventConnectionDown()
		{
			_inqueue.abort();

			try {
				_receiver.stop();
			} catch (const ibrcommon::ThreadException &ex) {
				IBRCOMMON_LOGGER_DEBUG(20) << "ThreadException in Client::eventConnectionDown: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
			}
		}

		void Client::eventBundleAck(size_t ack)
		{
			lastack = ack;
		}

		void Client::received(const dtn::api::Bundle &b)
		{
			// if we are in send only mode...
			if (_mode != dtn::api::Client::MODE_SENDONLY)
			{
				// ... then discard the received bundle
				_inqueue.push(b);
			}
		}

		dtn::api::Bundle Client::getBundle(size_t timeout) throw (ConnectionException)
		{
			try {
				return _inqueue.getnpop(true, timeout * 1000);
			} catch (const ibrcommon::QueueUnblockedException &ex) {
				if (ex.reason == ibrcommon::QueueUnblockedException::QUEUE_TIMEOUT)
				{
					throw ConnectionTimeoutException();
				}
				else if (ex.reason == ibrcommon::QueueUnblockedException::QUEUE_ABORT)
				{
					throw ConnectionAbortedException(ex.what());
				}

				throw ConnectionException(ex.what());
			} catch (const std::exception &ex) {
				throw ConnectionException(ex.what());
			}
		}
	}
}

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
		 : _client(client)
		{
			_client.exceptions(std::ios::badbit | std::ios::eofbit);
		}

		Client::AsyncReceiver::~AsyncReceiver()
		{
			join();
		}

		bool Client::AsyncReceiver::__cancellation()
		{
			// return false, to signal that further cancel (the hardway) is needed
			return false;
		}

		void Client::AsyncReceiver::run()
		{
			try {
				while (!_client.eof())
				{
					dtn::api::Bundle b;
					_client >> b;
					_client.received(b);
					yield();
				}
			} catch (const dtn::api::ConnectionException &ex) {
				IBRCOMMON_LOGGER(error) << "Client::AsyncReceiver - ConnectionException: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
				_client.shutdown(CONNECTION_SHUTDOWN_ERROR);
			} catch (dtn::streams::StreamConnection::StreamErrorException &ex) {
				IBRCOMMON_LOGGER(error) << "Client::AsyncReceiver - StreamErrorException: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
				_client.shutdown(CONNECTION_SHUTDOWN_ERROR);
			} catch (ibrcommon::IOException ex) {
				IBRCOMMON_LOGGER(error) << "Client::AsyncReceiver - IOException: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
				_client.shutdown(CONNECTION_SHUTDOWN_ERROR);
			} catch (dtn::InvalidDataException ex) {
				IBRCOMMON_LOGGER(error) << "Client::AsyncReceiver - InvalidDataException: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
				_client.shutdown(CONNECTION_SHUTDOWN_ERROR);
			} catch (std::exception) {
				IBRCOMMON_LOGGER(error) << "error" << IBRCOMMON_LOGGER_ENDL;
				_client.shutdown(CONNECTION_SHUTDOWN_ERROR);
			}
		}

		Client::Client(COMMUNICATION_MODE mode, string app, ibrcommon::tcpstream &stream)
		  : StreamConnection(*this, stream), _stream(stream), _mode(mode), _app(app), _connected(false), _receiver(*this)
		{
		}

		Client::Client(string app, ibrcommon::tcpstream &stream)
		  : StreamConnection(*this, stream), _stream(stream), _mode(MODE_BIDIRECTIONAL), _app(app), _connected(false), _receiver(*this)
		{
		}

		Client::~Client()
		{
			try {
				_receiver.stop();
			} catch (const ibrcommon::ThreadException &ex) {
				IBRCOMMON_LOGGER_DEBUG(20) << "ThreadException in Client destructor: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
			}

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

		bool Client::isConnected()
		{
			return _connected;
		}

		void Client::close()
		{
			shutdown(StreamConnection::CONNECTION_SHUTDOWN_SIMPLE_SHUTDOWN);
		}

		void Client::received(const StreamContactHeader&)
		{
			_connected = true;
		}

		void Client::eventTimeout()
		{
		}

		void Client::eventShutdown(StreamConnection::ConnectionShutdownCases csc)
		{
		}

		void Client::eventConnectionUp(const StreamContactHeader&)
		{
		}

		void Client::eventError()
		{
		}

		void Client::eventConnectionDown()
		{
			_connected = false;
			_inqueue.abort();

			try {
				_receiver.stop();
			} catch (const ibrcommon::ThreadException &ex) {
				IBRCOMMON_LOGGER_DEBUG(20) << "ThreadException in Client::eventConnectionDown: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
			}
		}

		void Client::eventBundleRefused()
		{

		}

		void Client::eventBundleForwarded()
		{

		}

		void Client::eventBundleAck(size_t ack)
		{
			lastack = ack;
		}

		void Client::received(const dtn::api::Bundle &b)
		{
			if (_mode != dtn::api::Client::MODE_SENDONLY)
			{
				_inqueue.push(b);
			}
		}

		dtn::api::Bundle Client::getBundle(size_t timeout)
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

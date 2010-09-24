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

#include "ibrcommon/net/tcpstream.h"
#include "ibrdtn/streams/StreamDataSegment.h"
#include "ibrdtn/streams/StreamContactHeader.h"

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
		}

		Client::AsyncReceiver::~AsyncReceiver()
		{
			join();
		}

		void Client::AsyncReceiver::run()
		{
			try {
				while (true)
				{
					dtn::api::Bundle b;
					_client >> b;
					_client.received(b);
					testcancel();
					yield();
				}
			} catch (dtn::api::ConnectionException ex) {
				IBRCOMMON_LOGGER(error) << "Client::AsyncReceiver: ConnectionException" << IBRCOMMON_LOGGER_ENDL;
				_client.shutdown(CONNECTION_SHUTDOWN_ERROR);
			} catch (ibrcommon::IOException ex) {
				IBRCOMMON_LOGGER(error) << "Client::AsyncReceiver: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
				_client.shutdown(CONNECTION_SHUTDOWN_ERROR);
			} catch (dtn::InvalidDataException ex) {
				IBRCOMMON_LOGGER(error) << "Client::AsyncReceiver: InvalidDataException" << IBRCOMMON_LOGGER_ENDL;
				_client.shutdown(CONNECTION_SHUTDOWN_ERROR);
			} catch (std::exception) {
				IBRCOMMON_LOGGER(error) << "error" << IBRCOMMON_LOGGER_ENDL;
				_client.shutdown(CONNECTION_SHUTDOWN_ERROR);
			}
		}

		void Client::AsyncReceiver::finally()
		{
			_client.shutdown();
		}

		Client::Client(COMMUNICATION_MODE mode, string app, ibrcommon::tcpstream &stream)
		  : StreamConnection(*this, stream), _stream(stream), _mode(mode), _app(app), _connected(false), _receiver(*this)
		{
			stream.enableNoDelay();
		}

		Client::Client(string app, ibrcommon::tcpstream &stream)
		  : StreamConnection(*this, stream), _stream(stream), _mode(MODE_BIDIRECTIONAL), _app(app), _connected(false), _receiver(*this)
		{
			stream.enableNoDelay();
		}

		Client::~Client()
		{
			close();
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

			// do the handshake
			handshake(localeid, 10, flags);

			// run the receiver
			_receiver.start();
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

		void Client::eventShutdown()
		{
		}

		void Client::eventConnectionUp(const StreamContactHeader &header)
		{
		}

		void Client::eventError()
		{
		}

		void Client::eventConnectionDown()
		{
			_connected = false;
			_inqueue.unblock();
		}

		void Client::eventBundleRefused()
		{

		}

		void Client::eventBundleForwarded()
		{

		}

		void Client::eventBundleAck(size_t)
		{
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
				return _inqueue.blockingpop(timeout * 1000);
			} catch (std::exception ex) {
				throw ibrcommon::ConnectionClosedException(ex.what());
			}
		}
	}
}

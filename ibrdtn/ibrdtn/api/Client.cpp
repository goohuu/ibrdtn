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
		 : _client(client), _shutdown(false)
		{
		}

		Client::AsyncReceiver::~AsyncReceiver()
		{
			_shutdown = true;
			join();
		}

		void Client::AsyncReceiver::run()
		{
			try {
				while (!_shutdown)
				{
					dtn::api::Bundle b;
					_client >> b;
					_client.received(b);
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
			} catch (...) {
				IBRCOMMON_LOGGER(error) << "unknown error" << IBRCOMMON_LOGGER_ENDL;
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
			close();
		}

		void Client::connect()
		{
			// do a handshake
			EID localeid(EID("dtn:local/" + _app));

			// connection flags
			char flags = 0;

			// request acknowledgements
			flags |= dtn::streams::StreamContactHeader::REQUEST_ACKNOWLEDGMENTS;

			// set comm. mode
			if (_mode == MODE_SENDONLY) flags |= 0x80;

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
			StreamConnection::shutdown();

			ibrcommon::MutexLock l(_inqueue);
			_inqueue.signal(true);
		}

		void Client::received(const StreamContactHeader &h)
		{
			_connected = true;
		}

		void Client::eventTimeout()
		{
			_stream.done();
			_stream.close();

			ibrcommon::MutexLock l(_inqueue);
			_inqueue.signal(true);
		}

		void Client::eventShutdown()
		{
			_stream.done();
			_stream.close();

			ibrcommon::MutexLock l(_inqueue);
			_inqueue.signal(true);
		}

		void Client::eventConnectionUp(const StreamContactHeader &header)
		{
			// call received method
			received(header);

			// set connected to true
			_connected = true;

			ibrcommon::MutexLock l(_inqueue);
			_inqueue.signal(true);
		}

		void Client::eventError()
		{
			_stream.close();

			ibrcommon::MutexLock l(_inqueue);
			_inqueue.signal(true);
		}

		void Client::eventConnectionDown()
		{
			_connected = false;

			ibrcommon::MutexLock l(_inqueue);
			_inqueue.signal(true);
		}

		void Client::received(const dtn::api::Bundle &b)
		{
			if (_mode != dtn::api::Client::MODE_SENDONLY)
			{
				_inqueue.push(b);
			}
		}

		dtn::api::Bundle Client::getBundle()
		{
			try {
				return _inqueue.blockingpop();
			} catch (ibrcommon::Exception ex) {
				throw ibrcommon::ConnectionClosedException();
			}
		}
	}
}

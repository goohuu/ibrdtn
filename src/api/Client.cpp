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
				_client.close();
			} catch (dtn::exceptions::IOException ex) {
#ifdef DO_EXTENDED_DEBUG_OUTPUT
				cout << ex.what() << endl;
#endif
				_client.close();
			}

		}

		Client::Client(COMMUNICATION_MODE mode, string app, ibrcommon::tcpstream &stream, bool async)
		  : StreamConnection(*this, stream), _stream(stream), _mode(mode), _app(app), _connected(false), _async(async), _receiver(*this)
		{
		}

		Client::Client(string app, ibrcommon::tcpstream &stream, bool async)
		  : StreamConnection(*this, stream), _stream(stream), _mode(MODE_BIDIRECTIONAL), _app(app), _connected(false), _async(async), _receiver(*this)
		{
		}

		Client::~Client()
		{
			// wait for the closed connection
			wait();
			_stream.close();
		}

		void Client::connect()
		{
			// do a handshake
			EID localeid(EID("dtn:local/" + _app));

			// connection flags
			char flags = 0;

			// set comm. mode
			if (_mode == MODE_SENDONLY) flags += 0x80;

			// do the handshake
			handshake(localeid, 10, flags);

			// run the receiver
			if (_async) _receiver.start();
		}

		bool Client::isConnected()
		{
			return _connected;
		}

		void Client::received(const StreamContactHeader &h)
		{
			_connected = true;
		}

		void Client::eventTimeout()
		{
			_connected = false;
			_stream.done();
		}

		void Client::eventShutdown()
		{
			_connected = false;
			_stream.done();
		}

		void Client::eventConnectionUp(const StreamContactHeader &header)
		{
			// call received method
			received(header);

			// set connected to true
			_connected = true;
		}
	}
}

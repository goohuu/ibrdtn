/*
 * Client.cpp
 *
 *  Created on: 24.06.2009
 *      Author: morgenro
 */




#include "ibrdtn/api/Client.h"
#include "ibrdtn/api/Bundle.h"
#include "ibrdtn/data/SDNV.h"
#include "ibrcommon/data/BLOBManager.h"
#include "ibrdtn/data/Exceptions.h"

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
				_client.shutdown();
			} catch (dtn::exceptions::IOException ex) {
				cout << ex.what() << endl;
				_client.shutdown();
			}

		}

		Client::Client(COMMUNICATION_MODE mode, string app, iostream &stream, bool async)
		  : StreamConnection(stream), _mode(mode), _app(app), _connected(false), _async(async), _receiver(*this)
		{
		}

		Client::Client(string app, iostream &stream, bool async)
		  : StreamConnection(stream), _mode(MODE_BIDIRECTIONAL), _app(app), _connected(false), _async(async), _receiver(*this)
		{
		}

		Client::~Client()
		{
		}

		void Client::connect()
		{
			// do a handshake
			StreamContactHeader header(EID("dtn:local/" + _app));

			// set comm. mode
			if ((_mode == MODE_SENDONLY) && !(header._flags & 0x80))
				header._flags += 0x80;

			// transmit the header
			(*this) << header;

			// read the header
			(*this) >> _header;

			// call received method
			received(_header);

			// set connected to true
			_connected = true;

			// run myself
			start();

			// run the receiver
			if (_async) _receiver.start();
		}

		bool Client::isConnected()
		{
			return _connected;
		}

		void Client::received(StreamContactHeader &h)
		{
			_connected = true;
		}

		void Client::shutdown()
		{
			_connected = false;
			StreamConnection::shutdown();

			// wait for the closed connection
			StreamConnection::waitState(StreamConnection::CONNECTION_CLOSED);
		}
	}
}

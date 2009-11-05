/*
 * Client.cpp
 *
 *  Created on: 24.06.2009
 *      Author: morgenro
 */




#include "ibrdtn/api/Client.h"
#include "ibrdtn/api/Bundle.h"
#include "ibrdtn/data/SDNV.h"
#include "ibrdtn/data/BLOBManager.h"
#include "ibrdtn/data/Exceptions.h"

#include "ibrdtn/streams/StreamDataSegment.h"
#include "ibrdtn/streams/StreamContactHeader.h"

using namespace dtn::blob;
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
			}
		}

		Client::Client(string app, iostream &stream, bool async)
		  : StreamConnection(stream, 0), _app(app), _connected(false), _receiver(*this)
		{
			if (async)
			{
				_receiver.start();
			}
		}

		Client::~Client()
		{
		}

		void Client::connect()
		{
			// do a handshake
			StreamContactHeader header(EID("dtn:local/" + _app));

			// transmit the header
			(*this) << header;

			// read the header
			(*this) >> _header;

			// run myself
			start();
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
		}
	}
}

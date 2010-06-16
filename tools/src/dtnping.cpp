/*
 * dtnping.cpp
 *
 *  Created on: 24.06.2009
 *      Author: morgenro
 */

#include "config.h"
#include "ibrdtn/api/Client.h"
#include "ibrdtn/api/StringBundle.h"
#include "ibrcommon/net/tcpclient.h"
#include "ibrcommon/thread/Mutex.h"
#include "ibrcommon/thread/MutexLock.h"
#include "ibrcommon/TimeMeasurement.h"

#include <iostream>

class EchoClient : public dtn::api::Client
{
	public:
		EchoClient(dtn::api::Client::COMMUNICATION_MODE mode, string app, ibrcommon::tcpstream &stream)
		 : dtn::api::Client(mode, app, stream), _stream(stream)
		{
		}

		virtual ~EchoClient()
		{
		}

		dtn::api::Bundle reply()
		{
			dtn::api::Bundle b = (*this).getBundle();

			// return the bundle
			return b;
		}

		void echo(EID destination, int size, int lifetime)
		{
			// create a bundle
			dtn::api::StringBundle b(destination);

			// set lifetime
			b.setLifetime(lifetime);

			// create testing pattern
			char pattern[2000];
			for (int i = 0; i < 2000; i++)
			{
				pattern[i] = '0';
				pattern[i] += i % 10;
			}

			// create data blob
			for (int i = 0; i < size; i += 2000)
			{
				if (i < (size - 2000))
				{
					b.append(string(pattern, 2000));
				}
				else
				{
					b.append(string(pattern, size - i));
				}
			}

			// send the bundle
			(*this) << b;

			// ... flush out
			flush();
		}

	private:
		ibrcommon::tcpstream &_stream;
};

void print_help()
{
	cout << "-- dtnping (IBR-DTN) --" << endl;
	cout << "Syntax: dtnping [options] <dst>"  << endl;
	cout << " <dst>    set the destination eid (e.g. dtn://node/echo)" << endl;
	cout << "* optional parameters *" << endl;
	cout << " -h|--help       display this text" << endl;
	cout << " --src <name>    set the source application name (e.g. echo-client)" << endl;
	cout << " --nowait        do not wait for a reply" << endl;
	cout << " --size          the size of the payload" << endl;
	cout << " --count X       send X echo in a row" << endl;
	cout << " --lifetime <seconds> set the lifetime of outgoing bundles; default: 30" << endl;

}

int main(int argc, char *argv[])
{
	string ping_destination = "dtn://local/echo";
	string ping_source = "echo-client";
	int ping_size = 64;
	unsigned int lifetime = 30;
	bool wait_for_reply = true;
	size_t count = 1;
	dtn::api::Client::COMMUNICATION_MODE mode = dtn::api::Client::MODE_BIDIRECTIONAL;

	if (argc == 1)
	{
		print_help();
		return 0;
	}

	for (int i = 0; i < argc; i++)
	{
		string arg = argv[i];

		// print help if requested
		if (arg == "-h" || arg == "--help")
		{
			print_help();
			return 0;
		}

		if (arg == "--nowait")
		{
			mode = dtn::api::Client::MODE_SENDONLY;
			wait_for_reply = false;
		}

		if (arg == "--src" && argc > i)
		{
			ping_source = argv[i + 1];
		}

		if (arg == "--size" && argc > i)
		{
			stringstream str_size;
			str_size.str( argv[i + 1] );
			str_size >> ping_size;
		}

		if (arg == "--count" && argc > i)
		{
			stringstream str_count;
			str_count.str( argv[i + 1] );
			str_count >> count;
		}

		if (arg == "--lifetime" && argc > i)
		{
			stringstream data; data << argv[i + 1];
			data >> lifetime;
		}
	}

	size_t bundlecounter = 0;

	// the last parameter is always the destination
	ping_destination = argv[argc - 1];

	ibrcommon::TimeMeasurement tm;

	try {
		// Create a stream to the server using TCP.
		ibrcommon::tcpclient conn("127.0.0.1", 4550);

		// Initiate a derivated client
		EchoClient client(mode, ping_source, conn);

		// Connect to the server. Actually, this function initiate the
		// stream protocol by starting the thread and sending the contact header.
		client.connect();

		// target address
		EID addr = EID(ping_destination);

		try {
			for (unsigned int i = 0; i < count; i++)
			{
				cout << "ECHO: " << addr.getNodeEID() << " ..."; cout.flush();

				// set sending time
				tm.start();

				// Call out a ECHO
				client.echo( addr, ping_size, lifetime );

				if (wait_for_reply)
				{
					// Get the echo reply. This method blocks
					dtn::api::Bundle relpy = client.reply();

					// set receiving time
					tm.stop();

					// print out measurement result
					cout << tm << endl;

					bundlecounter++;
				}
				else
					cout << endl;
			}
		} catch (dtn::api::ConnectionException ex) {
			cout << "Disconnected." << endl;
		} catch (ibrcommon::IOException ex) {
			cout << "Error while receiving a bundle." << endl;
		}

		// Shutdown the client connection.
		client.close();
		conn.close();

	} catch (ibrcommon::tcpclient::SocketException ex) {
		cerr << "Can not connect to the daemon. Does it run?" << endl;
		return -1;
	} catch (...) {

	}

	if (bundlecounter > 1)
	{
		std::cout << bundlecounter << " bundles received" << std::endl;
	}

	return 0;
}

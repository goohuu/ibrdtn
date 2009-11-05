/*
 * dtnping.cpp
 *
 *  Created on: 24.06.2009
 *      Author: morgenro
 */

#include "ibrdtn/api/Client.h"
#include "ibrdtn/api/StringBundle.h"
#include "ibrdtn/utils/tcpclient.h"
#include "ibrdtn/utils/Mutex.h"
#include "ibrdtn/utils/MutexLock.h"

#include <time.h>

#include <iostream>

class EchoClient : public dtn::api::Client
{
	public:
		EchoClient(string app, std::iostream &stream)
		 : dtn::api::Client(app, stream, false)
		{
		}

		virtual ~EchoClient()
		{
		}

		dtn::api::Bundle reply()
		{
			dtn::api::Bundle b;

			(*this) >> b;

			// return the bundle
			return b;
		}

		void echo(EID destination, int size)
		{
			// create a bundle
			dtn::api::StringBundle b(destination);

			char data[size];
			for (int i = 0; i < size; i++)
			{
				data[i] = i % 10;
				data[i] += '0';
			}

			// some data
			b.append(string(data, size));

			// send the bundle
			(*this) << b;

			// ... flush out
			flush();
		}
};


int64_t timespecDiff(struct timespec *timeA_p, struct timespec *timeB_p)
{
  return ((timeA_p->tv_sec * 1000000000) + timeA_p->tv_nsec) -
           ((timeB_p->tv_sec * 1000000000) + timeB_p->tv_nsec);
}

double Round(double Zahl, int Stellen)
{
    double v[] = { 1, 10, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8 };
    return floor(Zahl * v[Stellen] + 0.5) / v[Stellen];
}

void print_help()
{
	cout << "-- dtnping (IBR-DTN) --" << endl;
	cout << "Syntax: dtnping [options] <dst>"  << endl;
	cout << " <dst>    set the destination eid (e.g. dtn://node/echo)" << endl;
	cout << "* optional parameters *" << endl;
	cout << " -h|--help       display this text" << endl;
	cout << " --src <eid>     set the source application name (e.g. echo-client)" << endl;
	cout << " --nowait        do not wait for a reply" << endl;
	cout << " --size          the size of the payload" << endl;
	cout << " --count X       send X echo in a row" << endl;

}

int main(int argc, char *argv[])
{
	string ping_destination = "dtn://local/echo";
	string ping_source = "echo-client";
	int ping_size = 64;
	bool wait_for_reply = true;
	size_t count = 1;

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
	}

	// the last parameter is always the destination
	ping_destination = argv[argc - 1];

	struct timespec start, end;

	try {
		// Create a stream to the server using TCP.
		dtn::utils::tcpclient conn("127.0.0.1", 4550);

		// Initiate a derivated client
		EchoClient client(ping_source, conn);

		// Connect to the server. Actually, this function initiate the
		// stream protocol by starting the thread and sending the contact header.
		client.connect();

		// target address
		EID addr = EID(ping_destination);

		try {
			for (int i = 0; i < count; i++)
			{
				cout << "ECHO: " << addr.getNodeEID() << " ..."; cout.flush();

				// set sending time
				clock_gettime(CLOCK_MONOTONIC, &start);

				// Call out a ECHO
				client.echo( addr, ping_size );

				if (wait_for_reply)
				{
					// Get the echo reply. This method blocks
					dtn::api::Bundle relpy = client.reply();

					//We are DOOOOMED
					//http://www.wand.net.nz/~smr26/wordpress/2009/01/19/monotonic-time-in-mac-os-x/
					// set receiving time
					clock_gettime(CLOCK_MONOTONIC, &end);

					// calc difference
					uint64_t timeElapsed = timespecDiff(&end, &start);

					// make it readable
					double delay_ms = (double)timeElapsed / 1000000;
					double delay_sec = delay_ms / 1000;
					double delay_min = delay_sec / 60;
					double delay_h = delay_min / 60;

					if (delay_h > 1)
					{
						cout << " " << Round(delay_h, 2) << " h" << endl;
					}
					else if (delay_min > 1)
					{
						cout << " " << Round(delay_min, 2) << " m" << endl;
					}
					else if (delay_sec > 1)
					{
						cout << " " << Round(delay_sec, 2) << " s." << endl;
					}
					else
					{
						cout << " " << Round(delay_ms, 2) << " ms" << endl;
					}
				}
				else
					cout << endl;
			}
		} catch (dtn::api::ConnectionException ex) {
			cout << "Disconnected." << endl;
		} catch (dtn::exceptions::IOException ex) {
			cout << "Error while receiving a bundle." << endl;
		}

		// Shutdown the client connection.
		client.shutdown();

		conn.close();

	} catch (dtn::utils::tcpclient::SocketException ex) {
		cerr << "Can not connect to the daemon. Does it run?" << endl;
		return -1;
	}

	return 0;
}

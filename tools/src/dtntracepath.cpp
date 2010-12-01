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

#include <algorithm>
#include <iostream>

class Tracer : public dtn::api::Client
{
	public:
		Tracer(dtn::api::Client::COMMUNICATION_MODE mode, string app, ibrcommon::tcpstream &stream)
		 : dtn::api::Client(app, stream, mode), _stream(stream)
		{
		}

		virtual ~Tracer()
		{
		}

		void tracepath(const dtn::data::EID &destination)
		{
			// create a bundle
			dtn::api::StringBundle b(destination);

			// set lifetime
			b.setLifetime(30);

			// set some stupid payload
			b.append("follow the white rabbit");

			// request forward and delivery reports
			b.requestDeliveredReport();
			b.requestForwardedReport();
			b.requestDeletedReport();

			b.setReportTo( EID("api:me") );

			ibrcommon::TimeMeasurement tm;

			std::list<dtn::api::Bundle> bundles;

			try {
				// start the timer
				tm.start();

				// send the bundle
				(*this) << b << std::flush;

				try {
					// now receive all incoming bundles
					while (true)
					{
						dtn::api::Bundle recv = getBundle(10);
						bundles.push_back(recv);
						tm.stop();

						std::cout << "Report received after " << tm << std::endl;
					}
				} catch (std::exception) {

				}

				std::cout << "No more reports received." << std::endl;

				// sort the list
				bundles.sort();

				// print out each hop
				for (std::list<dtn::api::Bundle>::iterator iter = bundles.begin(); iter != bundles.end(); iter++)
				{
					std::cout << "Hop: " << (*iter).getSource().getString() << std::endl;
				}

			} catch (dtn::api::ConnectionException ex) {
				cout << "Disconnected." << endl;
			} catch (ibrcommon::IOException ex) {
				cout << "Error while receiving a bundle." << endl;
			}
		}

	private:
		ibrcommon::tcpstream &_stream;
};

void print_help()
{
	cout << "-- dtntracepath (IBR-DTN) --" << endl;
	cout << "Syntax: dtntracepath [options] <dst>"  << endl;
	cout << " <dst>    set the destination eid (e.g. dtn://node/echo)" << endl;
	cout << "* optional parameters *" << endl;
	cout << " -h|--help       display this text" << endl;
//	cout << " --src <name>    set the source application name (e.g. echo-client)" << endl;
//	cout << " --nowait        do not wait for a reply" << endl;
//	cout << " --size          the size of the payload" << endl;
//	cout << " --count X       send X echo in a row" << endl;
//	cout << " --lifetime <seconds> set the lifetime of outgoing bundles; default: 30" << endl;

}

int main(int argc, char *argv[])
{
	std::string trace_destination = "dtn://local";
	std::string trace_source = "tracepath";
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

//		if (arg == "--nowait")
//		{
//			mode = dtn::api::Client::MODE_SENDONLY;
//			wait_for_reply = false;
//		}
//
//		if (arg == "--src" && argc > i)
//		{
//			ping_source = argv[i + 1];
//		}
//
//		if (arg == "--size" && argc > i)
//		{
//			stringstream str_size;
//			str_size.str( argv[i + 1] );
//			str_size >> ping_size;
//		}
//
//		if (arg == "--count" && argc > i)
//		{
//			stringstream str_count;
//			str_count.str( argv[i + 1] );
//			str_count >> count;
//		}
//
//		if (arg == "--lifetime" && argc > i)
//		{
//			stringstream data; data << argv[i + 1];
//			data >> lifetime;
//		}
	}

	// the last parameter is always the destination
	trace_destination = argv[argc - 1];

	try {
		// Create a stream to the server using TCP.
		ibrcommon::tcpclient conn("127.0.0.1", 4550);

		// enable nodelay option
		conn.enableNoDelay();

		// Initiate a derivated client
		Tracer tracer(mode, trace_source, conn);

		// Connect to the server. Actually, this function initiate the
		// stream protocol by starting the thread and sending the contact header.
		tracer.connect();

		// target address
		tracer.tracepath(trace_destination);

		// Shutdown the client connection.
		tracer.close();
		conn.close();

	} catch (ibrcommon::tcpclient::SocketException ex) {
		cerr << "Can not connect to the daemon. Does it run?" << endl;
		return -1;
	} catch (...) {

	}

	return 0;
}

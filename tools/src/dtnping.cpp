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
#include <csignal>
#include <stdint.h>

#define CREATE_CHUNK_SIZE 2048

class EchoClient : public dtn::api::Client
{
	public:
		EchoClient(dtn::api::Client::COMMUNICATION_MODE mode, string app,  ibrcommon::tcpstream &stream)
		 : dtn::api::Client(mode, app, stream), _stream(stream)
		{
			seq=0;
		}

		virtual ~EchoClient()
		{
		}

		const dtn::api::Bundle waitForReply(const int timeout)
		{
			double wait=(timeout*1000);
			ibrcommon::TimeMeasurement tm;
			while ( wait > 0)
			{
				try {
					tm.start();
					dtn::api::Bundle b = this->getBundle((int)(wait/1000));
					tm.stop();
					checkReply(b);
					return b;
				} catch (const ibrcommon::QueueUnblockedException&) {
					throw ibrcommon::Exception("timeout reached");
				} catch (const std::string &errmsg) {
					std::cerr << errmsg << std::endl;
				}
				wait=wait-tm.getMilliseconds();
			}
			throw ibrcommon::Exception("timeout is set to zero");
		}

		void echo(EID destination, int size, int lifetime)
		{
			lastdestination=destination.getString();
			seq++;
			
			// create a bundle
			dtn::api::StringBundle b(destination);

			// set lifetime
			b.setLifetime(lifetime);
			
			//Add magic seqnr. Hmm, how to to do this without string?
			b.append(string((char *)(&seq),4));
			size-=4;
			
			// create testing pattern, chunkwise to ocnserve memory
			char pattern[CREATE_CHUNK_SIZE];
			for (size_t i = 0; i < sizeof(pattern); i++)
			{
				pattern[i] = '0';
				pattern[i] += i % 10;
			}
			string chunk=string(pattern,CREATE_CHUNK_SIZE);

			while (size > CREATE_CHUNK_SIZE) {
				b.append(chunk);
				size-=CREATE_CHUNK_SIZE;
			}
			b.append(chunk.substr(0,size));
			
			

			// send the bundle
			(*this) << b;

			// ... flush out
			flush();
			
		}
		
		void checkReply(dtn::api::Bundle &bundle) {
			size_t reply_seq = 0;
			ibrcommon::BLOB::Reference blob = bundle.getData();
			ibrcommon::MutexLock l(blob);
			(*blob).read((char *)(&reply_seq),4 );

			if (reply_seq != seq) {
				std::stringstream ss;
				ss << "sequence number mismatch, awaited " << seq << ", got " << reply_seq;
				throw ss.str();
			}
			if (bundle.getSource().getString() != lastdestination) {
				throw std::string("ignoring bundle from source " + bundle.getSource().getString() + " awaited " + lastdestination);
			}
		}

	private:
		ibrcommon::tcpstream &_stream;
		uint32_t seq;
		string lastdestination;
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
	cout << " --abortfail     Abort after first packetloss" << endl;
	cout << " --size          the size of the payload" << endl;
	cout << " --count X       send X echo in a row" << endl;
	cout << " --lifetime <seconds> set the lifetime of outgoing bundles; default: 30" << endl;
	cout << " -U <socket>     use UNIX domain sockets" << endl;
}

size_t _received = 0, _transmitted = 0;
float _min = 0.0, _max = 0.0, _avg = 0.0;
ibrcommon::TimeMeasurement _runtime;

EID _addr;
bool __exit = false;

void print_summary()
{
	_runtime.stop();

	float loss = 0; if (_transmitted > 0) loss = ((_transmitted - _received) / _transmitted) * 100.0;
	float avg_value = 0; if (_received > 0) avg_value = (_avg/_received);

	std::cout << std::endl << "--- " << _addr.getString() << " echo statistics --- " << std::endl;
	std::cout << _transmitted << " bundles transmitted, " << _received << " received, " << loss << "% bundle loss, time " << _runtime << std::endl;
	std::cout << "rtt min/avg/max = " << _min << "/" << _max << "/" << avg_value << " ms" << std::endl;
}

void term(int signal)
{
	if (signal >= 1)
	{
		if (!__exit)
		{
			print_summary();
			__exit = true;
		}
		exit(0);
	}
}

int main(int argc, char *argv[])
{
	// catch process signals
	signal(SIGINT, term);
	signal(SIGTERM, term);

	string ping_destination = "dtn://local/echo";
	string ping_source = "echo-client";
	int ping_size = 64;
	unsigned int lifetime = 30;
	bool wait_for_reply = true;
	bool stop_after_first_fail = false;
	bool nonstop = true;
	size_t count = 0;
	dtn::api::Client::COMMUNICATION_MODE mode = dtn::api::Client::MODE_BIDIRECTIONAL;
	ibrcommon::File unixdomain;

	if (argc == 1)
	{
		print_help();
		return 0;
	}

	for (int i = 1; i < argc; i++)
	{
		string arg = argv[i];

		// print help if requested
		if ((arg == "-h") || (arg == "--help"))
		{
			print_help();
			return 0;
		}

		else if (arg == "--nowait")
		{
			mode = dtn::api::Client::MODE_SENDONLY;
			wait_for_reply = false;
		}
		
		else if ( arg == "--abortfail") {
			stop_after_first_fail=true;
		}

		else if (arg == "--src" && argc > i)
		{
			ping_source = argv[i + 1];
			i++;
		}

		else if (arg == "--size" && argc > i)
		{
			stringstream str_size;
			str_size.str( argv[i + 1] );
			str_size >> ping_size;
			i++;
		}

		else if (arg == "--count" && argc > i)
		{
			stringstream str_count;
			str_count.str( argv[i + 1] );
			str_count >> count;
			i++;
			nonstop = false;
		}

		else if (arg == "--lifetime" && argc > i)
		{
			stringstream data; data << argv[i + 1];
			data >> lifetime;
			i++;
		}
		else if (arg == "-U" && argc > i)
		{
			if (++i > argc)
			{
					std::cout << "argument missing!" << std::endl;
					return -1;
			}

			unixdomain = ibrcommon::File(argv[i]);
		}
	}

	// the last parameter is always the destination
	ping_destination = argv[argc - 1];

	// target address
	_addr = EID(ping_destination);

	ibrcommon::TimeMeasurement tm;
	
	
	try {
		// Create a stream to the server using TCP.
		ibrcommon::tcpclient conn;

		// check if the unixdomain socket exists
		if (unixdomain.exists())
		{
			// connect to the unix domain socket
			conn.open(unixdomain);
		}
		else
		{
			// connect to the standard local api port
			conn.open("127.0.0.1", 4550);

			// enable nodelay option
			conn.enableNoDelay();
		}

		// Initiate a derivated client
		EchoClient client(mode, ping_source, conn);

		// Connect to the server. Actually, this function initiate the
		// stream protocol by starting the thread and sending the contact header.
		client.connect();

		std::cout << "ECHO " << _addr.getString() << " " << ping_size << " bytes of data." << std::endl;

		// measure runtime
		_runtime.start();

		try {
			for (unsigned int i = 0; (i < count) || nonstop; i++)
			{
				// set sending time
				tm.start();

				// Call out a ECHO
				client.echo( _addr, ping_size, lifetime );
				_transmitted++;
			
				if (wait_for_reply)
				{
					try {
						dtn::api::Bundle response = client.waitForReply(2*lifetime);

						// print out measurement result
						tm.stop();

						size_t reply_seq = 0;
						size_t payload_size = 0;

						// check for min/max/avg
						_avg += tm.getMilliseconds();
						if ((_min > tm.getMilliseconds()) || _min == 0) _min = tm.getMilliseconds();
						if ((_max < tm.getMilliseconds()) || _max == 0) _max = tm.getMilliseconds();

						{
							ibrcommon::BLOB::Reference blob = response.getData();
							ibrcommon::MutexLock l(blob);
							(*blob).read((char *)(&reply_seq),4 );
							payload_size = blob.getSize();
						}

						std::cout << payload_size << " bytes from " << response.getSource().getString() << ": seq=" << reply_seq << " ttl=" << response.getLifetime() << " time=" << tm << std::endl;
						_received++;
					} catch (const ibrcommon::Exception &ex) {
						std::cerr << ex.what() << std::endl;

						if (stop_after_first_fail)
						{
							std::cout << "No response, aborting." << std::endl;
							break;
						}
					}
				}

				if (nonstop) ::sleep(1);
			}
		} catch (dtn::api::ConnectionException ex) {
			std::cerr << "Disconnected." << std::endl;
		} catch (ibrcommon::IOException ex) {
			std::cerr << "Error while receiving a bundle." << std::endl;
		}

		// Shutdown the client connection.
		client.close();
		conn.close();

	} catch (ibrcommon::tcpclient::SocketException ex) {
		std::cerr << "Can not connect to the daemon. Does it run?" << std::endl;
		return -1;
	} catch (...) {

	}

	print_summary();

	return 0;
}

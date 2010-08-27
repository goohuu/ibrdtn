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

#include <stdint.h>

#define CREATE_CHUNK_SIZE 2048

class EchoClient : public dtn::api::Client
{
	public:
		EchoClient(dtn::api::Client::COMMUNICATION_MODE mode, string app,  ibrcommon::tcpstream &stream)
		 : dtn::api::Client(mode, app, stream), _stream(stream)
		{
			seq=0;
			this->app="/"+app; //bug?
		}

		virtual ~EchoClient()
		{
		}

		bool waitForReply(int timeout)
		{
			double wait=(timeout*1000);
			ibrcommon::TimeMeasurement tm;
			while ( wait > 0) {
				try {
					tm.start(); 
					dtn::api::Bundle b = this->getBundle((int)(wait/1000));
					tm.stop();
					if (checkReply(&b))
						return true;
					wait=wait-tm.getMilliseconds();
				}
				catch (ibrcommon::Exception ex) {
					cerr << "Timout." << endl;
					return false;
				} 
			}
			return false;

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
			for (int i = 0; i < sizeof(pattern); i++)
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
		
		bool checkReply(dtn::api::Bundle *bundle) {
			int reply_seq;
			ibrcommon::BLOB::Reference blob=bundle->getData();
			(*blob).read((char *)(&reply_seq),4 );
			//cout << "Reply seq is " << reply_seq << endl;
			if (reply_seq != seq) {
				cerr << "Seq-Nr mismatch, awaited  " << seq << ", got " << reply_seq << endl;
				return false;
			}
			/* Is checked by daemon
			if (bundle->getDestination().getApplication() !=app) {
				cerr << "Ignoring bundle for destination " << bundle->getDestination().getApplication() << " awaited " << app << endl;
				return false;
			}
			*/
			if (bundle->getSource().getString() !=lastdestination) {
				cerr << "Ignoring bundle from source " << bundle->getSource().getString() << " awaited " << lastdestination << endl;
				return false;
			}
			return true;
		}

	private:
		ibrcommon::tcpstream &_stream;
		uint32_t seq;
		string app;
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
	cout << " --size          the size of the payload" << endl;
	cout << " --count X       send X echo in a row" << endl;
	cout << " --lifetime <seconds> set the lifetime of outgoing bundles; default: 30" << endl;

}

EchoClient *client; //for signal cb

int main(int argc, char *argv[])
{
	string ping_destination = "dtn://local/echo";
	string ping_source = "echo-client";
	int ping_size = 64;
	unsigned int lifetime = 30;
	bool wait_for_reply = true;
	size_t count = 1;
	dtn::api::Client::COMMUNICATION_MODE mode = dtn::api::Client::MODE_BIDIRECTIONAL;

	 //signal (SIGALRM, catch_alarm);

	if (argc == 1)
	{
		print_help();
		return 0;
	}

	for (int i = 1; i < argc-1; i++)
	{
		string arg = argv[i];

		// print help if requested
		if (arg == "-h" || arg == "--help")
		{
			print_help();
			return 0;
		}

		else if (arg == "--nowait")
		{
			mode = dtn::api::Client::MODE_SENDONLY;
			wait_for_reply = false;
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
		}

		else if (arg == "--lifetime" && argc > i)
		{
			stringstream data; data << argv[i + 1];
			data >> lifetime;
			i++;
		}
		else {
			cout << "Unknown argument " << arg << endl;
			print_help();
			return -1;
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
		client = new EchoClient(mode, ping_source,  conn);
		


		// Connect to the server. Actually, this function initiate the
		// stream protocol by starting the thread and sending the contact header.
		client->connect();

		// target address
		EID addr = EID(ping_destination);

		try {
			for (unsigned int i = 0; i < count; i++)
			{
				cout << "ECHO: " << addr.getNodeEID() << " ..."; cout.flush();

				// set sending time
				tm.start();

				// Call out a ECHO
				client->echo( addr, ping_size, lifetime );
			
				if (wait_for_reply)
				{
					
					if (client->waitForReply(2*lifetime)) {
					// print out measurement result
						tm.stop();
						cout << tm << endl;
						bundlecounter++;
					}

					
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
		client->close();
		conn.close();

	} catch (ibrcommon::tcpclient::SocketException ex) {
		cerr << "Can not connect to the daemon. Does it run?" << endl;
		return -1;
	} catch (...) {

	}

	float loss = 100.0-(bundlecounter/count)*100.0;
	std::cout << loss << "% loss. " << bundlecounter << " bundles received " << (count-bundlecounter) << " lost."  << std::endl;

	return 0;
}

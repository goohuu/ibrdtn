/*
 * dtnsend.cpp
 *
 *  Created on: 15.10.2009
 *      Author: morgenro
 */

#include "ibrdtn/api/Client.h"
#include "ibrdtn/api/FileBundle.h"
#include "ibrcommon/net/tcpclient.h"
#include "ibrcommon/thread/Mutex.h"
#include "ibrcommon/thread/MutexLock.h"

#include <iostream>

void print_help()
{
	cout << "-- dtnsend (IBR-DTN) --" << endl;
	cout << "Syntax: dtnsend [options] <dst> <filename>"  << endl;
	cout << " <dst>         set the destination eid (e.g. dtn://node/filetransfer)" << endl;
	cout << " <filename>    the file to transfer" << endl;
	cout << "* optional parameters *" << endl;
	cout << " -h|--help       display this text" << endl;
	cout << " --src <name>    set the source application name (e.g. filetransfer)" << endl;
        cout << " --lifetime <seconds> set the lifetime of outgoing bundles; default: 3600" << endl;

}

int main(int argc, char *argv[])
{
	string file_destination = "dtn://local/filetransfer";
	string file_source = "filetransfer";
        unsigned int lifetime = 3600;

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

		if (arg == "--src" && argc > i)
		{
			file_source = argv[i + 1];
		}

		if (arg == "--lifetime" && argc > i)
		{
			stringstream data; data << argv[i + 1];
                        data >> lifetime;
		}
	}

	// the last - 1 parameter is always the destination
	file_destination = argv[argc - 2];

	// the last parameter is always the filename
	string filename = argv[argc -1];

	// Create a stream to the server using TCP.
	ibrcommon::tcpclient conn("127.0.0.1", 4550);

	// Initiate a client for synchronous receiving
	dtn::api::Client client(dtn::api::Client::MODE_SENDONLY, file_source, conn, false);

	// Connect to the server. Actually, this function initiate the
	// stream protocol by starting the thread and sending the contact header.
	client.connect();

	// target address
	EID addr = EID(file_destination);

	cout << "Transfer file \"" << filename << "\" to " << addr.getNodeEID() << endl;

	// create a bundle from the file
	dtn::api::FileBundle b(file_destination, filename);

        // set the lifetime
        b.setLifetime(lifetime);

	// send the bundle
	client << b;

	// flush the buffers
	client.flush();

	// Shutdown the client connection.
	client.close();

	try {
		conn.close();
	} catch (ibrcommon::ConnectionClosedException ex) {

	}

	return 0;
}

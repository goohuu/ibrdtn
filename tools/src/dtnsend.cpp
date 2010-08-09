/*
 * dtnsend.cpp
 *
 *  Created on: 15.10.2009
 *      Author: morgenro
 */

#include "config.h"
#include <ibrdtn/api/Client.h>
#include <ibrdtn/api/FileBundle.h>
#include <ibrdtn/api/BLOBBundle.h>
#include <ibrcommon/net/tcpclient.h>
#include <ibrcommon/thread/Mutex.h>
#include <ibrcommon/thread/MutexLock.h>
#include <ibrcommon/data/BLOB.h>

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
	bool use_stdin = false;
	std::string filename;

	if (argc == 1)
	{
		print_help();
		return 0;
	}
	else if (argc == 2)
	{
		// the last - 1 parameter is always the destination
		file_destination = argv[argc - 1];

		use_stdin = true;
	}
	else
	{
		// the last - 1 parameter is always the destination
		file_destination = argv[argc - 2];

		// the last parameter is always the filename
		filename = argv[argc -1];
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

	try {
		// Create a stream to the server using TCP.
		ibrcommon::tcpclient conn("127.0.0.1", 4550);

		try {
			// Initiate a client for synchronous receiving
			dtn::api::Client client(dtn::api::Client::MODE_SENDONLY, file_source, conn);

			// Connect to the server. Actually, this function initiate the
			// stream protocol by starting the thread and sending the contact header.
			client.connect();

			// target address
			EID addr = EID(file_destination);

			try {
				if (use_stdin)
				{
					cout << "Transfer stdin to " << addr.getNodeEID() << endl;

					// create an empty BLOB
					ibrcommon::BLOB::Reference ref = ibrcommon::TmpFileBLOB::create();

					// copy cin to a BLOB
					{
						ibrcommon::MutexLock l(ref);
						(*ref) << cin.rdbuf();
					}

					dtn::api::BLOBBundle b(file_destination, ref);

					// set the lifetime
					b.setLifetime(lifetime);

					// send the bundle
					client << b;
				}
				else
				{
					cout << "Transfer file \"" << filename << "\" to " << addr.getNodeEID() << endl;

					// create a bundle from the file
					dtn::api::FileBundle b(file_destination, filename);

					// set the lifetime
					b.setLifetime(lifetime);

					// send the bundle
					client << b;
				}

				// flush the buffers
				client.flush();
			} catch (ibrcommon::IOException ex) {
				std::cerr << "Error while sending bundle." << std::endl;
				std::cerr << "\t" << ex.what() << std::endl;
			}

			// Shutdown the client connection.
			client.close();
		} catch (ibrcommon::IOException ex) {
			cout << "Error: " << ex.what() << endl;
		}

		// close the tcpstream
		conn.close();
	} catch (...) {

	}

	return 0;
}

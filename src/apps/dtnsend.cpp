/*
 * dtnsend.cpp
 *
 *  Created on: 15.10.2009
 *      Author: morgenro
 */

#include "ibrdtn/api/Client.h"
#include "ibrdtn/api/FileBundle.h"
#include "ibrdtn/utils/tcpclient.h"
#include "ibrdtn/utils/Mutex.h"
#include "ibrdtn/utils/MutexLock.h"

#include <iostream>

class FileClient : public dtn::api::Client
{
	public:
		FileClient(string app, std::iostream &stream)
		 : dtn::api::Client(app, stream)
		{
		}

		virtual ~FileClient()
		{
		}

		void send(EID destination, string filename)
		{
			// create a bundle
			dtn::api::FileBundle b(destination, filename);

			// send the bundle
			(*this) << b;
		}

	protected:
		void received(dtn::api::Bundle &b)
		{
		};
};

void print_help()
{
	cout << "-- dtnping (IBR-DTN) --" << endl;
	cout << "Syntax: dtnsend [options] <dst> <filename>"  << endl;
	cout << " <dst>         set the destination eid (e.g. dtn://node/filetransfer)" << endl;
	cout << " <filename>    the file to transfer" << endl;
	cout << "* optional parameters *" << endl;
	cout << " -h|--help       display this text" << endl;
	cout << " --src <eid>     set the source application name (e.g. filetransfer)" << endl;

}

int main(int argc, char *argv[])
{
	string file_destination = "dtn://local/filetransfer";
	string file_source = "filetransfer";

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
	}

	// the last - 1 parameter is always the destination
	file_destination = argv[argc - 2];

	// the last parameter is always the filename
	string filename = argv[argc -1];

	// Create a stream to the server using TCP.
	dtn::utils::tcpclient conn("127.0.0.1", 4550);

	// Initiate a derivated client
	FileClient client(file_source, conn.stream());

	// Connect to the server. Actually, this function initiate the
	// stream protocol by starting the thread and sending the contact header.
	client.connect();

	// target address
	EID addr = EID(file_destination);

	cout << "Transfer file \"" << filename << "\" to " << addr.getNodeEID() << endl;

	// send the file
	client.send(file_destination, filename);

	// Shutdown the client connection.
	client.shutdown();

	conn.close();

	return 0;
}

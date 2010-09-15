/*
 * dtnrecv.cpp
 *
 *  Created on: 06.11.2009
 *      Author: morgenro
 */

#include "config.h"
#include "ibrdtn/api/Client.h"
#include "ibrdtn/api/FileBundle.h"
#include "ibrcommon/net/tcpclient.h"
#include "ibrcommon/thread/Mutex.h"
#include "ibrcommon/thread/MutexLock.h"

#include <csignal>
#include <sys/types.h>
#include <iostream>

void print_help()
{
	cout << "-- dtnrecv (IBR-DTN) --" << endl;
	cout << "Syntax: dtnrecv [options]"  << endl;
	cout << "* optional parameters *" << endl;
	cout << " -h|--help            display this text" << endl;
	cout << " --file <filename>    write the incoming data to the a file instead of the standard output" << endl;
	cout << " --name <name>        set the application name (e.g. filetransfer)" << endl;
	cout << " --timeout <seconds>  receive timeout in seconds" << endl;
	cout << " --count <number>     receive that many bundles" << endl;
}

void writeBundle(bool stdout, string filename, dtn::api::Bundle &b)
{
	ibrcommon::BLOB::Reference data = b.getData();

	// write the data to output
	if (stdout)
	{
		ibrcommon::MutexLock l(data);
		cout << (*data).rdbuf();
	}
	else
	{
		cout << " received." << endl;
		cout << "Writing bundle payload to " << filename << endl;

		fstream file(filename.c_str(), ios::in|ios::out|ios::binary|ios::trunc);

		ibrcommon::MutexLock l(data);
		file << (*data).rdbuf();

		file.close();

		cout << "finished" << endl;
	}
}

dtn::api::Client *_client = NULL;
ibrcommon::tcpclient *_conn = NULL;

void term(int signal)
{
	if (signal >= 1)
	{
		if (_client != NULL)
		{
			_client->close();
			_conn->close();
		}
		exit(0);
	}
}

int main(int argc, char *argv[])
{
	// catch process signals
	signal(SIGINT, term);
	signal(SIGTERM, term);

	int ret = EXIT_SUCCESS;
	string filename = "";
	string name = "filetransfer";
	bool stdout = true;
	int timeout = 0;
	int count   = 1;
	int h       = 0;

	for (int i = 0; i < argc; i++)
	{
		string arg = argv[i];

		// print help if requested
		if (arg == "-h" || arg == "--help")
		{
			print_help();
			return ret;
		}

		if (arg == "--name" && argc > i)
		{
			name = argv[i + 1];
		}

		if (arg == "--file" && argc > i)
		{
			filename = argv[i + 1];
			stdout = false;
		}

		if (arg == "--timeout" && argc > i)
		{
			timeout = atoi(argv[i + 1]);
		}

		if (arg == "--count" && argc > i) 
		{
			count = atoi(argv[i + 1]);
		}
	}

	try {
		// Create a stream to the server using TCP.
		ibrcommon::tcpclient conn("127.0.0.1", 4550);

		// Initiate a client for synchronous receiving
		dtn::api::Client client(name, conn);

		// export objects for the signal handler
		_conn = &conn;
		_client = &client;

		// Connect to the server. Actually, this function initiate the
		// stream protocol by starting the thread and sending the contact header.
		client.connect();

		if (!stdout) std::cout << "Wait for incoming bundle... " << std::flush;

		dtn::api::Bundle b;

		for(h=0; h<count; h++) {
			// receive the bundle
			b = client.getBundle(timeout);

			// write the bundle to stdout/file
			writeBundle(stdout, filename, b);
		}

		// Shutdown the client connection.
		client.close();

		// close the tcp connection
		conn.close();

		// write all following bundles
		while (!client.eof())
		{
			client >> b;

			// write the bundle to stdout/file
			writeBundle(stdout, filename, b);
		}
	} catch (...) {
		ret = EXIT_FAILURE;
		cout << "EXCEPTION" << endl;
	}

	return ret;
}

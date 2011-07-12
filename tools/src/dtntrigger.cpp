/*
 * dtntrigger.cpp
 *
 *  Created on: 02.07.2010
 *      Author: morgenro
 */

#include "config.h"
#include <ibrdtn/api/Client.h>
#include <ibrcommon/net/tcpclient.h>

#include <ibrcommon/data/File.h>

/**
#include "ibrcommon/thread/Mutex.h"
#include "ibrcommon/thread/MutexLock.h"
#include "ibrdtn/data/PayloadBlock.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrcommon/data/BLOB.h"

#include "ibrcommon/appstreambuf.h"

#include <stdlib.h>
#include <iostream>
#include <map>
#include <vector>

#include <sys/types.h>
*/

#include <csignal>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


// set this variable to false to stop the app
bool _running = true;

// global connection
ibrcommon::tcpclient *_conn = NULL;

std::string _appname = "trigger";
std::string _script = "";
std::string _shell = "/bin/sh";

ibrcommon::File blob_path("/tmp");

void print_help()
{
	cout << "-- dtntrigger (IBR-DTN) --" << endl;
	cout << "Syntax: dtntrigger [options] <name> <shell> [trigger-script]"  << endl;
	cout << "<name>            the application name" << endl;
	cout << "<shell>           shell to execute the trigger script" << endl;
	cout << "[trigger-script]  optional: the trigger script to execute on incoming bundle" << endl;
	cout << "* optional parameters *" << endl;
	cout << " -h|--help        display this text" << endl;
	cout << " -w|--workdir     temporary work directory" << endl;
}

int init(int argc, char** argv)
{
	int index;
	int c;

	opterr = 0;

	while ((c = getopt (argc, argv, "w:")) != -1)
	switch (c)
	{
		case 'w':
			blob_path = ibrcommon::File(optarg);
			break;

		case '?':
			if (optopt == 'w')
			fprintf (stderr, "Option -%c requires an argument.\n", optopt);
			else if (isprint (optopt))
			fprintf (stderr, "Unknown option `-%c'.\n", optopt);
			else
			fprintf (stderr,
					 "Unknown option character `\\x%x'.\n",
					 optopt);
			return 1;

		default:
			print_help();
			abort();
	}

	int optindex = 0;
	for (index = optind; index < argc; index++)
	{
		switch (optindex)
		{
		case 0:
			_appname = std::string(argv[index]);
			break;

		case 1:
			_shell = std::string(argv[index]);
			break;

		case 2:
			_script = std::string(argv[index]);
			break;
		}

		optindex++;
	}

	// print help if not enough parameters are set
	if (optindex < 2) { print_help(); exit(0); }

	// enable file based BLOBs if a correct path is set
	if (blob_path.exists())
	{
		ibrcommon::BLOB::changeProvider(new ibrcommon::FileBLOBProvider(blob_path));
	}

	return 0;
}

void term(int signal)
{
	if (signal >= 1)
	{
		_running = false;
		if (_conn != NULL) _conn->close();
		exit(0);
	}
}

/*
 * main application method
 */
int main(int argc, char** argv)
{
	// catch process signals
	signal(SIGINT, term);
	signal(SIGTERM, term);

	// read the configuration
	if (init(argc, argv) > 0)
	{
		return (EXIT_FAILURE);
	}

	// backoff for reconnect
	size_t backoff = 2;

	// loop, if no stop if requested
	while (_running)
	{
		try {
			// Create a stream to the server using TCP.
			ibrcommon::tcpclient conn("127.0.0.1", 4550);

			// enable nodelay option
			conn.enableNoDelay();

			// set the connection globally
			_conn = &conn;

			// Initiate a client for synchronous receiving
			dtn::api::Client client(_appname, conn);

			// Connect to the server. Actually, this function initiate the
			// stream protocol by starting the thread and sending the contact header.
			client.connect();

			// reset backoff if connected
			backoff = 2;

			// check the connection
			while (_running)
			{
				// receive the bundle
				dtn::api::Bundle b = client.getBundle();

				// get the reference to the blob
				ibrcommon::BLOB::Reference ref = b.getData();

				// get a temporary file name
				ibrcommon::TemporaryFile file(blob_path, "bundle");

				// write data to temporary file
				try {
					std::fstream out(file.getPath().c_str(), ios::out|ios::binary|ios::trunc);
					out.exceptions(std::ios::badbit | std::ios::eofbit);
					out << ref.iostream()->rdbuf();
					out.close();

					// call the script
					std::string cmd = _shell + " " + _script + " " + b.getSource().getString() + " " + file.getPath();
					::system(cmd.c_str());

					// remove temporary file
					file.remove();
				} catch (const ios_base::failure&) {

				}
			}

			// close the client connection
			client.close();

			// close the connection
			conn.close();

			// set the global connection to NULL
			_conn = NULL;
		} catch (const ibrcommon::tcpclient::SocketException&) {
			// set the global connection to NULL
			_conn = NULL;

			if (_running)
			{
				cout << "Connection to bundle daemon failed. Retry in " << backoff << " seconds." << endl;
				sleep(backoff);

				// if backoff < 10 minutes
				if (backoff < 600)
				{
					// set a new backoff
					backoff = backoff * 2;
				}
			}
		} catch (const ibrcommon::IOException &ex) {
			// set the global connection to NULL
			_conn = NULL;

			if (_running)
			{
				cout << "Connection to bundle daemon failed. Retry in " << backoff << " seconds." << endl;
				sleep(backoff);

				// if backoff < 10 minutes
				if (backoff < 600)
				{
					// set a new backoff
					backoff = backoff * 2;
				}
			}
		} catch (const std::exception&) {
			// set the global connection to NULL
			_conn = NULL;
		}
	}

	return (EXIT_SUCCESS);
}

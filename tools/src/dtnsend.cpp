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
#include <ibrcommon/Logger.h>

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
	cout << " -p <0..2>       set the bundle priority (0 = low, 1 = normal, 2 = high)" << endl;
	cout << " --lifetime <seconds> set the lifetime of outgoing bundles; default: 3600" << endl;
	cout << " -U <socket>     use UNIX domain sockets" << endl;

}

int main(int argc, char *argv[])
{
	bool error = false;
	string file_destination = "dtn://local/filetransfer";
	string file_source = "filetransfer";
	unsigned int lifetime = 3600;
	bool use_stdin = false;
	std::string filename;
	ibrcommon::File unixdomain;
	int priority = 1;

//	ibrcommon::Logger::setVerbosity(99);
//	ibrcommon::Logger::addStream(std::cout, ibrcommon::Logger::LOGGER_ALL, ibrcommon::Logger::LOG_DATETIME | ibrcommon::Logger::LOG_LEVEL);

	std::list<std::string> arglist;

	for (int i = 0; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			std::string arg = argv[i];

			// print help if requested
			if (arg == "-h" || arg == "--help")
			{
				print_help();
				return 0;
			}

			if (arg == "--src" && argc > i)
			{
				if (++i > argc)
				{
					std::cout << "argument missing!" << std::endl;
					return -1;
				}

				file_source = argv[i];
			}

			if (arg == "--lifetime" && argc > i)
			{
				if (++i > argc)
				{
					std::cout << "argument missing!" << std::endl;
					return -1;
				}

				stringstream data; data << argv[i];
					data >> lifetime;
			}

			if (arg == "-p" && argc > i)
			{
				if (++i > argc)
				{
					std::cout << "argument missing!" << std::endl;
					return -1;
				}
				stringstream data; data << argv[i];
					data >> priority;
			}

			if (arg == "-U" && argc > i)
			{
				if (++i > argc)
				{
					std::cout << "argument missing!" << std::endl;
					return -1;
				}

				unixdomain = ibrcommon::File(argv[i]);
			}
		}
		else
		{
			arglist.push_back(argv[i]);
		}
	}

	if (arglist.size() <= 1)
	{
		print_help();
		return -1;
	} else if (arglist.size() == 2)
	{
		std::list<std::string>::iterator iter = arglist.begin(); iter++;

		// the first parameter is the destination
		file_destination = (*iter);

		use_stdin = true;
	}
	else if (arglist.size() > 2)
	{
		std::list<std::string>::iterator iter = arglist.begin(); iter++;

		// the first parameter is the destination
		file_destination = (*iter); iter++;

		// the second parameter is the filename
		filename = (*iter);
	}

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

					// set the bundles priority
					b.setPriority(dtn::api::Bundle::BUNDLE_PRIORITY(priority));

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

					// set the bundles priority
					b.setPriority(dtn::api::Bundle::BUNDLE_PRIORITY(priority));

					// send the bundle
					client << b;
				}

				// flush the buffers
				client.flush();
			} catch (const ibrcommon::IOException &ex) {
				std::cerr << "Error while sending bundle." << std::endl;
				std::cerr << "\t" << ex.what() << std::endl;
				error = true;
			}

			// Shutdown the client connection.
			client.close();

			// print out the transmitted bytes
			std::cout << client.lastack << " bytes sent" << std::endl;

		} catch (const ibrcommon::IOException &ex) {
			cout << "Error: " << ex.what() << endl;
			error = true;
		}

		// close the tcpstream
		conn.close();
	} catch (const ibrcommon::ConnectionClosedException&) {
		// connection already closed, the daemon was faster
	} catch (const std::exception &ex) {
		cout << "Error: " << ex.what() << endl;
		error = true;
	}

	if (error) return -1;

	return 0;
}

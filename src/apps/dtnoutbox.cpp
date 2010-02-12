/*
 * dtnoutbox.cpp
 *
 *  Created on: 20.11.2009
 *      Author: morgenro
 */

#include "ibrdtn/default.h"
#include "ibrdtn/api/Client.h"
#include "ibrdtn/api/FileBundle.h"
#include "ibrcommon/net/tcpclient.h"
#include "ibrcommon/thread/Mutex.h"
#include "ibrcommon/thread/MutexLock.h"
#include "ibrdtn/data/PayloadBlock.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrcommon/data/BLOB.h"
#include "ibrcommon/data/File.h"
#include "ibrcommon/appstreambuf.h"

#include <stdlib.h>
#include <iostream>
#include <map>
#include <vector>

using namespace ibrcommon;

void print_help()
{
        cout << "-- dtnoutbox (IBR-DTN) --" << endl;
        cout << "Syntax: dtnoutbox [options] <name> <outbox> <destination>"  << endl;
        cout << " <name>           the application name" << endl;
        cout << " <outbox>         directory with outgoing files" << endl;
        cout << " <destination>    the destination EID for all outgoing files" << endl;
        cout << "* optional parameters *" << endl;
        cout << " -h|--help        display this text" << endl;
        cout << " -w|--workdir     temporary work directory" << endl;
}

map<string,string> readconfiguration(int argc, char** argv)
{
    // print help if not enough parameters are set
    if (argc < 4) { print_help(); exit(0); }

    map<string,string> ret;

    ret["name"] = argv[argc - 3];
    ret["outbox"] = argv[argc - 2];
    ret["destination"] = argv[argc - 1];

    for (int i = 0; i < (argc - 3); i++)
    {
        string arg = argv[i];

        // print help if requested
        if (arg == "-h" || arg == "--help")
        {
            print_help();
            exit(0);
        }

        if ((arg == "-w" || arg == "--workdir") && (argc > i))
        {
            ret["workdir"] = argv[i + 1];
        }
    }

    return ret;
}

// set this variable to false to stop the app
bool _running = true;

// global connection
ibrcommon::tcpclient *_conn = NULL;

void term(int signal)
{
    if (signal >= 1)
    {
        _running = false;
        if (_conn != NULL) _conn->close();
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
    map<string,string> conf = readconfiguration(argc, argv);

    // init working directory
    if (conf.find("workdir") != conf.end())
    {
    	ibrcommon::BLOB::tmppath = File(conf["workdir"]);
    }

    // backoff for reconnect
    size_t backoff = 2;

    // check outbox for files
	File outbox(conf["outbox"]);

    // loop, if no stop if requested
    while (_running)
    {
        try {
        	// Create a stream to the server using TCP.
        	ibrcommon::tcpclient conn("127.0.0.1", 4550);

        	// set the connection globally
        	_conn = &conn;

            // Initiate a client for synchronous receiving
            dtn::api::Client client(dtn::api::Client::MODE_SENDONLY, conf["name"], conn, false);

            // Connect to the server. Actually, this function initiate the
            // stream protocol by starting the thread and sending the contact header.
            client.connect();

            // reset backoff if connected
            if (client.isConnected()) backoff = 2;

            // check the connection
            while (client.isConnected() && _running)
            {
            	list<File> files;
            	outbox.getFiles(files);

            	// <= 2 because of "." and ".."
            	if (files.size() <= 2)
            	{
                    // wait some seconds
                    sleep(10);

                    continue;
            	}

            	stringstream file_list;

            	int prefix_length = outbox.getPath().length() + 1;

            	for (list<File>::iterator iter = files.begin(); iter != files.end(); iter++)
            	{
            		File &f = (*iter);

            		// skip system files ("." and "..")
            		if (f.isSystem()) continue;

					// remove the prefix of the outbox path
            		string rpath = f.getPath();
            		rpath = rpath.substr(prefix_length, rpath.length() - prefix_length);

            		file_list << rpath << " ";
            	}

            	// output of all files to send
            	cout << "files: " << file_list.str() << endl;

            	// "--remove-files" deletes files after adding
            	stringstream cmd; cmd << "tar --remove-files -cO -C " << outbox.getPath() << " " << file_list.str();

            	// make a tar archive
            	appstreambuf app(cmd.str(), appstreambuf::MODE_READ);
            	istream stream(&app);

    			// create a bundle
    			dtn::data::Bundle b;

            	// create a payloadblock
    			dtn::data::PayloadBlock *payload = new dtn::data::PayloadBlock(ibrcommon::TmpFileBLOB::create());

    			// stream the content of "tar" to the payload block
    			{
    				ibrcommon::BLOB::Reference data = payload->getBLOB();
    				ibrcommon::MutexLock l(data);
    				data.write(stream);
    			}

    			// add the payload block to the bundle
    			b.addBlock(payload);

    			// set the destination
    			b._destination = EID(conf["destination"]);

                // send the bundle
    			client << b; client.flush();

            	if (_running)
            	{
					// wait some seconds
					sleep(10);
            	}
            }

            // close the client connection
            client.shutdown();

            // close the connection
            conn.close();

            // set the global connection to NULL
            _conn = NULL;
        } catch (ibrcommon::tcpclient::SocketException ex) {
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
        } catch (dtn::exceptions::IOException ex) {
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
        }
    }

    return (EXIT_SUCCESS);
}

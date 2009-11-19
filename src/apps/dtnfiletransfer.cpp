/*
 * File:   dtnfiletransfer.cpp
 * Author: morgenro
 *
 * Created on 19. November 2009, 08:21
 */

#include <stdlib.h>

#include "ibrdtn/default.h"
#include "ibrdtn/api/Client.h"
#include "ibrdtn/api/FileBundle.h"
#include "ibrdtn/utils/tcpclient.h"
#include "ibrdtn/utils/Mutex.h"
#include "ibrdtn/utils/MutexLock.h"
#include "ibrdtn/data/PayloadBlock.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/data/BLOBManager.h"

#include <iostream>
#include <map>
#include <vector>
#include <dirent.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

class appstream : public std::streambuf
{
public:
    enum { BUF_SIZE = 512 };

    enum Mode
    {
    	MODE_READ = 0,
    	MODE_WRITE = 1
    };

    appstream(string command, appstream::Mode mode)
     : m_buf( BUF_SIZE+1 ), m_inbuf( BUF_SIZE+1 )
    {
        setp( &m_buf[0], &m_buf[0] + (m_buf.size() - 1) );

        // execute the command
        if (mode == MODE_READ)
        	_handle = popen(command.c_str(), "r");
        else
        	_handle = popen(command.c_str(), "w");
    }

    virtual ~appstream()
    {
        // close the handle
        pclose(_handle);
    }


protected:
	virtual int underflow()
	{
		// read the stdout of the process
		size_t ret = fread(&m_buf[0], m_buf.size(), 1, _handle);

		// Since the input buffer content is now valid (or is new)
		// the get pointer should be initialized (or reset).
		setg(&m_buf[0], &m_buf[0], &m_buf[0] + ret);

		return std::char_traits<char>::not_eof(m_buf[0]);
	}

    virtual int sync()
    {
		int ret = std::char_traits<char>::eq_int_type(this->overflow(std::char_traits<char>::eof()),
				std::char_traits<char>::eof()) ? -1 : 0;

		return ret;
    }

    virtual int_type overflow( int_type m = traits_type::eof() )
    {
		char *ibegin = pbase();
		char *iend = pptr();

		// if there is nothing to send, just return
        if ( iend <= ibegin ) return std::char_traits<char>::not_eof(m);

        // mark the buffer as free
        setp( &m_buf[0], &m_buf[0] + (m_buf.size() - 1) );

		if(!std::char_traits<char>::eq_int_type(m, std::char_traits<char>::eof())) {
			*iend++ = std::char_traits<char>::to_char_type(m);
		}

        // write the data
        fwrite(pbase(), (iend - ibegin), 1, _handle);

        return std::char_traits<char>::not_eof(m);
    }

private:
	void writeToProcess()
	{

	}

    std::vector< char_type > m_buf;
    std::vector< char_type > m_inbuf;
    FILE *_handle;
};

class filereceiver : public dtn::api::Client
{
    public:
        filereceiver(string app, string inbox, string address = "127.0.0.1", int port = 4550)
        : _tcpclient(address, port), dtn::api::Client(app, _tcpclient), _inbox(inbox)
        { };

        /**
         * Destructor of the connection.
         */
        virtual ~filereceiver()
        {
                dtn::api::Client::shutdown();

                // Close the tcp connection.
                _tcpclient.close();
        };

    private:
        // file descriptor for the tun device
        string _inbox;

        /**
         * In this API bundles are received asynchronous. To receive bundles it is necessary
         * to overload the Client::received()-method. This will be call on a incoming bundles
         * by another thread.
         */
        void received(dtn::api::Bundle &b)
        {
            dtn::blob::BLOBReference ref = b.getData();

            stringstream cmdstream; cmdstream << "tar -x -C " << _inbox;

            // create a tar handler
            appstream extractor(cmdstream.str(), appstream::MODE_WRITE);
            ostream stream(&extractor);

            // write the payload to the extractor
            ref.read( stream );

            // flush the stream
            stream.flush();
        }

        dtn::utils::tcpclient _tcpclient;
};

void print_help()
{
        cout << "-- dtnfiletransfer (IBR-DTN) --" << endl;
        cout << "Syntax: dtnfiletransfer [options] <name> <inbox> <outbox> <destination>"  << endl;
        cout << " <name>           the application name" << endl;
        cout << " <inbox>          directory where incoming files should be placed" << endl;
        cout << " <outbox>         directory with outgoing files" << endl;
        cout << " <destination>    the destination EID for all outgoing files" << endl;
        cout << "* optional parameters *" << endl;
        cout << " -h|--help        display this text" << endl;
        cout << " -w|--workdir     temporary work directory" << endl;
}

map<string,string> readconfiguration(int argc, char** argv)
{
    // print help if not enough parameters are set
    if (argc < 5) { print_help(); exit(0); }

    map<string,string> ret;

    ret["name"] = argv[argc - 4];
    ret["inbox"] = argv[argc - 3];
    ret["outbox"] = argv[argc - 2];
    ret["destination"] = argv[argc - 1];

    for (int i = 0; i < (argc - 4); i++)
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

class File
{
public:
	File(const string path)
	 : _path(path), _type(DT_UNKNOWN)
	{
		struct stat s;
		int type;

		stat(path.c_str(), &s);

		type = s.st_mode & S_IFMT;

		switch (type)
		{
			case S_IFREG:
				_type = DT_REG;
				break;

			case S_IFLNK:
				_type = DT_LNK;
				break;

			case S_IFDIR:
				_type = DT_DIR;
				break;

			default:
				_type = DT_UNKNOWN;
				break;
		}
	}

	virtual ~File()
	{}

	unsigned char getType() const
	{
		return _type;
	}

	int getFiles(list<File> &files)
	{
		if (!isDirectory()) return -1;

	    DIR *dp;
	    struct dirent *dirp;
	    if((dp = opendir(_path.c_str())) == NULL) {
	        return errno;
	    }

	    while ((dirp = readdir(dp)) != NULL)
	    {
	    	string name = string(dirp->d_name);
	    	stringstream ss; ss << getPath() << "/" << name;
	    	File file(ss.str(), dirp->d_type);
	    	files.push_back(file);
	    }
	    closedir(dp);
	}

	bool isHidden()
	{
		//if (_name.substr(0,1) == ".") return true;
		return false;
	}

	bool isSystem()
	{
		if ((_path.substr(_path.length() - 2, 2) == "..") || (_path.substr(_path.length() - 1, 1) == ".")) return true;
		return false;
	}

	bool isDirectory()
	{
		if (_type == DT_DIR) return true;
		return false;
	}

	string getPath() const
	{
		return _path;
	}

	int remove(bool recursive = false)
	{
		int ret;

		if (isSystem()) return -1;

		if (isDirectory())
		{
			if (recursive)
			{
				// container for all files
				list<File> files;

				// get all files in this directory
				if ((ret = getFiles(files)) < 0)
					return ret;

				for (list<File>::iterator iter = files.begin(); iter != files.end(); iter++)
				{
					if (!(*iter).isSystem())
					{
						if ((ret = (*iter).remove(recursive)) < 0)
							return ret;
					}
				}
			}

			//rmdir(getPath());
			cout << "remove directory: " << getPath() << endl;
		}
		else
		{
			//remove(getPath());
			cout << "remove file: " << getPath() << endl;
		}

		return 0;
	}

private:
	File(const string path, const unsigned char t)
	 : _path(path), _type(t)
	{

	}

	const string _path;
	unsigned char _type;
};

// set this variable to false to stop the app
bool _running = true;

void term(int signal)
{
    if (signal >= 1)
    {
        _running = false;
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
        dtn::blob::BLOBManager::init(conf["workdir"]);
    }

    // backoff for reconnect
    size_t backoff = 2;

    // check outbox for files
	File outbox(conf["outbox"]);

    // loop, if no stop if requested
    while (_running)
    {
        try {
            // Initiate a client for synchronous receiving
            filereceiver client(conf["name"], conf["inbox"]);

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
            	stringstream cmd; cmd << "tar -cO -C " << outbox.getPath() << " " << file_list.str();

            	// make a tar archive
            	appstream app(cmd.str(), appstream::MODE_READ);
            	iostream stream(&app);

    			// create a bundle
    			dtn::data::Bundle b;

            	// create a payloadblock
    			dtn::data::PayloadBlock *payload = new dtn::data::PayloadBlock(dtn::blob::BLOBManager::BLOB_HARDDISK);

    			// stream the content of "tar" to the payload block
    			payload->getBLOBReference().write(0, stream);

    			// add the payload block to the bundle
    			b.addBlock(payload);

    			// set the destination
    			b._destination = EID(conf["destination"]);

                // send the bundle
    			client << b; client.flush();

                // wait some seconds
                sleep(10);
            }

            // close the client connection
            client.shutdown();
        } catch (dtn::utils::tcpclient::SocketException ex) {
            cout << "Connection to bundle daemon failed. Retry in " << backoff << " seconds." << endl;
            sleep(backoff);

            // if backoff < 10 minutes
            if (backoff < 600)
            {
            	// set a new backoff
            	backoff = backoff * 2;
            }
        } catch (dtn::exceptions::IOException ex) {
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

    return (EXIT_SUCCESS);
}

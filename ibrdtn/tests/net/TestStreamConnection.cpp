/*
 * TestStreamConnection.cpp
 *
 *  Created on: 04.11.2010
 *      Author: morgenro
 */

#include "net/TestStreamConnection.h"

#include <ibrcommon/net/tcpserver.h>
#include <ibrcommon/net/tcpclient.h>
#include <ibrdtn/streams/StreamConnection.h>
#include <ibrcommon/thread/Mutex.h>
#include <ibrcommon/thread/MutexLock.h>
#include <ibrcommon/thread/Thread.h>
#include <ibrdtn/data/Serializer.h>
#include <ibrcommon/data/BLOB.h>

CPPUNIT_TEST_SUITE_REGISTRATION (TestStreamConnection);

void TestStreamConnection::setUp()
{
}

void TestStreamConnection::tearDown()
{
}

void TestStreamConnection::connectionUpDown()
{
	class testserver : public ibrcommon::tcpserver, public ibrcommon::JoinableThread, dtn::streams::StreamConnection::Callback
	{
	public:
		testserver(ibrcommon::File &file) : ibrcommon::tcpserver(file), recv_bundles(0) {};
		testserver(ibrcommon::vinterface net, int port) : ibrcommon::tcpserver(net, port), recv_bundles(0) {};
		virtual ~testserver() { join(); };

		void eventShutdown(dtn::streams::StreamConnection::ConnectionShutdownCases csc) {};
		void eventTimeout() {};
		void eventError() {};
		void eventBundleRefused() {};
		void eventBundleForwarded() {};
		void eventBundleAck(size_t ack)
		{
//			std::cout << "server: ack received, value: " << ack << std::endl;
		};
		void eventConnectionUp(const dtn::streams::StreamContactHeader &header) {};
		void eventConnectionDown() {};

		unsigned int recv_bundles;

	protected:
		void run()
		{
			ibrcommon::tcpstream *conn = accept();
			dtn::streams::StreamConnection stream(*this, *conn);

			// do the handshake
			stream.handshake(dtn::data::EID("dtn:server"), 0, dtn::streams::StreamContactHeader::REQUEST_ACKNOWLEDGMENTS);

			while (conn->good())
			{
				dtn::data::Bundle b;
				dtn::data::DefaultDeserializer(stream) >> b;
//				std::cout << "server: bundle received" << std::endl;
				recv_bundles++;
			}
		}
	};

	class testclient : public ibrcommon::JoinableThread, dtn::streams::StreamConnection::Callback
	{
	private:
		ibrcommon::tcpclient &_client;
		dtn::streams::StreamConnection _stream;

	public:
		testclient(ibrcommon::tcpclient &client) : _client(client), _stream(*this, _client)
		{ }
		virtual ~testclient() { join(); };

		void eventShutdown(dtn::streams::StreamConnection::ConnectionShutdownCases csc) {};
		void eventTimeout() {};
		void eventError() {};
		void eventBundleRefused() {};
		void eventBundleForwarded() {};
		void eventBundleAck(size_t ack)
		{
//			std::cout << "client: ack received, value: " << ack << std::endl;
		};

		void eventConnectionUp(const dtn::streams::StreamContactHeader &header) {};
		void eventConnectionDown() {};

		void handshake()
		{
			// do the handshake
			_stream.handshake(dtn::data::EID("dtn:client"), 0, dtn::streams::StreamContactHeader::REQUEST_ACKNOWLEDGMENTS);
		}

		void send(int size = 2048)
		{
			dtn::data::Bundle b;
			ibrcommon::BLOB::Reference ref = ibrcommon::StringBLOB::create();

			{
				ibrcommon::MutexLock l(ref);
				(*ref) << "Hallo Welt" << std::endl;

				// create testing pattern, chunkwise to ocnserve memory
				char pattern[2048];
				for (size_t i = 0; i < sizeof(pattern); i++)
				{
					pattern[i] = '0';
					pattern[i] += i % 10;
				}
				string chunk=string(pattern,2048);

				while (size > 2048) {
					(*ref) << chunk;
					size-=2048;
				}
				(*ref) << chunk.substr(0,size);
			}

			dtn::data::PayloadBlock &payload = b.push_back(ref);
			dtn::data::DefaultSerializer(_stream) << b; _stream << std::flush;
		}

		void close()
		{
			_stream.shutdown();
			stop();
		}

	protected:
		void run()
		{
			while (_client.good())
			{
				dtn::data::Bundle b;
				dtn::data::DefaultDeserializer(_stream) >> b;
//				std::cout << "client: bundle received" << std::endl;
			}
		}
	};

	ibrcommon::vinterface net("lo");
	ibrcommon::File socket("/tmp/testsuite.sock");
	testserver srv(net, 1234); srv.start();

	ibrcommon::tcpclient conn("127.0.0.1", 1234);
	testclient cl(conn);
	cl.handshake();
	cl.start();

	for (int i = 0; i < 2000; i++)
	{
		cl.send(8192);
	}

	cl.close();
	conn.close();
	srv.close();

	CPPUNIT_ASSERT_EQUAL((unsigned int) 2000, srv.recv_bundles);
}


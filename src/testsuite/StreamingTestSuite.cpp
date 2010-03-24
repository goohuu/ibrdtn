/*
 * StreamingTestSuite.cpp
 *
 *  Created on: 16.10.2009
 *      Author: morgenro
 */

#include "testsuite/StreamingTestSuite.h"
#include <memory>

namespace dtn
{
	namespace testsuite
	{
		StreamingTestSuite::StreamConnChecker::StreamConnChecker(ibrcommon::NetInterface &net, int chars)
		 : _srv(net), failed(false), _chars(chars)
		{
		}

		StreamingTestSuite::StreamConnChecker::~StreamConnChecker()
		{
			join();
		}

		void StreamingTestSuite::StreamConnChecker::run()
		{
			char values[10] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
			size_t byte = 0;

			ibrcommon::tcpstream *tcpstream = _srv.accept();
			dtn::streams::StreamConnection *stream = new dtn::streams::StreamConnection(*this, *tcpstream);

			stream->handshake(EID("dtn:local/node2"));

			while (stream->good())
			{
				char value;

				// read one char
				stream->get(value);

				if ((value != values[byte % _chars]) && stream->good())
				{
					cout << "error in byte " << byte << ", " << value << " != " << values[byte % _chars] << endl;
					failed = true;
					break;
				}

				byte++;
			}

			cout << (byte-1) << " bytes received." << endl;

			stream->wait();
			stream->close();

			delete stream;

			tcpstream->close();
			delete tcpstream;
		}

		void StreamingTestSuite::StreamConnChecker::eventShutdown()
		{
			std::cout << "StreamingTestSuite::StreamConnChecker::eventShutdown" << std::endl;
		}

		void StreamingTestSuite::StreamConnChecker::eventTimeout()
		{

		}

		void StreamingTestSuite::StreamConnChecker::eventConnectionUp(const StreamContactHeader &header)
		{

		}

		void StreamingTestSuite::eventShutdown()
		{
			std::cout << "StreamingTestSuite::eventShutdown" << std::endl;
		}

		void StreamingTestSuite::eventTimeout()
		{

		}

		void StreamingTestSuite::eventConnectionUp(const StreamContactHeader &header)
		{

		}

		StreamingTestSuite::StreamingTestSuite()
		{

		}

		StreamingTestSuite::~StreamingTestSuite()
		{

		}

		bool StreamingTestSuite::runAllTests()
		{
			bool ret = true;
			cout << "StreamingTestSuite... ";

			for (int k = 2; k <= 10; k++)
			{
				cout << "streamconnectionTest with " << k << " chars: ";
				if ( !streamconnectionTest(k) )
				{
					cout << " failed" << endl;
					ret = false;
				}
			}

			if (ret) cout << " passed" << endl;

			if (ret) cout << "\t\t\tpassed" << endl;

			return ret;
		}

		StreamingTestSuite::Receiver::Receiver(dtn::streams::StreamConnection &conn)
		 : _conn(conn) {}

		StreamingTestSuite::Receiver::~Receiver()
		{
			_conn.close();
			join();
		}

		void StreamingTestSuite::Receiver::run()
		{
			while (!_conn.eof())
			{
				_conn.get();
				yield();
			}
		}

		bool StreamingTestSuite::streamconnectionTest(int chars)
		{
			ibrcommon::NetInterface net(ibrcommon::NetInterface::NETWORK_TCP, "lo", 4556);

			char values[10] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };


			StreamConnChecker *checker = new StreamConnChecker(net, chars);
			checker->start();

			ibrcommon::tcpclient client("127.0.0.1", 4556);
			dtn::streams::StreamConnection stream(*this, client);
			stream.handshake(EID("dtn:local/node1"));

			Receiver receiver(stream);
			receiver.start();

			// send some data
			for (size_t j = 0; j < 10; j++)
			{
				for (size_t i = 0; i < 100000; i++)
				{
					for (int k = 0; k < chars; k++)
					{
						stream.put(values[k]);
					}
				}
			}

			stream.flush();

			stream.wait();
			stream.close();

			receiver.waitFor();

			client.flush();
			client.close();

			checker->waitFor();

			if (checker->failed) return false;

			delete checker;

			return true;
		}
	}
}

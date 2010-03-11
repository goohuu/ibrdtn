/*
 * StreamingTestSuite.cpp
 *
 *  Created on: 16.10.2009
 *      Author: morgenro
 */

#include "testsuite/StreamingTestSuite.h"

namespace dtn
{
	namespace testsuite
	{
		StreamingTestSuite::StreamChecker::StreamChecker(ibrcommon::NetInterface &net, int chars)
		 : _srv(net), failed(false), _chars(chars)
		{
		}

		StreamingTestSuite::StreamChecker::~StreamChecker()
		{
			join();
		}

		void StreamingTestSuite::StreamChecker::run()
		{
			char values[10] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
			size_t byte = 0;

			ibrcommon::tcpstream stream(_srv.accept());

			while (stream.good())
			{
				char value;

				// read one char
				stream.get(value);

				if ((value != values[byte % _chars]) && stream.good())
				{
					cout << "error in byte " << byte << ", " << value << " != " << values[byte % _chars] << endl;
					failed = true;
					break;
				}

				byte++;
			}

			stream.close();
		}

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

			ibrcommon::tcpstream tcpstream(_srv.accept());
			dtn::streams::StreamConnection stream(tcpstream);
			stream.start();

			while (stream.good())
			{
				char value;

				// read one char
				stream.get(value);

				if ((value != values[byte % _chars]) && stream.good())
				{
					cout << "error in byte " << byte << ", " << value << " != " << values[byte % _chars] << endl;
					failed = true;
					break;
				}

				byte++;
			}

			cout << byte << " bytes received." << endl;

			tcpstream.close();
			stream.shutdown();
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

//			for (int k = 2; k <= 10; k++)
//			{
//				cout << endl << "tcpstreamTest with " << k << " chars: ";
//				if ( !tcpstreamTest(k) )
//				{
//					cout << " failed" << endl;
//					ret = false;
//				}
//			}

			for (int k = 2; k <= 10; k++)
			{
				cout << endl << "streamconnectionTest with " << k << " chars: ";
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

		bool StreamingTestSuite::streamconnectionTest(int chars)
		{
			ibrcommon::NetInterface net(ibrcommon::NetInterface::NETWORK_TCP, "lo", 4556);

			char values[10] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };


			StreamConnChecker receiver(net, chars);
			receiver.start();

			ibrcommon::tcpclient client("127.0.0.1", 4556);
			dtn::streams::StreamConnection stream(client);
			stream.start();

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

				stream.flush();
			}

			client.close();
			stream.shutdown();
			receiver.waitFor();
			if (receiver.failed) return false;

			return true;
		}

		bool StreamingTestSuite::tcpstreamTest(int chars)
		{
			ibrcommon::NetInterface net(ibrcommon::NetInterface::NETWORK_TCP, "lo", 1234);

			char values[10] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };

			StreamChecker receiver(net, chars);
			receiver.start();

			ibrcommon::tcpclient client("127.0.0.1", 1234);

			// send some data
			for (size_t j = 0; j < 10; j++)
			{
				for (size_t i = 0; i < 100000; i++)
				{
					for (int k = 0; k < chars; k++)
					{
						client.put(values[k]);
					}
				}

				client.flush();
			}

			client.close();
			receiver.waitFor();

			if (receiver.failed) return false;

			return true;
		}
	}
}

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
		StreamingTestSuite::StreamingPeer::StreamingPeer(std::iostream &stream)
		 : _buf(stream), std::iostream(&_buf), _shutdown(false)
		{
		};

		void StreamingTestSuite::StreamingPeer::run()
		{
			while (!_shutdown)
			{
				char data[256];
				read(data, 256);

				cout << "read 256 bytes" << endl;
			}
   		}

		void StreamingTestSuite::StreamingPeer::processHeader(dtn::streams::StreamContactHeader &h)
		{

		}

		void StreamingTestSuite::StreamingPeer::processSegment(dtn::streams::StreamDataSegment &seg)
		{

		}

		void StreamingTestSuite::StreamingPeer::datareceived(size_t newdata, size_t complete)
		{
			cout << newdata << " / " << complete << endl;
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

			if ( !commonTest() )
			{
				cout << endl << "commonTest failed" << endl;
				ret = false;
			}

			if (ret) cout << "\t\t\tpassed" << endl;

			return ret;
		}

		bool StreamingTestSuite::commonTest()
		{
			stringstream ss;

//			StreamingPeer sender(ss);
//			StreamingPeer receiver(ss);
//
//			// run the receiver
//			receiver.start();
//
//			ss.read(0, 0);
//
//			char data[256];
//
//			for (int i = 0; i < 256; i++)
//			{
//				data[i] = i;
//			}
//
//			// send some data
//			sender.write(data, 256);
//			sender.write(data, 256);
//			sender.write(data, 256);
//			sender.write(data, 256);
//			sender.write(data, 256);
//
//			receiver._shutdown = true;

			return true;
		}
	}
}

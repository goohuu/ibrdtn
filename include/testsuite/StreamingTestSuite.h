/*
 * StreamingTestSuite.h
 *
 *  Created on: 16.10.2009
 *      Author: morgenro
 */

#ifndef STREAMINGTESTSUITE_H_
#define STREAMINGTESTSUITE_H_

#include "ibrdtn/default.h"
#include "ibrdtn/streams/bpstreambuf.h"
#include "ibrcommon/thread/Thread.h"
#include "ibrdtn/streams/StreamConnection.h"

#include "ibrcommon/net/NetInterface.h"
#include "ibrcommon/net/tcpserver.h"
#include "ibrcommon/net/tcpclient.h"
#include "ibrcommon/net/tcpstream.h"

namespace dtn
{
	namespace testsuite
	{
		class StreamingTestSuite
		{
			class StreamChecker : public ibrcommon::JoinableThread
			{
			public:
				StreamChecker(ibrcommon::NetInterface &net, int chars = 10);
				~StreamChecker();

				void run();

				bool failed;

			private:
				ibrcommon::tcpserver _srv;
				int _chars;
			};

			class StreamConnChecker : public ibrcommon::JoinableThread
			{
			public:
				StreamConnChecker(ibrcommon::NetInterface &net, int chars = 10);
				~StreamConnChecker();

				void run();

				bool failed;

			private:
				ibrcommon::tcpserver _srv;
				int _chars;
			};

		public:
			StreamingTestSuite();
			virtual ~StreamingTestSuite();

			bool runAllTests();
			bool tcpstreamTest(int chars = 10);
			bool streamconnectionTest(int chars = 10);
		};
	}
}

#endif /* STREAMINGTESTSUITE_H_ */

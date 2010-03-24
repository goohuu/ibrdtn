/*
 * StreamingTestSuite.h
 *
 *  Created on: 16.10.2009
 *      Author: morgenro
 */

#ifndef STREAMINGTESTSUITE_H_
#define STREAMINGTESTSUITE_H_

#include "ibrdtn/default.h"
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
		class StreamingTestSuite : public dtn::streams::StreamConnection::Callback
		{
			class StreamConnChecker : public ibrcommon::JoinableThread, public dtn::streams::StreamConnection::Callback
			{
			public:
				StreamConnChecker(ibrcommon::NetInterface &net, int chars = 10);
				~StreamConnChecker();

				void run();

				bool failed;

				virtual void eventShutdown();
				virtual void eventTimeout();
				virtual void eventConnectionUp(const StreamContactHeader &header);

			private:
				ibrcommon::tcpserver _srv;
				int _chars;
			};

			class Receiver : public ibrcommon::JoinableThread
			{
			public:
				Receiver(dtn::streams::StreamConnection &conn);
				~Receiver();

				void run();

			private:
				dtn::streams::StreamConnection &_conn;
			};

		public:
			StreamingTestSuite();
			virtual ~StreamingTestSuite();

			bool runAllTests();
			bool streamconnectionTest(int chars = 10);

			virtual void eventShutdown();
			virtual void eventTimeout();
			virtual void eventConnectionUp(const StreamContactHeader &header);
		};
	}
}

#endif /* STREAMINGTESTSUITE_H_ */

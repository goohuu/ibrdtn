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

namespace dtn
{
	namespace testsuite
	{
		class StreamingTestSuite
		{
			class StreamingPeer : public std::iostream, public ibrcommon::JoinableThread
			{
			public:
				StreamingPeer(std::iostream &stream);
				void run();

				void processHeader(dtn::streams::StreamContactHeader &h);
				virtual void processSegment(dtn::streams::StreamDataSegment &seg);
				virtual void datareceived(size_t newdata, size_t complete);

			protected:
				//virtual void received(dtn::data::Bundle &b) {};
				virtual void received(dtn::streams::StreamContactHeader &h) {};
				virtual void disconnected() {};

				virtual void interrupted(dtn::data::Bundle &b, size_t position) {};

			private:
				dtn::streams::bpstreambuf _buf;

			public:
				bool _shutdown;

			};

		public:
			StreamingTestSuite();
			virtual ~StreamingTestSuite();

			bool runAllTests();
			bool commonTest();
		};
	}
}

#endif /* STREAMINGTESTSUITE_H_ */

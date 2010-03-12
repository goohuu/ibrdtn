/*
 * bpstreambuf.h
 *
 *  Created on: 02.07.2009
 *      Author: morgenro
 */

#ifndef BPSTREAMBUF_H_
#define BPSTREAMBUF_H_

#include "ibrdtn/default.h"
#include "ibrdtn/streams/StreamDataSegment.h"
#include "ibrdtn/streams/StreamContactHeader.h"
#include "ibrcommon/thread/Mutex.h"
#include "ibrcommon/thread/MutexLock.h"
#include <iostream>
#include <streambuf>
#include <queue>

using namespace std;
using namespace dtn::data;

namespace dtn
{
	namespace streams
	{
		class bpstreambuf : public std::basic_streambuf<char, std::char_traits<char> >
		{
		public:

			enum State
			{
				IDLE = 0,
				DATA_AVAILABLE = 1,
				DATA_TRANSFER = 2,
				SHUTDOWN = 3
			};

			// The size of the input and output buffers.
			static const size_t BUFF_SIZE = 1024;

			/**
			 * constructor
			 */
			bpstreambuf(iostream &stream);
			virtual ~bpstreambuf();

			virtual void shutdown();

			size_t getOutSize();

		protected:
			virtual int sync();
			virtual int overflow(int = std::char_traits<char>::eof());
			virtual int underflow();

			std::iostream& rawstream();

			friend bpstreambuf &operator<<(bpstreambuf &buf, const StreamDataSegment &seg);
			friend bpstreambuf &operator>>(bpstreambuf &buf, StreamDataSegment &seg);

			friend bpstreambuf &operator<<(bpstreambuf &buf, const StreamContactHeader &h);
			friend bpstreambuf &operator>>(bpstreambuf &buf, StreamContactHeader &h);

		private:
			ibrcommon::StatefulConditional<bpstreambuf::State, bpstreambuf::SHUTDOWN> _in_state;
			ibrcommon::StatefulConditional<bpstreambuf::State, bpstreambuf::SHUTDOWN> _out_state;

			// Input buffer
			char *in_buf_;
			// Output buffer
			char *out_buf_;

			std::iostream &_stream;

			size_t in_data_remain_;
			bool _start_of_bundle;

			size_t out_size_;
		};
	}
}

#endif /* BPSTREAMBUF_H_ */

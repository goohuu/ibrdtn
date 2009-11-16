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
#include "ibrdtn/streams/BundleStreamWriter.h"
#include "ibrdtn/streams/BundleStreamReader.h"
#include <iostream>
#include <streambuf>
#include <queue>
#include "ibrdtn/utils/Mutex.h"
#include "ibrdtn/utils/MutexLock.h"

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

			friend std::ostream &operator<<(std::ostream &stream, const StreamDataSegment &seg);
			friend std::istream &operator>>(std::istream &stream, StreamDataSegment &seg);

			friend std::ostream &operator<<(std::ostream &stream, const StreamContactHeader &h);
			friend std::istream &operator>>(std::istream &stream, StreamContactHeader &h);

		private:
			void write(const StreamDataSegment &seg, const char *data = NULL);
			void read(StreamDataSegment &seg);
			void write(const StreamContactHeader &h);
			void read(StreamContactHeader &h);

			void setState(bpstreambuf::State state);
			bool waitState(bpstreambuf::State state);
			bpstreambuf::State getState();
			bool ifState(bpstreambuf::State state);

			dtn::utils::Mutex _write_lock;
			dtn::utils::Conditional _state_changed;

			// Input buffer
			char *in_buf_;
			// Output buffer
			char *out_buf_;

			std::iostream &_stream;

			BundleStreamWriter bundlewriter_;
			BundleStreamReader bundlereader_;

			size_t in_data_remain_;
			bool _start_of_bundle;
			bpstreambuf::State _state;

                        size_t out_size_;
		};
	}
}

#endif /* BPSTREAMBUF_H_ */

/*
 * bpstreambuf.cpp
 *
 *  Created on: 14.07.2009
 *      Author: morgenro
 */

#include "ibrdtn/streams/bpstreambuf.h"
#include "ibrdtn/data/Exceptions.h"
#include "ibrdtn/streams/StreamContactHeader.h"
#include "ibrdtn/streams/StreamDataSegment.h"
#include "ibrdtn/data/Exceptions.h"

using namespace std;

namespace dtn
{
	namespace streams
	{
		bpstreambuf::bpstreambuf(iostream &stream)
			: in_buf_(new char[BUFF_SIZE]), out_buf_(new char[BUFF_SIZE]),
			  _stream(stream), _start_of_bundle(true), _state(IDLE)
		{
			// Initialize get pointer.  This should be zero so that underflow is called upon first read.
			setg(0, 0, 0);
			setp(out_buf_, out_buf_ + BUFF_SIZE - 1);

#ifdef DO_EXTENDED_DEBUG_OUTPUT
			cout << "new bpstreambuf" << endl;
#endif
		}

		bpstreambuf::~bpstreambuf()
		{
			delete [] in_buf_;
			delete [] out_buf_;
#ifdef DO_EXTENDED_DEBUG_OUTPUT
			cout << "remove bpstreambuf" << endl;
#endif
		}

		void bpstreambuf::setState(bpstreambuf::State state)
		{
			if (_state == SHUTDOWN) return;
			ibrcommon::MutexLock l(_state_changed);
			_state = state;
			_state_changed.signal(true);
		}

		bool bpstreambuf::waitState(bpstreambuf::State state)
		{
			ibrcommon::MutexLock l(_state_changed);
			bool ret = true;

			if (_state == SHUTDOWN)
			{
				ret = false;
			}

			while ((_state != state) && (ret))
			{
				_state_changed.wait();

				if (_state == SHUTDOWN) ret = false;
			}

			return ret;
		}

		bpstreambuf::State bpstreambuf::getState()
		{
			ibrcommon::MutexLock l(_state_changed);
			return _state;
		}

		bool bpstreambuf::ifState(bpstreambuf::State state)
		{
			ibrcommon::MutexLock l(_state_changed);
			return (state == _state);
		}

		void bpstreambuf::shutdown()
		{
#ifdef DO_EXTENDED_DEBUG_OUTPUT
			cout << "shutdown: bpstreambuf" << endl;
#endif
			_stream.clear(iostream::eofbit);
			setState(SHUTDOWN);
		}

		size_t bpstreambuf::getOutSize()
		{
			return out_size_;
		}

		std::iostream& bpstreambuf::rawstream()
		{
			return _stream;
		}

		bpstreambuf &operator<<(bpstreambuf &buf, const StreamDataSegment &seg)
		{
			if (buf.ifState(bpstreambuf::SHUTDOWN)) return buf;

			ibrcommon::MutexLock l(buf._write_lock);
			buf.rawstream() << seg;
		}

		bpstreambuf &operator<<(bpstreambuf &buf, const StreamContactHeader &h)
		{
			if (buf.ifState(bpstreambuf::SHUTDOWN)) return buf;

			// write data
			{
				ibrcommon::MutexLock l(buf._write_lock);
				buf.rawstream() << h;
			}

			buf.rawstream().flush();
		}

		bpstreambuf &operator>>(bpstreambuf &buf, StreamDataSegment &seg)
		{
			if (buf.ifState(bpstreambuf::SHUTDOWN)) return buf;

			if (!buf.rawstream().good()) throw dtn::exceptions::IOException("stream closed");

			// read the next segment
			buf.rawstream() >> seg;

			// if the segment header indicates new data
			if ((seg._type == StreamDataSegment::MSG_DATA_SEGMENT) && (seg._value != 0))
			{
				// set the new data length
				buf.in_data_remain_ = seg._value;

				// announce the new data block
				buf.setState(bpstreambuf::DATA_AVAILABLE);

				// and wait until the data is received completely
				buf.waitState(bpstreambuf::IDLE);
			}
		}

		bpstreambuf &operator>>(bpstreambuf &buf, StreamContactHeader &h)
		{
			if (buf.ifState(bpstreambuf::SHUTDOWN)) return buf;
			if (!buf.rawstream().good()) throw dtn::exceptions::IOException("stream closed");
			buf.rawstream() >> h;
		}

		// This function is called when the output buffer is filled.
		// In this function, the buffer should be written to wherever it should
		// be written to (in this case, the streambuf object that this is controlling).
		int bpstreambuf::overflow(int c)
		{
			char *ibegin = out_buf_;
			char *iend = pptr();

			// mark the buffer as free
			setp(out_buf_, out_buf_ + BUFF_SIZE - 1);

			// append the last character
			if(!traits_type::eq_int_type(c, traits_type::eof())) {
				*iend++ = traits_type::to_char_type(c);
			}

			// if there is nothing to send, just return
			if ((iend - ibegin) == 0)
			{
				return traits_type::not_eof(c);
			}

			// wrap a segment around the data
			StreamDataSegment seg(StreamDataSegment::MSG_DATA_SEGMENT, (iend - ibegin));

			// set the start flag
			if (_start_of_bundle)
			{
				if (!(seg._flags & 0x01)) seg._flags += 0x01;
				_start_of_bundle = false;
				out_size_ = 0;
			}

			if (char_traits<char>::eq_int_type(c, char_traits<char>::eof()))
			{
				// set the end flag
				if (!(seg._flags & 0x02)) seg._flags += 0x02;
				_start_of_bundle = true;
			}

			// write the segment to the stream
			_stream << seg;
			_stream.write(out_buf_, seg._value);

			// add size to outgoing size
			out_size_ += seg._value;

			return traits_type::not_eof(c);
		}

		// This is called to flush the buffer.
		// This is called when we're done with the file stream (or when .flush() is called).
		int bpstreambuf::sync()
		{
			int ret = traits_type::eq_int_type(this->overflow(traits_type::eof()),
											traits_type::eof()) ? -1 : 0;

//			// ... and flush.
//			_stream.flush();

			return ret;
		}

		// Fill the input buffer.  This reads out of the streambuf.
		int bpstreambuf::underflow()
		{
			if ( !waitState(DATA_AVAILABLE) )
			{
				return traits_type::eof();
			}

			setState(DATA_TRANSFER);

			// currently transferring data
			size_t readsize = BUFF_SIZE;
			if (in_data_remain_ <= BUFF_SIZE) readsize = in_data_remain_;

			// here receive the data
			_stream.read(in_buf_, readsize);

			// Since the input buffer content is now valid (or is new)
			// the get pointer should be initialized (or reset).
			setg(in_buf_, in_buf_, in_buf_ + readsize);

			// adjust the remain counter
			in_data_remain_ -= readsize;

			// if the transmission is completed, return to idle state
			if (in_data_remain_ == 0)
			{
				setState(IDLE);
			}

			return traits_type::not_eof(in_buf_[0]);
		}
	}
}

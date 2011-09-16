/*
 * lowpanstream.cpp
 */

#include "ibrcommon/config.h"
#include "ibrcommon/Logger.h"
#include "lowpanstream.h"
#include "net/LOWPANConvergenceLayer.h"
#include "ibrcommon/thread/MutexLock.h"
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>

/* Header:
 * +---------------+
 * |7 6 5 4 3 2 1 0|
 * +---------------+
 * Bit 7-6: 00 to be compatible with 6LoWPAN
 * Bit 5-4: 00 Middle segment
 *	    01 Last segment
 *	    10 First segment
 *	    11 First and last segment
 * Bit 3:   0 Extended frame not available
 *          1 Extended frame available
 * Bit 2-0: sequence number (0-7)
 *
 * Extended header (only if extended frame available)
 * +---------------+
 * |7 6 5 4 3 2 1 0|
 * +---------------+
 * Bit 7:   0 No discovery frame
 *          1 Discovery frame
 * Bit 6-0: Reserved
 *
 * Two bytes at the end of the frame are reserved for the short address of the
 * sender. This is a workaround until recvfrom() gets fixed.
 */
#define SEGMENT_FIRST   0x25
#define SEGMENT_LAST    0x10
#define SEGMENT_BOTH    0x30
#define SEGMENT_MIDDLE  0x00
#define EXTENDED_MASK   0x04

namespace dtn
{
	namespace net
	{
		lowpanstream::lowpanstream(lowpanstream_callback &callback, unsigned int address) :
			std::iostream(this), in_buf_(new char[BUFF_SIZE]), out_buf_(new char[BUFF_SIZE]), out2_buf_(new char[BUFF_SIZE]), _address(address), callback(callback), in_seq_num_(0), out_seq_num_(0)
		{
			// Initialize get pointer.  This should be zero so that underflow is called upon first read.
			setg(0, 0, 0);
			setp(out_buf_ + 1, out_buf_ + BUFF_SIZE - 1);
		}

		lowpanstream::~lowpanstream()
		{
			delete[] in_buf_;
			delete[] out_buf_;
		}

		void lowpanstream::queue(char *buf, int len)
		{
			ibrcommon::MutexLock l(in_buf_cond);
			in_buf_len = len;

			cout << "Buffer received in lowpanstream::queue() " << buf << endl;

			while (!in_buf_free)
			{
				in_buf_cond.wait();
			}
			memcpy(in_buf_, buf, len);
			in_buf_free = false;
			in_buf_cond.signal();
		}

		// close mit abort für conditional einführen

		int lowpanstream::sync()
		{
			int ret = std::char_traits<char>::eq_int_type(this->overflow(
					std::char_traits<char>::eof()), std::char_traits<char>::eof()) ? -1
					: 0;
			//Header vorbereiten. Hier erkenne ich wenn das letzte Segment kommt
			return ret;
		}

		int lowpanstream::overflow(int c)
		{
			char *ibegin = out_buf_;
			char *iend = pptr();

			// mark the buffer as free
			setp(out_buf_, out_buf_ + BUFF_SIZE - 1);

			if (!std::char_traits<char>::eq_int_type(c, std::char_traits<char>::eof()))
			{
				*iend++ = std::char_traits<char>::to_char_type(c);
			}

			// bytes to send
			size_t bytes = (iend - ibegin);

			// if there is nothing to send, just return
			if (bytes == 0)
			{
				IBRCOMMON_LOGGER_DEBUG(90) << "lowpanstream::overflow() nothing to sent" << IBRCOMMON_LOGGER_ENDL;
				return std::char_traits<char>::not_eof(c);
			}

			out_seq_num_++;
			if (out_seq_num_ == 8)
				out_seq_num_ = 0;

			out_buf_[0] = SEGMENT_MIDDLE+ out_seq_num_;

			// set write lock
			ibrcommon::MutexLock l(m_writelock);

			// Send segment to CL, use callback interface
			//int ret;// = LOWPANConvergenceLayer::send_cb(out_buf_, bytes + 1, _address);
			callback.send_cb(out_buf_, bytes + 1, _address);
#if 0
			if (ret == -1)
			{
				// CL is busy, requeue bundle
				//dtn::routing::RequeueBundleEvent::raise(job._destination, job._bundle);
				return ret;
			}
			if (ret < 0)
			{
				// failure
				close();
				std::stringstream ss; ss << "<lowpanstream> send() in lowpanstream failed: " << errno;
			} else {
				// check how many bytes are sent
				if ((size_t)ret < bytes)
				{
					// we did not sent all bytes
					char *resched_begin = ibegin + ret;
					char *resched_end = iend;

					// bytes left to send
					size_t bytes_left = resched_end - resched_begin;

					// move the data to the begin of the buffer
					::memcpy(ibegin, resched_begin, bytes_left);

					// new free buffer
					char *buffer_begin = ibegin + bytes_left;

					// mark the buffer as free
					setp(buffer_begin, out_buf_ + BUFF_SIZE - 1);
				}
			}
#endif

			// raise bundle event
			//dtn::net::TransferCompletedEvent::raise(job._destination, bundle);
			//dtn::core::BundleEvent::raise(bundle, dtn::core::BUNDLE_FORWARDED);

			return std::char_traits<char>::not_eof(c);
	}

		int lowpanstream::underflow()
		{
			ibrcommon::MutexLock l(in_buf_cond);

			while (in_buf_free)
			{
				in_buf_cond.wait();
			}
			memcpy(out2_buf_ ,in_buf_, in_buf_len);
			in_buf_free = true;
			in_buf_cond.signal();

			// Since the input buffer content is now valid (or is new)
			// the get pointer should be initialized (or reset).
			setg(out2_buf_, out2_buf_, out2_buf_ + in_buf_len);

			return std::char_traits<char>::not_eof(out2_buf_[0]);
		}
	}
}

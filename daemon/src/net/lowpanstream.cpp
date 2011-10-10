#include "ibrcommon/config.h"
#include "ibrcommon/Logger.h"
#include "lowpanstream.h"
#include "net/LOWPANConvergenceLayer.h"
#include "ibrcommon/thread/MutexLock.h"
#include <signal.h>
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
#define SEGMENT_FIRST   0x20
#define SEGMENT_LAST    0x10
//#define SEGMENT_BOTH    0x30
#define SEGMENT_MIDDLE  0x00

#define SEQ_NUM_MASK	0x07

namespace dtn
{
	namespace net
	{
		lowpanstream::lowpanstream(lowpanstream_callback &callback, unsigned int address) :
			std::iostream(this), _in_first_segment(true), _out_stat(SEGMENT_FIRST), in_buf_(new char[BUFF_SIZE]), out_buf_(new char[BUFF_SIZE+2]), out2_buf_(new char[BUFF_SIZE]), _address(address), callback(callback), in_seq_num_(0), out_seq_num_(0), in_buf_free(true), out_seq_num_global(0), _abort(false)
		{
			// Initialize get pointer. This should be zero so that underflow is called upon first read.
			setg(0, 0, 0);
			setp(out_buf_ + 1, out_buf_ + BUFF_SIZE -1);
		}

		lowpanstream::~lowpanstream()
		{
			delete[] in_buf_;
			delete[] out_buf_;
			delete[] out2_buf_;
		}

		void lowpanstream::abort()
		{
			ibrcommon::MutexLock l(in_buf_cond);

			while (!in_buf_free)
			{
				in_buf_cond.wait();
			}

			_abort = true;
			in_buf_cond.signal();
		}

		void lowpanstream::queue(char *buf, int len)
		{
			ibrcommon::MutexLock l(in_buf_cond);

			IBRCOMMON_LOGGER_DEBUG(10) << "lowpanstream::queue"<< IBRCOMMON_LOGGER_ENDL;

			// Retrieve sequence number of frame
			unsigned int seq_num = buf[0] & SEQ_NUM_MASK;

			IBRCOMMON_LOGGER_DEBUG(45) << "Received frame sequence number: " << seq_num << IBRCOMMON_LOGGER_ENDL;

			// Check if the sequence number is what we expect
			if (in_seq_num_ != seq_num)
			{
				IBRCOMMON_LOGGER(error) << "Received frame with out of bound sequence number (" << seq_num << " expected " << (int)in_seq_num_ << ")"<< IBRCOMMON_LOGGER_ENDL;
				_abort = true;
				in_buf_cond.signal();
				return;
			}

			// increment the sequence number
			in_seq_num_ = (in_seq_num_ + 1) % 8;

			// check if this is the right segment
			if (_in_first_segment)
			{
				if (!(buf[0] & SEGMENT_FIRST)) return;
				if (!(buf[0] & SEGMENT_LAST)) _in_first_segment = false;
			}

			while (!in_buf_free)
			{
				in_buf_cond.wait();
			}

			memcpy(in_buf_, buf + 1, len - 1);
			in_buf_len = len - 1;
			in_buf_free = false;
			in_buf_cond.signal();
		}

		// close woth abort for conditional

		int lowpanstream::sync()
		{
			// Here we know we get the last segment. Mark it so.
			_out_stat |= SEGMENT_LAST;

			int ret = std::char_traits<char>::eq_int_type(this->overflow(
					std::char_traits<char>::eof()), std::char_traits<char>::eof()) ? -1
					: 0;

			IBRCOMMON_LOGGER_DEBUG(10) << "lowpanstream::sync"<< IBRCOMMON_LOGGER_ENDL;

			return ret;
		}

		int lowpanstream::overflow(int c)
		{
			char *ibegin = out_buf_;
			char *iend = pptr();

			IBRCOMMON_LOGGER_DEBUG(10) << "lowpanstream::overflow"<< IBRCOMMON_LOGGER_ENDL;

			// mark the buffer as free
			setp(out_buf_ + 1, out_buf_ + BUFF_SIZE - 1);

			if (!std::char_traits<char>::eq_int_type(c, std::char_traits<char>::eof()))
			{
				*iend++ = std::char_traits<char>::to_char_type(c);
			}

			// bytes to send
			size_t bytes = (iend - ibegin);

			// if there is nothing to send, just return
			if (bytes == 0)
			{
				IBRCOMMON_LOGGER_DEBUG(10) << "lowpanstream::overflow() nothing to sent" << IBRCOMMON_LOGGER_ENDL;
				return std::char_traits<char>::not_eof(c);
			}

			//FIXME: Should we write in the segment position here?
			out_buf_[0] = 0x07 & out_seq_num_;
			out_buf_[0] |= _out_stat;

			out_seq_num_global++;
			out_seq_num_ = (out_seq_num_ + 1) % 8;

			IBRCOMMON_LOGGER_DEBUG(10) << "lowpanstream send segment " << (int)out_seq_num_ << " / " << out_seq_num_global << IBRCOMMON_LOGGER_ENDL;

			// Send segment to CL, use callback interface
			callback.send_cb(out_buf_, bytes, _address);

			if (_out_stat & SEGMENT_LAST)
			{
				// reset outgoing status byte
				_out_stat = SEGMENT_FIRST;
			}
			else
			{
				_out_stat = SEGMENT_MIDDLE;
			}

			return std::char_traits<char>::not_eof(c);
	}

		int lowpanstream::underflow()
		{
			ibrcommon::MutexLock l(in_buf_cond);

			IBRCOMMON_LOGGER_DEBUG(10) << "lowpanstream::underflow"<< IBRCOMMON_LOGGER_ENDL;

			while (in_buf_free)
			{
				if (_abort) throw ibrcommon::Exception("stream aborted");
				in_buf_cond.wait();
			}
			memcpy(out2_buf_ ,in_buf_, in_buf_len);
			in_buf_free = true;
			IBRCOMMON_LOGGER_DEBUG(10) << "lowpanstream::underflow in_buf_free: " << in_buf_free << IBRCOMMON_LOGGER_ENDL;
			in_buf_cond.signal();

			// Since the input buffer content is now valid (or is new)
			// the get pointer should be initialized (or reset).
			setg(out2_buf_, out2_buf_, out2_buf_ + in_buf_len);

			return std::char_traits<char>::not_eof(out2_buf_[0]);
		}
	}
}

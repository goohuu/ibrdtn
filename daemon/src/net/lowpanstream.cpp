/*
 * lowpanstream.cpp
 */

#include "ibrcommon/config.h"
#include "ibrcommon/Logger.h"
#include "lowpanstream.h"
#include "ibrcommon/thread/MutexLock.h"
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>

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
			std::iostream(this), in_buf_(new char[BUFF_SIZE]), out_buf_(new char[BUFF_SIZE]), out2_buf_(new char[BUFF_SIZE]), _address(address)
		{
			// Initialize get pointer.  This should be zero so that underflow is called upon first read.
			setg(0, 0, 0);
			setp(out_buf_, out_buf_ + BUFF_SIZE - 1);
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

			while (!in_buf_free)
			{
				in_buf_cond.wait();
			}
			memcpy(in_buf_, buf, len);
			in_buf_free = false;
			in_buf_cond.signal();
		}

		int lowpanstream::sync()
		{
			int ret = std::char_traits<char>::eq_int_type(this->overflow(
					std::char_traits<char>::eof()), std::char_traits<char>::eof()) ? -1
					: 0;
			//Header vorbereiten. Hier alknde ich wenn das letzte Segment kommt
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

			// if there is nothing to send, just return
			if ((iend - ibegin) == 0)
			{
				IBRCOMMON_LOGGER_DEBUG(90) << "lowpanstream::overflow() nothing to sent" << IBRCOMMON_LOGGER_ENDL;
				return std::char_traits<char>::not_eof(c);
			}

			bool read = false, write = true, error = false;

			// bytes to send
			size_t bytes = (iend - ibegin);

			if (bytes > 114) {
				std::string chunk, tmp;
				char header = 0;
				int i, seq_num;
				int chunks = ceil(bytes / 114.0);
				cout << "Bundle to big to fit into one packet. Need to split into " << dec << chunks << " segments" << endl;
				for (i = 0; i < chunks; i++) {
					stringstream buf;
					//chunk = data.substr(i * 114, 114);
					seq_num =  i % 16;

					printf("Iteration %i with seq number %i, from %i chunks\n", i, seq_num, chunks);
					if (i == 0) // First segment
						header = SEGMENT_FIRST + seq_num;
					else if (i == (chunks - 1)) // Last segment
						header = SEGMENT_LAST + seq_num;
					else
						header = SEGMENT_MIDDLE+ seq_num;

					buf << header;
					tmp = buf.str() + chunk; // Prepand header to chunk
					chunk = "";
					chunk = tmp;

					// set write lock
					ibrcommon::MutexLock l(m_writelock);

					// Send segment to CL, use callback
					// interface
					int ret;
//					int ret = callback::send(chunk.c_str(), chunk.length(), _address);

					if (ret == -1)
					{
						// CL is busy, requeue bundle
						//dtn::routing::RequeueBundleEvent::raise(job._destination, job._bundle);
						return ret;
					}
				}
				// raise bundle event
				//dtn::net::TransferCompletedEvent::raise(job._destination, bundle);
				//dtn::core::BundleEvent::raise(bundle, dtn::core::BUNDLE_FORWARDED);
			} else {
				std::string tmp;
				stringstream buf;

				buf << SEGMENT_BOTH;

				//tmp = buf.str() + data; // Prepand header to chunk
				//data = "";
				//data = tmp;

				// set write lock
				ibrcommon::MutexLock l(m_writelock);

				// send converted line back to client.
				int ret;
				//int ret = lowpanstream_callback::send(data.c_str(), data.length(), _address);

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
					int ret;
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
			}
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

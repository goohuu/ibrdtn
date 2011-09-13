/*
 * lowpanstream.cpp
 *
 *  Created on: 29.07.2009
 *      Author: morgenro
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

/* TODO
Use recv and send functionality from CL
Use this stream in CL
*/

namespace ibrcommon
{
	lowpanstream::lowpanstream(lowpanstream_callback &callback, unsigned int address) :
		std::iostream(this), in_buf_(new char[BUFF_SIZE]), out_buf_(new char[BUFF_SIZE])
	{
		// Initialize get pointer.  This should be zero so that underflow is called upon first read.
		setg(0, 0, 0);
		setp(out_buf_, out_buf_ + BUFF_SIZE - 1);
	}

	lowpanstream::~lowpanstream()
	{
		delete[] in_buf_;
		delete[] out_buf_;

		// finally, close the socket
		close();
	}

	void lowpanstream::close()
	{
		static ibrcommon::Mutex close_lock;
		ibrcommon::MutexLock l(close_lock);
	}

	int lowpanstream::sync()
	{
		int ret = std::char_traits<char>::eq_int_type(this->overflow(
				std::char_traits<char>::eof()), std::char_traits<char>::eof()) ? -1
				: 0;

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

			// send the data
			//ssize_t ret = ::send(_socket, out_buf_, (iend - ibegin), 0);

#if 0
				// get a lowpan peer
				ibrcommon::lowpansocket::peer p = _socket->getPeer(address, pan);

				if (data.length() > 114) {
					std::string chunk, tmp;
					char header = 0;
					int i, seq_num;
					int chunks = ceil(data.length() / 114.0);
					cout << "Bundle to big to fit into one packet. Need to split into " << dec << chunks << " segments" << endl;
					for (i = 0; i < chunks; i++) {
						stringstream buf;
						chunk = data.substr(i * 114, 114);

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

						// send converted line back to client.
						int ret = p.send(chunk.c_str(), chunk.length());

						if (ret == -1)
						{
							// CL is busy, requeue bundle
							dtn::routing::RequeueBundleEvent::raise(job._destination, job._bundle);

							return;
						}
					}
					// raise bundle event
					dtn::net::TransferCompletedEvent::raise(job._destination, bundle);
					dtn::core::BundleEvent::raise(bundle, dtn::core::BUNDLE_FORWARDED);
				} else {

					std::string tmp;
					stringstream buf;

					buf << SEGMENT_BOTH;

					tmp = buf.str() + data; // Prepand header to chunk
					data = "";
					data = tmp;

					// set write lock
					ibrcommon::MutexLock l(m_writelock);

					// send converted line back to client.
					int ret = p.send(data.c_str(), data.length());

					if (ret == -1)
					{
						// CL is busy, requeue bundle
						dtn::routing::RequeueBundleEvent::raise(job._destination, job._bundle);

						return;
					}

					// raise bundle event
					dtn::net::TransferCompletedEvent::raise(job._destination, bundle);
					dtn::core::BundleEvent::raise(bundle, dtn::core::BUNDLE_FORWARDED);

			if (ret < 0)
			{
				// failure
				close();
				std::stringstream ss; ss << "<lowpanstream> send() in lowpanstream failed: " << errno;
				throw ConnectionClosedException(ss.str());
			}
			else
#endif
			{
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
		return std::char_traits<char>::not_eof(c);
	}

	int lowpanstream::underflow()
	{
			bool read = true;
			bool write = false;
			bool error = false;

			int bytes;
#if 0
			char header, tmp;
			stringstream ss;

			// read some bytes
			//int bytes = ::recv(_socket, in_buf_, BUFF_SIZE, 0);
			int bytes = _socket->receive(in_buf_, BUFF_SIZE);

			if (len <= 0)
				return (*this);

			header = data[0];
			header &= 0xF0; // Clear seq number bits
			ss.write(data+1, len-1);

			while (header != SEGMENT_LAST) {

				len = _socket->receive(data, m_maxmsgsize);
				header = data[0];
				header &= 0xF0;

				if (len > 0)
					ss.write(data+1, len-1);
			}

			if (len > 0)
				dtn::data::DefaultDeserializer(ss, dtn::core::BundleCore::getInstance()) >> bundle;
#endif

			// end of stream
			if (bytes == 0)
			{
				close();
				IBRCOMMON_LOGGER_DEBUG(40) << "<lowpanstream> recv() returned zero: " << errno << IBRCOMMON_LOGGER_ENDL;
				return std::char_traits<char>::eof();
			}
			else if (bytes < 0)
			{
				close();
				IBRCOMMON_LOGGER_DEBUG(40) << "<lowpanstream> recv() failed: " << errno << IBRCOMMON_LOGGER_ENDL;
				return std::char_traits<char>::eof();
			}

			// Since the input buffer content is now valid (or is new)
			// the get pointer should be initialized (or reset).
			setg(in_buf_, in_buf_, in_buf_ + bytes);

			return std::char_traits<char>::not_eof(in_buf_[0]);
	}
}

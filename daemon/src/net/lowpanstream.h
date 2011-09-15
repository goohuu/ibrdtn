/*
 * lowpanstream.h
 */

#ifndef IBRCOMMON_LOWPANSTREAM_H_
#define IBRCOMMON_LOWPANSTREAM_H_

#include "ibrcommon/Exceptions.h"
#include <streambuf>
#include <sys/types.h>
#include <sys/socket.h>
#include <iostream>
#include "ibrcommon/net/vsocket.h"
#include "ibrcommon/thread/Conditional.h"

namespace dtn
{
	namespace net
	{
		class lowpanstream_callback
		{
		public:
			virtual void send(char *buf, int len, unsigned int address) = 0;
		};

		class lowpanstream : public std::basic_streambuf<char, std::char_traits<char> >, public std::iostream
		{
		public:
			// The size of the input and output buffers.
			static const size_t BUFF_SIZE = 115;

			lowpanstream(lowpanstream_callback &callback, unsigned int address);
			virtual ~lowpanstream();

			void queue(char *buf, int len);

			void close();

		protected:
			virtual int sync();
			virtual int overflow(int = std::char_traits<char>::eof());
			virtual int underflow();

			unsigned int _address;

		private:
			// Input buffer
			char *in_buf_;
			int in_buf_len;
			bool in_buf_free;

			// Output buffer
			char *out_buf_;
			char *out2_buf_;

			ibrcommon::Conditional in_buf_cond;
			ibrcommon::Mutex m_writelock;
			ibrcommon::Mutex m_readlock;
		};
	}
}
#endif /* LOWPANSTREAM_H_ */

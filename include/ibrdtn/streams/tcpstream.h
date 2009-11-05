/*
 * tcpstream.h
 *
 *  Created on: 29.07.2009
 *      Author: morgenro
 */

#ifndef TCPSTREAM_H_
#define TCPSTREAM_H_

#include "ibrdtn/default.h"
#include <streambuf>
#include <sys/types.h>
#include <sys/socket.h>
#include "ibrdtn/data/Exceptions.h"

namespace dtn
{
	namespace streams
	{
		class ConnectionClosedException : public dtn::exceptions::Exception
		{
		public:
			ConnectionClosedException(string what = "The connection has been closed.") throw() : Exception(what)
			{
			};
		};

		class tcpstream : public std::basic_streambuf<char, std::char_traits<char> >, public std::iostream
		{
		public:
			enum stream_direction {
				STREAM_INCOMING = 0,
				STREAM_OUTGOING = 1
			};

			// The size of the input and output buffers.
			static const size_t BUFF_SIZE = 2048;

			tcpstream(int socket = 0, stream_direction d = STREAM_OUTGOING);
			virtual ~tcpstream();

			string getAddress(stream_direction d = STREAM_INCOMING) const;
			int getPort(stream_direction d = STREAM_INCOMING) const;

			virtual void close();

		protected:
			virtual int sync();
			virtual int overflow(int = std::char_traits<char>::eof());
			virtual int underflow();

			void setSocket(int socket);

		private:
			tcpstream(const tcpstream &s) {};

			int _socket;

			// Input buffer
			char *in_buf_;
			// Output buffer
			char *out_buf_;

			bool _closed;

			stream_direction _direction;

//			dtn::utils::Mutex _overflow_lock;
//			dtn::utils::Mutex _underflow_lock;
		};
	}
}

#endif /* TCPSTREAM_H_ */

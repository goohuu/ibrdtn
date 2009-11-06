/*
 * tcpstream.cpp
 *
 *  Created on: 29.07.2009
 *      Author: morgenro
 */

#include "ibrdtn/streams/tcpstream.h"
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

namespace dtn
{
	namespace streams
	{
		tcpstream::tcpstream(int socket, stream_direction d)
		 : iostream(this), _socket(0), in_buf_(new char[BUFF_SIZE]), out_buf_(new char[BUFF_SIZE]), _closed(false)
		{
			if (socket > 0) setSocket(socket);
		}

		tcpstream::~tcpstream()
		{
			delete [] in_buf_;
			delete [] out_buf_;
#ifdef DO_EXTENDED_DEBUG_OUTPUT
			cout << "remove tcpstream" << endl;
#endif
		}

		void tcpstream::setSocket(int socket)
		{
			_socket = socket;

			// Initialize get pointer.  This should be zero so that underflow is called upon first read.
			setg(0, 0, 0);
			setp(out_buf_, out_buf_ + BUFF_SIZE - 1);
#ifdef DO_EXTENDED_DEBUG_OUTPUT
			cout << "new tcpstream, " << getAddress() << ":" << getPort() << " <-> " << getAddress(STREAM_OUTGOING) << ":" << getPort(STREAM_OUTGOING) << endl;
#endif
		}

		string tcpstream::getAddress(stream_direction d) const
		{
			struct ::sockaddr_in sa;
			int iLen = sizeof(sa);

			if (d == STREAM_INCOMING)
			{
				getpeername(_socket, (sockaddr*)&sa, (socklen_t*)&iLen);
			}
			else
			{
				getsockname(_socket, (sockaddr*)&sa, (socklen_t*)&iLen);
			}

			return inet_ntoa(sa.sin_addr);
		}

		int tcpstream::getPort(stream_direction d) const
		{
			struct ::sockaddr_in sa;
			int iLen = sizeof(sa);

			if (d == STREAM_INCOMING)
			{
				getpeername(_socket, (sockaddr*)&sa, (socklen_t*)&iLen);
			}
			else
			{
				getsockname(_socket, (sockaddr*)&sa, (socklen_t*)&iLen);
			}

			return ntohs(sa.sin_port);
		}

		void tcpstream::close()
		{
			if (_closed) return;
#ifdef DO_EXTENDED_DEBUG_OUTPUT
			cout << "shutdown: tcpstream" << endl;
#endif
			::shutdown(_socket, SHUT_RDWR);
			::close(_socket);
			_closed = true;
		}

		int tcpstream::sync()
		{
			int ret = std::char_traits<char>::eq_int_type(this->overflow(std::char_traits<char>::eof()),
					std::char_traits<char>::eof()) ? -1 : 0;

			return ret;
		}

		int tcpstream::overflow(int c)
		{
			if ( _closed )
			{
				//throw ConnectionClosedException("Connection has been closed.");
				return std::char_traits<char>::eof();
			}

			char *ibegin = out_buf_;
			char *iend = pptr();

			// mark the buffer as free
			setp(out_buf_, out_buf_ + BUFF_SIZE - 1);

			if(!std::char_traits<char>::eq_int_type(c, std::char_traits<char>::eof())) {
				*iend++ = std::char_traits<char>::to_char_type(c);
			}

			// if there is nothing to send, just return
			if ((iend - ibegin) == 0)
			{
				return std::char_traits<char>::not_eof(c);
			}

			// send the data
			if ( ::send(_socket, out_buf_, (iend - ibegin), 0) == -1 )
			{
				// failure
				//throw ConnectionClosedException("Error while writing to the socket.");
				close();
				return std::char_traits<char>::eof();
			}

			if ( _closed )
			{
				//throw ConnectionClosedException("Connection has been closed.");
				return std::char_traits<char>::eof();
			}

			return std::char_traits<char>::not_eof(c);
		}

		int tcpstream::underflow()
		{
			// end of stream
			if ( _closed  )
			{
				//  throw Exception?
				return std::char_traits<char>::eof();
			}

			int bytes = ::recv(_socket, in_buf_, BUFF_SIZE, 0);

			// end of stream
			if ( _closed || ( bytes <= 0 ) )
			{
				//  throw Exception?
				//throw ConnectionClosedException("Connection has been closed.");
				close();
				return std::char_traits<char>::eof();
			}

			// Since the input buffer content is now valid (or is new)
			// the get pointer should be initialized (or reset).
			setg(in_buf_, in_buf_, in_buf_ + bytes);

			return std::char_traits<char>::not_eof(in_buf_[0]);
		}
	}
}

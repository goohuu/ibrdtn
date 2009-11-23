/*
 * tcpserver.cpp
 *
 *  Created on: 30.07.2009
 *      Author: morgenro
 */

#include "ibrdtn/utils/tcpserver.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/poll.h>

namespace dtn
{
	namespace utils
	{
		tcpserver::tcpserver(string address, int port)
		 : _socket(0), _closed(false)
		{
			struct sockaddr_in sock_address;

			// define the protocol family
			sock_address.sin_family = AF_INET;

			// set the local interface address
			sock_address.sin_addr.s_addr = inet_addr(address.c_str());

			// set the local port
			sock_address.sin_port = htons(port);

			// Create socket for listening for client connection requests.
			_socket = socket(AF_INET, SOCK_STREAM, 0);

			// check for errors
			if (_socket < 0)
			{
				throw SocketException("TCPConvergenceLayer: cannot create listen socket");
			}

			// bind to the socket
			if ( bind(_socket, (struct sockaddr *) &sock_address, sizeof(sock_address)) < 0 )
			{

				throw SocketException("TCPConvergenceLayer: cannot bind socket");
			}

			// listen on the socket, max. 5 concurrent awaiting connections
			listen (_socket, 5);
		}

		tcpserver::~tcpserver()
		{
			close();
		}

		void tcpserver::close()
		{
			_closed = true;
			::shutdown(_socket, SHUT_RD);
			::close(_socket);
		}

		bool tcpserver::waitPending(int timeout)
		{
			// return, if the socket is invalid
			if(_socket == -1)
				return false;

			// define the descriptor-array
			struct pollfd pfd;
			pfd.fd = _socket;
			pfd.revents = 0;
			pfd.events = POLLIN;

			// wait for a connection
			int status = ::poll(&pfd, 1, timeout);

			if (_socket == -1) return false;
			if (_closed) return false;

			if ((status == -1) && (errno == EINTR))
			{
				// Interrupted system call
				return false;
			}
			else if (status > 0)
			{
				// connection awaiting
				return true;
			}
			else if (status == 0)
			{
				// timeout
				return false;
			}

			return false;
		}

		int tcpserver::accept()
		{
			return ::accept(_socket, NULL, NULL );
		}
	}
}

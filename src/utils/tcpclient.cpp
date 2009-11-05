/*
 * tcpclient.cpp
 *
 *  Created on: 29.07.2009
 *      Author: morgenro
 */

#include "ibrdtn/utils/tcpclient.h"
#include <sys/socket.h>
#include <streambuf>
#include <netinet/in.h>
#include <arpa/inet.h>



namespace dtn
{
	namespace utils
	{
		tcpclient::tcpclient(string address, int port)
		{
			struct sockaddr_in sock_address;
			int sock = socket(AF_INET, SOCK_STREAM, 0);

			if (sock <= 0)
			{
				// error
				throw SocketException("Could not create a socket.");
			}

			sock_address.sin_family = AF_INET;
			sock_address.sin_addr.s_addr = inet_addr(address.c_str());
			sock_address.sin_port = htons(port);

			if (connect ( sock, (struct sockaddr *) &sock_address, sizeof (sock_address)) != 0)
			{
				// error
				throw SocketException("Could not connect to the server.");
			}

			setSocket(sock);
		}

		tcpclient::~tcpclient()
		{
		}

		void tcpclient::close()
		{
			tcpstream::close();
		}
	}
}

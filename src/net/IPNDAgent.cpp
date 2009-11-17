/*
 * IPNDAgent.cpp
 *
 *  Created on: 14.09.2009
 *      Author: morgenro
 */

#include "net/IPNDAgent.h"
#include "ibrdtn/data/Exceptions.h"
#include <sstream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

namespace dtn
{
	namespace net
	{
		IPNDAgent::IPNDAgent(string broadcast_ip, size_t port)
		 : _port(port), _ip(broadcast_ip)
		{
			struct sockaddr_in address;

			address.sin_addr.s_addr = inet_addr(broadcast_ip.c_str());
			address.sin_port = htons(port);

			// Create socket for listening for client connection requests.
			_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

			if (_socket < 0)
			{
				cerr << "IPNDAgent: cannot create listen socket" << endl;
				::exit(1);
			}

			// enable broadcast
			int b = 1;
			if ( setsockopt(_socket, SOL_SOCKET, SO_BROADCAST, (char*)&b, sizeof(b)) == -1 )
			{
				cerr << "IPNDAgent: cannot send broadcasts" << endl;
				::exit(1);
			}

			// broadcast addresses should be usable more than once.
			setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&b, sizeof(b));

			address.sin_family = AF_INET;

			if ( bind(_socket, (struct sockaddr *) &address, sizeof(address)) < 0 )
			{
				cerr << "IPNDAgent: cannot bind socket" << endl;
				::exit(1);
			}

			cout << "DiscoveryAgent listen to " << broadcast_ip << ":" << port << endl;
		}

		IPNDAgent::~IPNDAgent()
		{
			::shutdown(_socket, SHUT_RDWR);
			::close(_socket);
		}

		void IPNDAgent::send(DiscoveryAnnouncement &announcement)
		{
                        // update service blocks
                        announcement.updateServices();

			stringstream ss;
			ss << announcement;

			string data = ss.str();

			struct sockaddr_in clientAddress;

			// destination parameter
			clientAddress.sin_family = AF_INET;

			// broadcast
			clientAddress.sin_addr.s_addr = inet_addr(_ip.c_str());
			clientAddress.sin_port = htons(_port);

			// send converted line back to client.
			int ret = sendto(_socket, data.c_str(), data.length(), 0, (struct sockaddr *) &clientAddress, sizeof(clientAddress));
		}

		void IPNDAgent::run()
		{
			while (_running)
			{
				DiscoveryAnnouncement announce;

				struct sockaddr_in clientAddress;
				socklen_t clientAddressLength = sizeof(clientAddress);
				char data[1500];

				int len = recvfrom(_socket, data, 1500, MSG_WAITALL, (struct sockaddr *) &clientAddress, &clientAddressLength);

				if (len < 0) return;

				stringstream ss;
				ss.write(data, len);

				try {
					ss >> announce;
					received(announce);
				} catch (dtn::exceptions::InvalidDataException ex) {
				} catch (dtn::exceptions::IOException ex) {
				}

				yield();
			}
		}
	}
}

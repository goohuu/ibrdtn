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
#include <string.h>

namespace dtn
{
	namespace net
	{
		IPNDAgent::IPNDAgent(NetInterface net)
		 : _interface(net)
		{
			// Create socket for listening for client connection requests.
			_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

			if (_socket < 0)
			{
				cerr << "IPNDAgent: cannot create listen socket" << endl;
				::exit(1);
			}

			struct sockaddr_in address;
			bzero((char *) &address, sizeof(address));
			address.sin_family = AF_INET;
			net.getInterfaceAddress(&address.sin_addr);
			address.sin_port = htons(net.getPort());

			// enable broadcast
			int b = 1;
			if ( setsockopt(_socket, SOL_SOCKET, SO_BROADCAST, (char*)&b, sizeof(b)) == -1 )
			{
				cerr << "IPNDAgent: cannot send broadcasts" << endl;
				::exit(1);
			}

			if ( bind(_socket, (struct sockaddr *) &address, sizeof(address)) < 0 )
			{
				cerr << "IPNDAgent: cannot bind socket to " << net.getAddress() << ":" << net.getPort() << endl;
				::exit(1);
			}

			cout << "DiscoveryAgent listen to " << net.getAddress() << ":" << net.getPort() << endl;
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
			_interface.getInterfaceBroadcastAddress(&clientAddress.sin_addr);
			clientAddress.sin_port = htons(_interface.getPort());

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

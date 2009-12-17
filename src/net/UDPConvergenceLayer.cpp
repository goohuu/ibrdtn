#include "net/UDPConvergenceLayer.h"
#include "net/UDPConnection.h"
#include "core/EventSwitch.h"
#include "core/RouteEvent.h"
#include "core/BundleEvent.h"

#include "ibrdtn/streams/BundleFactory.h"
#include "ibrdtn/streams/BundleStreamReader.h"
#include "ibrcommon/data/BLOB.h"

#include "ibrdtn/utils/Utils.h"
#include <sys/socket.h>
#include <poll.h>
#include <errno.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>

#include <iostream>


using namespace dtn::data;

namespace dtn
{
	namespace net
	{
		const int UDPConvergenceLayer::DEFAULT_PORT = 4556;

		UDPConvergenceLayer::UDPConvergenceLayer(ibrcommon::NetInterface net, bool broadcast, unsigned int mtu)
			: _net(net), m_maxmsgsize(mtu), _running(true)
		{
			struct sockaddr_in address;

			net.getInterfaceAddress(&(address.sin_addr));
		  	address.sin_port = htons(net.getPort());

			// Create socket for listening for client connection requests.
			m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

			if (m_socket < 0)
			{
				throw SocketException("UDPConvergenceLayer: cannot create listen socket");
			}

			// enable broadcast
			int b = 1;

			if (broadcast)
			{
				if ( setsockopt(m_socket, SOL_SOCKET, SO_BROADCAST, (char*)&b, sizeof(b)) == -1 )
				{
					throw SocketException("UDPConvergenceLayer: cannot send broadcasts");
				}

				// broadcast addresses should be usable more than once.
				setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&b, sizeof(b));
			}

			address.sin_family = AF_INET;

			if ( bind(m_socket, (struct sockaddr *) &address, sizeof(address)) < 0 )
			{
				throw SocketException("UDPConvergenceLayer: cannot bind socket");
			}
		}

		UDPConvergenceLayer::~UDPConvergenceLayer()
		{
			_running = false;
			::shutdown(m_socket, SHUT_RDWR);
			join();
		}

                void UDPConvergenceLayer::update(std::string &name, std::string &data)
                {
                    // TODO: update the values
                }

		TransmitReport UDPConvergenceLayer::transmit(const Bundle &b)
		{
			unsigned int size = b.getSize();

			if (size > m_maxmsgsize)
			{
				// get the payload block
				PayloadBlock *payload = utils::Utils::getPayloadBlock( b );
				size_t application_length = payload->getBLOB().getSize();

				// the new size for the payload
				size_t payload_size = m_maxmsgsize;

				// reduce by the overhead
				payload_size -= (size - application_length);

				// possible size of fragment offset and application data length
				payload_size -= 20;

				// split the payload block
				pair<PayloadBlock*, PayloadBlock*> frags = payload->split(payload_size);

				Bundle frag1 = b;
				frag1.clearBlocks();
				frag1.addBlock(frags.first);

				Bundle frag2 = b;
				frag2.clearBlocks();
				frag2.addBlock(frags.second);
				frag2._fragmentoffset += payload_size;

				if (!(b._procflags & Bundle::FRAGMENT))
				{
					frag1._procflags += Bundle::FRAGMENT;
					frag1._appdatalength += application_length;
					frag2._procflags += Bundle::FRAGMENT;
					frag2._appdatalength += application_length;
				}

				// transfer the fragment
				transmit(frag1);

				// return the fragment to the router
				EventSwitch::raiseEvent( new RouteEvent(frag2, ROUTE_PROCESS_BUNDLE) );

				return TRANSMIT_SUCCESSFUL;
			}

			stringstream ss;
			ss << b;

			string data = ss.str();

			struct sockaddr_in clientAddress;

			// destination parameter
			clientAddress.sin_family = AF_INET;

			// broadcast
			_net.getInterfaceAddress(&(clientAddress.sin_addr));
			clientAddress.sin_port = htons(_net.getPort());

			ibrcommon::MutexLock l(m_writelock);

		    // send converted line back to client.
		    int ret = sendto(m_socket, data.c_str(), data.length(), 0, (struct sockaddr *) &clientAddress, sizeof(clientAddress));

		    if (ret < 0)
		    {
		    	return CONVERGENCE_LAYER_BUSY;
		    }

		    return TRANSMIT_SUCCESSFUL;
		}

		TransmitReport UDPConvergenceLayer::transmit(const Bundle &b, const Node &node)
		{
			unsigned int size = b.getSize();

			if (size > m_maxmsgsize)
			{
				// get the payload block
				PayloadBlock *payload = utils::Utils::getPayloadBlock( b );
				size_t application_length = payload->getBLOB().getSize();

				// the new size for the payload
				size_t payload_size = m_maxmsgsize;

				// reduce by the overhead
				payload_size -= (size - application_length);

				// possible size of fragment offset and application data length
				payload_size -= 20;

				// split the payload block
				pair<PayloadBlock*, PayloadBlock*> frags = payload->split(payload_size);

				Bundle frag1 = b;
				frag1.clearBlocks();
				frag1.addBlock(frags.first);

				Bundle frag2 = b;
				frag2.clearBlocks();
				frag2.addBlock(frags.second);
				frag2._fragmentoffset += payload_size;

				if (!(b._procflags & Bundle::FRAGMENT))
				{
					frag1._procflags += Bundle::FRAGMENT;
					frag1._appdatalength += application_length;
					frag2._procflags += Bundle::FRAGMENT;
					frag2._appdatalength += application_length;
				}

				// transfer the fragment
				transmit(frag1, node);

				// return the fragment to the router
				EventSwitch::raiseEvent( new RouteEvent(frag2, ROUTE_PROCESS_BUNDLE) );

				return TRANSMIT_SUCCESSFUL;
			}

			stringstream ss;
			ss << b;
			string data = ss.str();

			struct sockaddr_in clientAddress;

			// destination parameter
			clientAddress.sin_family = AF_INET;

			// use node to define the destination
			clientAddress.sin_addr.s_addr = inet_addr(node.getAddress().c_str());
			clientAddress.sin_port = htons(node.getPort());

			ibrcommon::MutexLock l(m_writelock);

		    // Send converted line back to client.
		    int ret = sendto(m_socket, data.c_str(), data.length(), 0, (struct sockaddr *) &clientAddress, sizeof(clientAddress));

		    if (ret < 0)
		    {
		    	return CONVERGENCE_LAYER_BUSY;
		    }

		    return TRANSMIT_SUCCESSFUL;
		}

		void UDPConvergenceLayer::receive(dtn::data::Bundle &bundle)
		{
			ibrcommon::MutexLock l(m_readlock);

			struct sockaddr_in clientAddress;
			socklen_t clientAddressLength = sizeof(clientAddress);

			char data[m_maxmsgsize];

			// first peek if data is incoming, wait max. 100ms
			struct pollfd psock[1];

			psock[0].fd = m_socket;
			psock[0].events = POLLIN;
			psock[0].revents = POLLERR;

			// data waiting
			int len = recvfrom(m_socket, data, m_maxmsgsize, MSG_WAITALL, (struct sockaddr *) &clientAddress, &clientAddressLength);

			if (len < 0)
			{
				// no data received
				return;
			}

			// read all data into a stream
			stringstream ss;
			ss.write(data, len);

			// get the bundle
			ss >> bundle;
		}

		BundleConnection* UDPConvergenceLayer::getConnection(const dtn::core::Node &n)
		{
			if (n.getProtocol() != UDP_CONNECTION) return NULL;

			return new dtn::net::UDPConnection(*this, n);
		}

		void UDPConvergenceLayer::run()
		{
			while (_running)
			{
				try {
					dtn::data::Bundle bundle;
					receive(bundle);

					// raise default bundle received event
					dtn::core::EventSwitch::raiseEvent( new dtn::core::BundleEvent(bundle, dtn::core::BUNDLE_RECEIVED) );
					dtn::core::EventSwitch::raiseEvent( new dtn::core::RouteEvent(bundle, dtn::core::ROUTE_PROCESS_BUNDLE) );
				} catch (dtn::exceptions::InvalidBundleData ex) {
					cerr << "Received a invalid bundle: " << ex.what() << endl;
				} catch (dtn::exceptions::IOException ex) {

				}
				yield();
			}
		}
	}
}

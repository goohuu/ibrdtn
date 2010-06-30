#include "net/UDPConvergenceLayer.h"
#include "net/BundleReceivedEvent.h"
#include "core/BundleEvent.h"
#include "net/TransferCompletedEvent.h"
#include "routing/RequeueBundleEvent.h"
#include <ibrcommon/net/UnicastSocket.h>
#include <ibrcommon/net/BroadcastSocket.h>
#include "core/BundleCore.h"

#include <ibrcommon/data/BLOB.h>
#include <ibrcommon/Logger.h>
#include <ibrcommon/thread/MutexLock.h>

#include <ibrdtn/utils/Utils.h>
#include <ibrdtn/data/Serializer.h>

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

		UDPConvergenceLayer::UDPConvergenceLayer(ibrcommon::NetInterface net, int port, bool broadcast, unsigned int mtu)
			: _socket(NULL), _net(net), _port(port), m_maxmsgsize(mtu), _running(false)
		{
			if (broadcast)
			{
				_socket = new ibrcommon::BroadcastSocket();
			}
			else
			{
				_socket = new ibrcommon::UnicastSocket();
			}
		}

		UDPConvergenceLayer::~UDPConvergenceLayer()
		{
			if (isRunning())
			{
				componentDown();
			}

			delete _socket;
		}

		dtn::core::NodeProtocol UDPConvergenceLayer::getDiscoveryProtocol() const
		{
			return dtn::core::UDP_CONNECTION;
		}

		void UDPConvergenceLayer::update(std::string &name, std::string &params)
		{
			name = "udpcl";

			stringstream service; service << "ip=" << _net.getAddress() << ";port=" << _port << ";";
			params = service.str();
		}

		bool UDPConvergenceLayer::onInterface(const ibrcommon::NetInterface &net) const
		{
			if (_net.getInterface() == net.getInterface()) return true;
			return false;
		}

		void UDPConvergenceLayer::queue(const dtn::core::Node &node, const ConvergenceLayer::Job &job)
		{
			std::stringstream ss;
			dtn::data::DefaultSerializer serializer(ss);

			unsigned int size = serializer.getLength(job._bundle);

			if (size > m_maxmsgsize)
			{
				// TODO: create a fragment of length "size"

//				// get the payload block
//				PayloadBlock *payload = utils::Utils::getPayloadBlock( b );
//				size_t application_length = payload->getBLOB().getSize();
//
//				// the new size for the payload
//				size_t payload_size = m_maxmsgsize;
//
//				// reduce by the overhead
//				payload_size -= (size - application_length);
//
//				// possible size of fragment offset and application data length
//				payload_size -= 20;
//
//				// split the payload block
//				pair<PayloadBlock*, PayloadBlock*> frags = payload->split(payload_size);
//
//				Bundle frag1 = b;
//				frag1.clearBlocks();
//				frag1.addBlock(frags.first);
//
//				Bundle frag2 = b;
//				frag2.clearBlocks();
//				frag2.addBlock(frags.second);
//				frag2._fragmentoffset += payload_size;
//
//				if (!(b._procflags & Bundle::FRAGMENT))
//				{
//					frag1._procflags += Bundle::FRAGMENT;
//					frag1._appdatalength += application_length;
//					frag2._procflags += Bundle::FRAGMENT;
//					frag2._appdatalength += application_length;
//				}

				// TODO: transfer the fragment

				throw ConnectionInterruptedException();
			}

			serializer << job._bundle;
			string data = ss.str();

			// get a udp peer
			ibrcommon::udpsocket::peer p = _socket->getPeer(node.getAddress(), node.getPort());

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
			dtn::net::TransferCompletedEvent::raise(job._destination, job._bundle);
			dtn::core::BundleEvent::raise(job._bundle, dtn::core::BUNDLE_FORWARDED);
		}

		UDPConvergenceLayer& UDPConvergenceLayer::operator>>(dtn::data::Bundle &bundle)
		{
			ibrcommon::MutexLock l(m_readlock);

			char data[m_maxmsgsize];

			// data waiting
			int len = _socket->receive(data, m_maxmsgsize);

			if (len > 0)
			{
				// read all data into a stream
				stringstream ss;
				ss.write(data, len);

				// get the bundle
				dtn::data::DefaultDeserializer(ss, dtn::core::BundleCore::getInstance()) >> bundle;
			}

			return (*this);
		}

		void UDPConvergenceLayer::componentUp()
		{
			try {
				try {
					ibrcommon::UnicastSocket &sock = dynamic_cast<ibrcommon::UnicastSocket&>(*_socket);
					sock.bind(_port, _net);
				} catch (std::bad_cast) {

				}

				try {
					ibrcommon::BroadcastSocket &sock = dynamic_cast<ibrcommon::BroadcastSocket&>(*_socket);
					sock.bind(_port, _net);
				} catch (std::bad_cast) {

				}

			} catch (ibrcommon::udpsocket::SocketException ex) {
				IBRCOMMON_LOGGER(error) << "Failed to add UDP ConvergenceLayer on " << _net.getAddress() << ":" << _port << IBRCOMMON_LOGGER_ENDL;
				IBRCOMMON_LOGGER(error) << "      Error: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
			}
		}

		void UDPConvergenceLayer::componentDown()
		{
			_running = false;
			_socket->shutdown();
			join();
		}

		void UDPConvergenceLayer::componentRun()
		{
			_running = true;

			while (_running)
			{
				try {
					dtn::data::Bundle bundle;
					(*this) >> bundle;

					// determine sender
					EID sender;

					// raise default bundle received event
					dtn::net::BundleReceivedEvent::raise(sender, bundle);

				} catch (dtn::InvalidDataException ex) {
					IBRCOMMON_LOGGER(warning) << "Received a invalid bundle: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
				} catch (ibrcommon::IOException ex) {

				}
				yield();
			}
		}
	}
}

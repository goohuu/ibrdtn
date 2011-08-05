#include "net/UDPConvergenceLayer.h"
#include "net/BundleReceivedEvent.h"
#include "net/TransferCompletedEvent.h"
#include "net/TransferAbortedEvent.h"
#include "core/BundleEvent.h"
#include "core/BundleCore.h"
#include "routing/RequeueBundleEvent.h"

#include <ibrdtn/utils/Utils.h>
#include <ibrdtn/data/Serializer.h>
#include <ibrdtn/data/ScopeControlHopLimitBlock.h>

#include <ibrcommon/net/UnicastSocket.h>
#include <ibrcommon/net/vaddress.h>
#include <ibrcommon/net/vinterface.h>
#include <ibrcommon/data/BLOB.h>
#include <ibrcommon/Logger.h>
#include <ibrcommon/thread/MutexLock.h>

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
#include <list>


using namespace dtn::data;

namespace dtn
{
	namespace net
	{
		const int UDPConvergenceLayer::DEFAULT_PORT = 4556;

		UDPConvergenceLayer::UDPConvergenceLayer(ibrcommon::vinterface net, int port, unsigned int mtu)
			: _socket(NULL), _net(net), _port(port), m_maxmsgsize(mtu), _running(false)
		{
			_socket = new ibrcommon::UnicastSocket();
		}

		UDPConvergenceLayer::~UDPConvergenceLayer()
		{
			componentDown();
			delete _socket;
		}

		dtn::core::Node::Protocol UDPConvergenceLayer::getDiscoveryProtocol() const
		{
			return dtn::core::Node::CONN_UDPIP;
		}

		void UDPConvergenceLayer::update(const ibrcommon::vinterface &iface, std::string &name, std::string &params) throw (dtn::net::DiscoveryServiceProvider::NoServiceHereException)
		{
			if (iface == _net)
			{
				name = "udpcl";
				stringstream service;

				try {
					std::list<ibrcommon::vaddress> list = _net.getAddresses(ibrcommon::vaddress::VADDRESS_INET);
					if (!list.empty())
					{
						 service << "ip=" << list.front().get(false) << ";port=" << _port << ";";
					}
					else
					{
						service << "port=" << _port << ";";
					}
				} catch (const ibrcommon::vinterface::interface_not_set&) {
					service << "port=" << _port << ";";
				};

				params = service.str();
			}
			else
			{
				 throw dtn::net::DiscoveryServiceProvider::NoServiceHereException();
			}
		}

		void UDPConvergenceLayer::queue(const dtn::core::Node &node, const ConvergenceLayer::Job &job)
		{
			const std::list<dtn::core::Node::URI> uri_list = node.get(dtn::core::Node::CONN_UDPIP);
			if (uri_list.empty())
			{
				dtn::net::TransferAbortedEvent::raise(node.getEID(), job._bundle, dtn::net::TransferAbortedEvent::REASON_UNDEFINED);
				return;
			}

			std::stringstream ss;
			dtn::data::DefaultSerializer serializer(ss);

			dtn::core::BundleStorage &storage = dtn::core::BundleCore::getInstance().getStorage();

			try {
				// read the bundle out of the storage
				const dtn::data::Bundle bundle = storage.get(job._bundle);

				unsigned int size = serializer.getLength(bundle);

				if (size > m_maxmsgsize)
				{
					// TODO: create a fragment of length "size"
					throw ConnectionInterruptedException();
				}

				serializer << bundle;
				string data = ss.str();

				const dtn::core::Node::URI &uri = uri_list.front();

				std::string address = "0.0.0.0";
				unsigned int port = 0;

				// read values
				uri.decode(address, port);

				// get the address of the node
				ibrcommon::vaddress addr(address);

				// set write lock
				ibrcommon::MutexLock l(m_writelock);

				// send converted line back to client.
				int ret = _socket->send(addr, port, data.c_str(), data.length());

				if (ret == -1)
				{
					// CL is busy, requeue bundle
					dtn::routing::RequeueBundleEvent::raise(job._destination, job._bundle);

					return;
				}

				// raise bundle event
				dtn::net::TransferCompletedEvent::raise(job._destination, bundle);
				dtn::core::BundleEvent::raise(bundle, dtn::core::BUNDLE_FORWARDED);
			} catch (const dtn::core::BundleStorage::NoBundleFoundException&) {
				// send transfer aborted event
				dtn::net::TransferAbortedEvent::raise(node.getEID(), job._bundle, dtn::net::TransferAbortedEvent::REASON_BUNDLE_DELETED);
			}

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
				} catch (const std::bad_cast&) {

				}
			} catch (const ibrcommon::udpsocket::SocketException &ex) {
				IBRCOMMON_LOGGER(error) << "Failed to add UDP ConvergenceLayer on " << _net.toString() << ":" << _port << IBRCOMMON_LOGGER_ENDL;
				IBRCOMMON_LOGGER(error) << "      Error: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
			}
		}

		void UDPConvergenceLayer::componentDown()
		{
			_running = false;
			_socket->shutdown();
			stop();
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

					// increment value in the scope control hop limit block
					try {
						dtn::data::ScopeControlHopLimitBlock &schl = bundle.getBlock<dtn::data::ScopeControlHopLimitBlock>();
						schl.increment();
					} catch (const dtn::data::Bundle::NoSuchBlockFoundException&) { };

					// raise default bundle received event
					dtn::net::BundleReceivedEvent::raise(sender, bundle);

				} catch (const dtn::InvalidDataException &ex) {
					IBRCOMMON_LOGGER(warning) << "Received a invalid bundle: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
				} catch (const ibrcommon::IOException &ex) {

				}
				yield();
			}
		}

		bool UDPConvergenceLayer::__cancellation()
		{
			// since this is an receiving thread we have to cancel the hard way
			return false;
		}

		const std::string UDPConvergenceLayer::getName() const
		{
			return "UDPConvergenceLayer";
		}
	}
}

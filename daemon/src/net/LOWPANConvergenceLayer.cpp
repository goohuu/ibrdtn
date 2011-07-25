#include "net/LOWPANConvergenceLayer.h"
#include "net/BundleReceivedEvent.h"
#include "core/BundleEvent.h"
#include "net/TransferCompletedEvent.h"
#include "net/TransferAbortedEvent.h"
#include "routing/RequeueBundleEvent.h"
#include <ibrcommon/net/UnicastSocketLowpan.h>
#include <ibrcommon/net/vaddress.h>
#include <ibrcommon/net/vinterface.h>
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
#include <list>


using namespace dtn::data;

namespace dtn
{
	namespace net
	{
		const int LOWPANConvergenceLayer::DEFAULT_PANID = 0x780;

		LOWPANConvergenceLayer::LOWPANConvergenceLayer(ibrcommon::vinterface net, int panid, bool, unsigned int mtu)
			: _socket(NULL), _net(net), _panid(panid), m_maxmsgsize(mtu), _running(false)
		{
			_socket = new ibrcommon::UnicastSocketLowpan();
		}

		LOWPANConvergenceLayer::~LOWPANConvergenceLayer()
		{
			componentDown();
			delete _socket;
		}

		dtn::core::Node::Protocol LOWPANConvergenceLayer::getDiscoveryProtocol() const
		{
			return dtn::core::Node::CONN_LOWPAN;
		}

		void LOWPANConvergenceLayer::update(const ibrcommon::vinterface &iface, std::string &name, std::string &params) throw(dtn::net::DiscoveryServiceProvider::NoServiceHereException)
		{
			if (iface == _net) throw dtn::net::DiscoveryServiceProvider::NoServiceHereException();

			name = "lowpancl";
			stringstream service;

			try {
				std::list<ibrcommon::vaddress> list = _net.getAddresses();
				if (!list.empty())
				{
					 service << "short address=" << list.front().get(false) << ";panid=" << _panid << ";";
				}
				else
				{
					service << "panid=" << _panid << ";";
				}
			} catch (const ibrcommon::vinterface::interface_not_set&) {
				service << "panid=" << _panid << ";";
			};

			params = service.str();
		}

		void LOWPANConvergenceLayer::queue(const dtn::core::Node &node, const ConvergenceLayer::Job &job)
		{
			const std::list<dtn::core::Node::URI> uri_list = node.get(dtn::core::Node::CONN_LOWPAN);
			if (uri_list.empty()) return;

			std::stringstream ss;
			dtn::data::DefaultSerializer serializer(ss);

			dtn::core::BundleStorage &storage = dtn::core::BundleCore::getInstance().getStorage();

			try {
				// read the bundle out of the storage
				const dtn::data::Bundle bundle = storage.get(job._bundle);

				unsigned int size = serializer.getLength(bundle);

				if (size > m_maxmsgsize)
				{
					throw ConnectionInterruptedException();
				}

				const dtn::core::Node::URI &uri = uri_list.front();

				std::string address = "0";
				unsigned int pan = 0x00;
				int length;

				// read values
				uri.decode(address, pan);

				cout << "CL LOWPAN was asked to connect to " << hex << address << " in PAN " << hex << pan << endl;

				serializer << bundle;
				string data = ss.str();

				// get a lowpan peer
				ibrcommon::lowpansocket::peer p = _socket->getPeer(address, pan);

				cout << "Send out bundle with length " << dec << data.length() << endl;

				if (data.length() > 116) {
					string chunk;
					int i;
					int chunks = ceil(data.length() / 116.0);
					cout << "Bundle to big to fit into one packet. Need to split into " << dec << chunks << " segments" << endl;
					for (i = 0; i < data.length(); i += 116) {
						chunk = data.substr(i, 116);
						cout << "Chunk with offset " << dec << i << " : " << chunk << endl;
						// set write lock
						ibrcommon::MutexLock l(m_writelock);

						// send converted line back to client.
						int ret = p.send(chunk.c_str(), chunk.length());

						if (ret == -1)
						{
							// CL is busy, requeue bundle
							dtn::routing::RequeueBundleEvent::raise(job._destination, job._bundle);

							return;
						}
					}
					// raise bundle event
					dtn::net::TransferCompletedEvent::raise(job._destination, bundle);
					dtn::core::BundleEvent::raise(bundle, dtn::core::BUNDLE_FORWARDED);
				} else {

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
					dtn::net::TransferCompletedEvent::raise(job._destination, bundle);
					dtn::core::BundleEvent::raise(bundle, dtn::core::BUNDLE_FORWARDED);
				}
			} catch (const dtn::core::BundleStorage::NoBundleFoundException&) {
				// send transfer aborted event
				dtn::net::TransferAbortedEvent::raise(EID(node.getEID()), job._bundle, dtn::net::TransferAbortedEvent::REASON_BUNDLE_DELETED);
			}
		}

		LOWPANConvergenceLayer& LOWPANConvergenceLayer::operator>>(dtn::data::Bundle &bundle)
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

		void LOWPANConvergenceLayer::componentUp()
		{
			try {
				try {
					ibrcommon::UnicastSocketLowpan &sock = dynamic_cast<ibrcommon::UnicastSocketLowpan&>(*_socket);
					sock.bind(_panid, _net);
				} catch (const std::bad_cast&) {

				}
			} catch (const ibrcommon::lowpansocket::SocketException &ex) {
				IBRCOMMON_LOGGER(error) << "Failed to add LOWPAN ConvergenceLayer on " << _net.toString() << ":" << _panid << IBRCOMMON_LOGGER_ENDL;
				IBRCOMMON_LOGGER(error) << "      Error: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
			}
		}

		void LOWPANConvergenceLayer::componentDown()
		{
			_running = false;
			_socket->shutdown();
			join();
		}

		void LOWPANConvergenceLayer::componentRun()
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

				} catch (const dtn::InvalidDataException &ex) {
					IBRCOMMON_LOGGER(warning) << "Received a invalid bundle: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
				} catch (const ibrcommon::IOException&) {

				}
				yield();
			}
		}

		bool LOWPANConvergenceLayer::__cancellation()
		{
			// since this is an receiving thread we have to cancel the hard way
			return false;
		}

		const std::string LOWPANConvergenceLayer::getName() const
		{
			return "LOWPANConvergenceLayer";
		}
	}
}

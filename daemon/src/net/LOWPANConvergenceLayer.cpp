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

/* Header:
 * +---------------+
 * |7 6 5 4 3 2 1 0|
 * +---------------+
 * Bit 0-3: sequence number (0-16)
 * Bit 4-5: 00 Middle segment
 *	    01 Last segment
 *	    10 First segment
 *	    11 First and last segment
 * Bit 6-7: 00 to be compatible with 6LoWPAN
 */
#define SEGMENT_FIRST	0x25
#define SEGMENT_LAST	0x10
#define SEGMENT_BOTH	0x30
#define SEGMENT_MIDDLE	0x00

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

				//cout << "CL LOWPAN was asked to connect to " << hex << address << " in PAN " << hex << pan << endl;

				serializer << bundle;
				string data = ss.str();

				// get a lowpan peer
				ibrcommon::lowpansocket::peer p = _socket->getPeer(address, pan);

				//cout << "Send out bundle with length " << dec << data.length() << endl;

				if (data.length() > 114) {
					std::string chunk, tmp;
					char header = 0;
					int i, seq_num;
					int chunks = ceil(data.length() / 114.0);
					cout << "Bundle to big to fit into one packet. Need to split into " << dec << chunks << " segments" << endl;
					for (i = 0; i < chunks; i++) {
						stringstream buf;
						chunk = data.substr(i * 114, 114);

						seq_num =  i % 16;

						printf("Iteration %i with seq number %i, from %i chunks\n", i, seq_num, chunks);
						if (i == 0) // First segment
							header = SEGMENT_FIRST + seq_num;
						else if (i == (chunks - 1)) // Last segment
							header = SEGMENT_LAST + seq_num;
						else
							header = SEGMENT_MIDDLE+ seq_num;

						printf("WTF header: %x\n", header);
						buf << header;
						tmp = buf.str() + chunk; // Prepand header to chunk
						chunk = "";
						chunk = tmp;

						//cout << "Chunk with offset " << dec << i << " : " << chunk << endl;
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

					std::string tmp;
					stringstream buf;

					buf << SEGMENT_BOTH;

					tmp = buf.str() + data; // Prepand header to chunk
					data = "";
					data = tmp;

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
			stringstream ss, ss2;
			std::string buf, header;
//			char foo;

			// data waiting
			int len = _socket->receive(data, m_maxmsgsize);
			printf("Header first segment: %x\n", data[0]);
			printf("Buffer from receive: %x\n", data[1]);

			//cout << "Chunk 1 with length " << dec << len << " : " << data << endl;
			if (len > 0)
			{
				// read all data into a stream
				ss2.write(data, len);
				buf = ss2.str();
				//header = buf.substr(0,1);
				//foo = atoi(header.c_str());
				buf.erase(0,1); // remove header byte
				ss << buf;
			}

			len = _socket->receive(data, m_maxmsgsize);
			printf("Header second segment: %x\n", data[0]);
			printf("Buffer from receive: %x\n", data[1]);

			//cout << "Chunk 2 with length " << dec << len << " : " << data << endl;
			if (len > 0)
			{
				// read all data into a stream
				ss2.str("");
				ss2.write(data, len);
				buf = ss2.str();
				//header = buf.substr(0,1);
				//foo = atoi(header.c_str());
				buf.erase(0,1); // remove header byte
				ss << buf;

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

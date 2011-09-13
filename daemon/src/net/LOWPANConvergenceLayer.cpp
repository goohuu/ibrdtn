#include "net/LOWPANConvergenceLayer.h"
#include "net/LOWPANConnection.h"
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

#include <ibrdtn/data/ScopeControlHopLimitBlock.h>
#include <ibrdtn/utils/Utils.h>
#include <ibrdtn/data/Serializer.h>

#include <sys/socket.h>
#include <poll.h>
#include <errno.h>

#include <sys/types.h>
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
 * Bit 7-6: 00 to be compatible with 6LoWPAN
 * Bit 5-4: 00 Middle segment
 *	    01 Last segment
 *	    10 First segment
 *	    11 First and last segment
 * Bit 3:   0 Extended frame not available
 *          1 Extended frame available
 * Bit 2-0: sequence number (0-7)
 *
 * Extended header (only if extended frame available)
 * +---------------+
 * |7 6 5 4 3 2 1 0|
 * +---------------+
 * Bit 7:   0 No discovery frame
 *          1 Discovery frame
 * Bit 6-0: Reserved
 *
 * Two bytes at the end of the frame are reserved for the short address of the
 * sender. This is a workaround until recvfrom() gets fixed.
 */

#define SEGMENT_FIRST	0x25
#define SEGMENT_LAST	0x10
#define SEGMENT_BOTH	0x30
#define SEGMENT_MIDDLE	0x00
#define EXTENDED_MASK	0x04

using namespace dtn::data;

namespace dtn
{
	namespace net
	{
		const int LOWPANConvergenceLayer::DEFAULT_PANID = 0x780;

		LOWPANConvergenceLayer::LOWPANConvergenceLayer(ibrcommon::vinterface net, int panid, bool broadcast, unsigned int mtu)
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
			if (iface == _net)
			{
				name = "lowpancl";
				stringstream service;

				try {
					std::list<ibrcommon::vaddress> list = _net.getAddresses(ibrcommon::vaddress::VADDRESS_INET);
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
			else
			{
				 throw dtn::net::DiscoveryServiceProvider::NoServiceHereException();
			}
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
				int length, ret;
				std::string tmp;

				// read values
				uri.decode(address, pan);

				serializer << bundle;
				string data = ss.str();

				// get a lowpan peer
				ibrcommon::lowpansocket::peer p = _socket->getPeer(address, pan);

				/* Get frame from connection, add own address at
				 * the end and send it off */

				stringstream buf;
				char header;
				header = SEGMENT_BOTH;

				buf.put(header);
				buf << data;

				// Get address via netlink
				struct sockaddr_ieee802154 _sockaddr;
				_socket->getAddress(&_sockaddr.addr, _net);
				_sockaddr.addr.short_addr = 0x1234; // HACK

				buf.write((char *)&_sockaddr.addr.short_addr, sizeof(_sockaddr.addr.short_addr));
				cout << "Sender address set to " << hex << _sockaddr.addr.short_addr << endl;

				data = buf.str();
				cout << "Sending following data stream: " << data << endl;

#if 0
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

						buf << header;
						tmp = buf.str() + chunk; // Prepand header to chunk
						chunk = "";
						chunk = tmp;

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
#endif

					// set write lock
					ibrcommon::MutexLock l(m_writelock);


					// send converted line back to client.
					ret = p.send(data.c_str(), data.length());

					if (ret == -1)
					{
						// CL is busy, requeue bundle
						dtn::routing::RequeueBundleEvent::raise(job._destination, job._bundle);

						return;
					}

					// raise bundle event
					dtn::net::TransferCompletedEvent::raise(job._destination, bundle);
					dtn::core::BundleEvent::raise(bundle, dtn::core::BUNDLE_FORWARDED);
//				}

			} catch (const dtn::core::BundleStorage::NoBundleFoundException&) {
				// send transfer aborted event
				dtn::net::TransferAbortedEvent::raise(EID(node.getEID()), job._bundle, dtn::net::TransferAbortedEvent::REASON_BUNDLE_DELETED);
			}
		}

		LOWPANConvergenceLayer& LOWPANConvergenceLayer::operator>>(dtn::data::Bundle &bundle)
		{
			ibrcommon::MutexLock l(m_readlock);

			char data[m_maxmsgsize];
			char header, extended_header;
			short address;
			stringstream ss, tmp;
			string payload;

			/* This worker needs to take care of all incoming frames
			 * and puts them into the right channels
			 */

			// Receive full frame from socket
			int len = _socket->receive(data, m_maxmsgsize);

			if (len <= 0)
				return (*this);

			// Retrieve header of frame
			header = data[0];

			// Check for extended header and retrieve
			if ((header & EXTENDED_MASK) == 0x04)
				extended_header = data[1];

			// Retrieve sender address from the end of the frame
			address = data[len-1] | (data[len-2] << 8);
			cout << "Received address " << hex << address << endl;

			// Put the data frame in the queue corresponding to its sender address
			/* FIXME here we need a list with the connection objects
			 * and the queues we can write into */
#if 0
			LOWPANConnection *connection = new LOWPANConnection(address, LOWPANConvergenceLayer &cl);

			std::list<LOWPANConnection*>::iterator i;
			for(i = ConnectionList.begin(); i != ConnectionList.end(); ++i)
				cout << *i << " ";
			cout << endl;
#endif
			cout << "Received frame: " << data << endl;
			ss.write(data+1, len-3); // remove header and address "footer"
			//tmp.write(data+1, len-1);
			//payload = tmp.str();
			//address = payload.substr (payload.length()-4,4);
			//payload.erase(payload.length()-4,4);

//			ss << payload;

			// Send off discovery frame
			if (extended_header == 0x80)
				cout << "Received beacon frame for LoWPAN discovery" << endl;

#if 0
			while (header != SEGMENT_LAST) {

				len = _socket->receive(data, m_maxmsgsize);
				header = data[0];
				//header &= 0xF0;

				if (len > 0)
					ss.write(data+1, len-1);
			}
#endif

			if (len > 0)
				dtn::data::DefaultDeserializer(ss, dtn::core::BundleCore::getInstance()) >> bundle;
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
			// stop(); needed here?
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

					// increment value in the scope control hop limit block
					try {
						dtn::data::ScopeControlHopLimitBlock &schl = bundle.getBlock<dtn::data::ScopeControlHopLimitBlock>();
						schl.increment();
					} catch (const dtn::data::Bundle::NoSuchBlockFoundException&) { };

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

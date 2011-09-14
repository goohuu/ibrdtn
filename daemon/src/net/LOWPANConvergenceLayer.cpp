#include "net/LOWPANConvergenceLayer.h"
#include "net/LOWPANConnection.h"
#include "net/lowpanstream.h"
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

				std::string address;
				unsigned int pan;
				int ret;
				stringstream buf;
				char header;

				// read values
				uri.decode(address, pan);

				serializer << bundle;
				string data = ss.str();

				// get a lowpan peer
				ibrcommon::lowpansocket::peer p = _socket->getPeer(address, pan);

				/* Get frame from connection, add own address at
				 * the end and send it off */
				header = SEGMENT_BOTH;
				buf.put(header);
				buf << data;

				// Get address via netlink
				struct sockaddr_ieee802154 _sockaddr;
				_socket->getAddress(&_sockaddr.addr, _net);

				buf.write((char *)&_sockaddr.addr.short_addr, sizeof(_sockaddr.addr.short_addr));
				data = buf.str();

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
			unsigned short address;
			stringstream ss;

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
			address = (data[len-1] << 8) | data[len-2];

			// Received discovery frame?
			if (extended_header == 0x80)
				cout << "Received beacon frame for LoWPAN discovery" << endl;

			/* Decide in which queue to write based on the src address */
			/* Get matching connection and queue the data */
			LOWPANConnection *connection = getConnection(address);
			//connection->getStream().queue(data+1, len-3);
			ss.write(data+1, len-3); // remove header and address "footer"

			if (len > 0)
				dtn::data::DefaultDeserializer(ss, dtn::core::BundleCore::getInstance()) >> bundle;
			return (*this);
		}

		LOWPANConnection* LOWPANConvergenceLayer::getConnection(unsigned short address)
		{
			//LOWPANConvergenceLayer cl;
			// Test if connection for this address already exist
			std::list<LOWPANConnection*>::iterator i;
			for(i = ConnectionList.begin(); i != ConnectionList.end(); ++i)
			{
				cout << "Searched address: " << address << endl;
				cout << "Connection address: " << (*i)->address << endl;
				if ((*i)->address == address)
					return (*i);
			}

			// Connection does not exist, create one and put it into the list
			//LOWPANConnection *connection = new LOWPANConnection(address, cl);
			LOWPANConnection *connection = new LOWPANConnection(address);

			ConnectionList.push_back(connection);
			return connection;
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

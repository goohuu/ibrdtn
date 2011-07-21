#include "net/LOWPANConvergenceLayer.h"
#include "net/LOWPANConnection.h"
#include "net/lowpanstream.h"
#include <ibrcommon/net/UnicastSocketLowpan.h>

#include <ibrcommon/Logger.h>
#include <ibrcommon/thread/MutexLock.h>

#include <ibrdtn/data/ScopeControlHopLimitBlock.h>

#include <sys/socket.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <list>

#define EXTENDED_MASK	0x04

using namespace dtn::data;

namespace dtn
{
	namespace net
	{
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

		void LOWPANConvergenceLayer::send_cb(char *buf, int len, unsigned int address)
		{
			// Add own address at the end
			struct sockaddr_ieee802154 _sockaddr;
			_socket->getAddress(&_sockaddr.addr, _net);

			// get a lowpan peer
			ibrcommon::lowpansocket::peer p = _socket->getPeer(address, _sockaddr.addr.pan_id);

			// Add own address at the end
			memcpy(&buf[len], &_sockaddr.addr.short_addr, sizeof(_sockaddr.addr.short_addr));

			// set write lock
			ibrcommon::MutexLock l(m_writelock);

			// send converted line
			int ret = p.send(buf, len + 2);

			if (ret == -1)
			{
				// CL is busy
				throw(ibrcommon::Exception("Send on socket failed"));
			}
		}

		void LOWPANConvergenceLayer::queue(const dtn::core::Node &node, const ConvergenceLayer::Job &job)
		{
			const std::list<dtn::core::Node::URI> uri_list = node.get(dtn::core::Node::CONN_LOWPAN);
			if (uri_list.empty()) return;

			const dtn::core::Node::URI &uri = uri_list.front();

			std::string address;
			unsigned int pan;

			// read values
			uri.decode(address, pan);

			IBRCOMMON_LOGGER_DEBUG(10) << "LOWPANConvergenceLayer::queue"<< IBRCOMMON_LOGGER_ENDL;

			getConnection(atoi(address.c_str()))->_sender.queue(job);
		}

		LOWPANConnection* LOWPANConvergenceLayer::getConnection(unsigned short address)
		{
			// Test if connection for this address already exist
			std::list<LOWPANConnection*>::iterator i;
			for(i = ConnectionList.begin(); i != ConnectionList.end(); ++i)
			{
				IBRCOMMON_LOGGER_DEBUG(10) << "Connection address: " << hex << (*i)->_address << IBRCOMMON_LOGGER_ENDL;
				if ((*i)->_address == address)
					return (*i);
			}

			// Connection does not exist, create one and put it into the list
			LOWPANConnection *connection = new LOWPANConnection(address, (*this));

			ConnectionList.push_back(connection);
			IBRCOMMON_LOGGER_DEBUG(10) << "LOWPANConvergenceLayer::getConnection "<< connection->_address << IBRCOMMON_LOGGER_ENDL;
			connection->start();
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
				IBRCOMMON_LOGGER_DEBUG(10) << "Failed to add LOWPAN ConvergenceLayer on " << _net.toString() << ":" << _panid << IBRCOMMON_LOGGER_ENDL;
				IBRCOMMON_LOGGER_DEBUG(10) << "Exception: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
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
				ibrcommon::MutexLock l(m_readlock);

				char data[m_maxmsgsize];
				char header, extended_header;
				unsigned int address;

				// Receive full frame from socket
				int len = _socket->receive(data, m_maxmsgsize);

				IBRCOMMON_LOGGER_DEBUG(10) << ":LOWPANConvergenceLayer::componentRun" << IBRCOMMON_LOGGER_ENDL;

				// We got nothing from the socket, keep reading
				if (len <= 0)
					continue;

				// Retrieve header of frame
				header = data[0];

				// Retrieve sender address from the end of the frame
				address = (data[len-1] << 8) | data[len-2];

				// Check for extended header and retrieve if available
				if (header & EXTENDED_MASK)
					extended_header = data[1];

				// Received discovery frame?
				if (extended_header == 0x80)
					IBRCOMMON_LOGGER_DEBUG(10) << "Received beacon frame for LoWPAN discovery" << IBRCOMMON_LOGGER_ENDL;

				// Decide in which queue to write based on the src address
				getConnection(address)->getStream().queue(data+1, len-3); // Cut off address from end

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

/*
 * UDPDatagramService.cpp
 *
 *  Created on: 22.11.2011
 *      Author: morgenro
 */

#include "net/UDPDatagramService.h"
#include <ibrdtn/utils/Utils.h>
#include <ibrcommon/Logger.h>
#include <vector>

namespace dtn
{
	namespace net
	{
		UDPDatagramService::UDPDatagramService(const ibrcommon::vinterface &iface, int port, size_t mtu)
		 : _iface(iface), _bind_port(port), _mtu(mtu)
		{
		}

		UDPDatagramService::~UDPDatagramService()
		{
		}

		/**
		 * Bind to the local socket.
		 * @throw If the bind fails, an DatagramException is thrown.
		 */
		void UDPDatagramService::bind() throw (DatagramException)
		{
			try {
				// bind socket to interface
				_socket.bind(_iface, _bind_port, SOCK_DGRAM);
			} catch (const ibrcommon::Exception&) {
				throw DatagramException("bind failed");
			}
		}

		/**
		 * Shutdown the socket. Unblock all calls on the socket (recv, send, etc.)
		 */
		void UDPDatagramService::shutdown()
		{
			// abort socket operations
			udpsocket::shutdown();
		}

		/**
		 * Send the payload as datagram to a defined destination
		 * @param address The destination address encoded as string.
		 * @param buf The buffer to send.
		 * @param length The number of available bytes in the buffer.
		 */
		void UDPDatagramService::send(const std::string &identifier, const char *buf, size_t length) throw (DatagramException)
		{
			try {
				unsigned int port = 0;
				std::string address;

				// decode address
				UDPDatagramService::decode(identifier, address, port);

				IBRCOMMON_LOGGER_DEBUG(20) << "UDPDatagramService::send() to [" << address << "]:" << port << IBRCOMMON_LOGGER_ENDL;

				// create vaddress
				const ibrcommon::vaddress destination(address);
				udpsocket::send(destination, port, buf, length);
			} catch (const ibrcommon::Exception&) {
				throw DatagramException("send failed");
			}
		}

		/**
		 * Send the payload as datagram to all neighbors (broadcast)
		 * @param buf The buffer to send.
		 * @param length The number of available bytes in the buffer.
		 */
		void UDPDatagramService::send(const char *buf, size_t length) throw (DatagramException)
		{
			try {
				// create broadcast address
				const ibrcommon::vaddress broadcast(ibrcommon::vaddress::VADDRESS_INET6, "ff02::1", true);
				udpsocket::send(broadcast, BROADCAST_PORT, buf, length);
			} catch (const ibrcommon::Exception&) {
				throw DatagramException("send failed");
			}
		}

		/**
		 * Receive an incoming datagram.
		 * @param buf A buffer to catch the incoming data.
		 * @param length The length of the buffer.
		 * @param address A buffer for the address of the sender.
		 * @throw If the receive call failed for any reason, an DatagramException is thrown.
		 * @return The number of received bytes.
		 */
		size_t UDPDatagramService::recvfrom(char *buf, size_t length, std::string &address) throw (DatagramException)
		{
			try {
				std::string from;
				unsigned int port;
				size_t ret = receive(buf, length, from, port);

				address = UDPDatagramService::encode(from, port);

				return ret;
			} catch (const ibrcommon::Exception&) {
				throw DatagramException("receive failed");
			}
		}

		/**
		 * Get the maximum message size (MTU) for datagrams of this service.
		 * @return The maximum message size as bytes.
		 */
		size_t UDPDatagramService::getMaxMessageSize() const
		{
			return _mtu;
		}

		/**
		 * Get the tag for this service used in discovery messages.
		 * @return The tag as string.
		 */
		const std::string UDPDatagramService::getServiceTag() const
		{
			return "dgram:udp";
		}

		/**
		 * Get the service description for this convergence layer. This
		 * data is used to contact this node.
		 * @return The service description as string.
		 */
		const std::string UDPDatagramService::getServiceDescription() const
		{
			std::string address;

			// first try IPv6 addresses
			std::list<ibrcommon::vaddress> addrs = _iface.getAddresses(ibrcommon::vaddress::VADDRESS_INET6);

			// fallback to IPv4 if IPv6 is not available
			if (addrs.size() == 0)
			{
				addrs = _iface.getAddresses(ibrcommon::vaddress::VADDRESS_INET);
			}

			// no addresses available, return empty string
			if (addrs.size() == 0) return "";

			const ibrcommon::vaddress &addr = addrs.front();

			return UDPDatagramService::encode(addr.toString(), _bind_port);
		}

		/**
		 * The used interface as vinterface object.
		 * @return A vinterface object.
		 */
		const ibrcommon::vinterface& UDPDatagramService::getInterface() const
		{
			return _iface;
		}

		/**
		 * The protocol identifier for this type of service.
		 * @return
		 */
		dtn::core::Node::Protocol UDPDatagramService::getProtocol() const
		{
			return dtn::core::Node::CONN_DGRAM_UDP;
		}

		const std::string UDPDatagramService::encode(const std::string &address, const unsigned int &port)
		{
			std::stringstream ss;
			ss << "ip=" << address << ";port=" << port << ";";
			return ss.str();
		}

		void UDPDatagramService::decode(const std::string &identifier, std::string &address, unsigned int &port)
		{
			// parse parameters
			std::vector<string> parameters = dtn::utils::Utils::tokenize(";", identifier);
			std::vector<string>::const_iterator param_iter = parameters.begin();

			while (param_iter != parameters.end())
			{
				std::vector<string> p = dtn::utils::Utils::tokenize("=", (*param_iter));

				if (p[0].compare("ip") == 0)
				{
					address = p[1];
				}

				if (p[0].compare("port") == 0)
				{
					std::stringstream port_stream;
					port_stream << p[1];
					port_stream >> port;
				}

				param_iter++;
			}
		}
	} /* namespace net */
} /* namespace dtn */

/*
 * IPNDAgent.cpp
 *
 *  Created on: 14.09.2009
 *      Author: morgenro
 */

#include "net/IPNDAgent.h"
#include "core/BundleCore.h"
#include <ibrdtn/data/Exceptions.h>
#include <sstream>
#include <string.h>
#include <ibrcommon/Logger.h>
#include <ibrcommon/net/MulticastSocket.h>
#include <ibrcommon/net/BroadcastSocket.h>
#include "Configuration.h"
#include <typeinfo>

namespace dtn
{
	namespace net
	{
		IPNDAgent::IPNDAgent(int port, std::string address)
		 : DiscoveryAgent(dtn::daemon::Configuration::getInstance().getDiscovery()), _version(DiscoveryAnnouncement::DISCO_VERSION_01), _socket(NULL), _destination(ibrcommon::vaddress::VADDRESS_INET, address), _port(port)
		{
			if (_destination.isMulticast())
			{
				IBRCOMMON_LOGGER(info) << "DiscoveryAgent: multicast mode " << address << ":" << port << IBRCOMMON_LOGGER_ENDL;
				_socket = new ibrcommon::MulticastSocket();
			}
			else
			{
				IBRCOMMON_LOGGER(info) << "DiscoveryAgent: broadcast mode " << address << ":" << port << IBRCOMMON_LOGGER_ENDL;
				_socket = new ibrcommon::BroadcastSocket();
			}

			switch (_config.version())
			{
			case 2:
				_version = DiscoveryAnnouncement::DISCO_VERSION_01;
				break;

			case 1:
				_version = DiscoveryAnnouncement::DISCO_VERSION_00;
				break;

			case 0:
				IBRCOMMON_LOGGER(info) << "DiscoveryAgent: DTN2 compatibility mode" << IBRCOMMON_LOGGER_ENDL;
				_version = DiscoveryAnnouncement::DTND_IPDISCOVERY;
				break;
			};
		}

		IPNDAgent::~IPNDAgent()
		{
			delete _socket;
		}

		void IPNDAgent::bind(const ibrcommon::vinterface &net)
		{
			IBRCOMMON_LOGGER(info) << "DiscoveryAgent: bind to interface " << net.toString() << IBRCOMMON_LOGGER_ENDL;
			_interfaces.push_back(net);
		}

		void IPNDAgent::send(ibrcommon::udpsocket::peer &p, const DiscoveryAnnouncement &announcement)
		{
			stringstream ss;
			ss << announcement;

			string data = ss.str();
			p.send(data.c_str(), data.length());
		}

		void IPNDAgent::sendAnnoucement(const u_int16_t &sn, const std::list<DiscoveryService> &services)
		{
			DiscoveryAnnouncement announcement(_version, dtn::core::BundleCore::local);
			ibrcommon::MulticastSocket &sock = dynamic_cast<ibrcommon::MulticastSocket&>(*_socket);

			// set sequencenumber
			announcement.setSequencenumber(sn);

			if (!_config.shortbeacon())
			{
				// add services
				for (std::list<DiscoveryService>::const_iterator iter = services.begin(); iter != services.end(); iter++)
				{
					const DiscoveryService &service = (*iter);
					announcement.addService(service);
				}
			}
			
			ibrcommon::udpsocket::peer p = _socket->getPeer(_destination.get(), _port);
			
			// send announcement
			send(p, announcement);
		}

		void IPNDAgent::componentUp()
		{
			DiscoveryAgent::componentUp();

			try {
				ibrcommon::MulticastSocket &sock = dynamic_cast<ibrcommon::MulticastSocket&>(*_socket);
				sock.bind(_port);

				if (_interfaces.empty())
				{
					sock.joinGroup(_destination);
				}
				else
				{
					for (std::list<ibrcommon::vinterface>::const_iterator iter = _interfaces.begin(); iter != _interfaces.end(); iter++)
					{
						try {
							const ibrcommon::vinterface &net = (*iter);
							sock.joinGroup(_destination, net);
						} catch (const ibrcommon::vinterface::interface_not_set&) {
							sock.joinGroup(_destination);
						};
					}
				}
			} catch (std::bad_cast) {

			}

			try {
				ibrcommon::BroadcastSocket &sock = dynamic_cast<ibrcommon::BroadcastSocket&>(*_socket);
				sock.bind(_port, ibrcommon::vaddress());
			} catch (std::bad_cast) {

			}
		}

		void IPNDAgent::componentDown()
		{
			_socket->shutdown();
			DiscoveryAgent::componentDown();
		}

		void IPNDAgent::componentRun()
		{
			while (true)
			{
				DiscoveryAnnouncement announce(_version);

				char data[1500];

				std::string sender;
				int len = _socket->receive(data, 1500, sender);
				
				if (announce.isShort())
				{
					// TODO: generate name with the sender address
				}

				if (announce.getServices().empty())
				{
					announce.addService(dtn::net::DiscoveryService("tcpcl", "ip=" + sender + ";port=4556;"));
				}

				if (len < 0) return;

				stringstream ss;
				ss.write(data, len);

				try {
					ss >> announce;
					received(announce);
				} catch (dtn::InvalidDataException ex) {
				} catch (ibrcommon::IOException ex) {
				}

				yield();
			}
		}

		bool IPNDAgent::__cancellation()
		{
			// since this is an receiving thread we have to cancel the hard way
			return false;
		}

		const std::string IPNDAgent::getName() const
		{
			return "IPNDAgent";
		}
	}
}

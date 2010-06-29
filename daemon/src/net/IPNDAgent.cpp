/*
 * IPNDAgent.cpp
 *
 *  Created on: 14.09.2009
 *      Author: morgenro
 */

#include "net/IPNDAgent.h"
#include "ibrdtn/data/Exceptions.h"
#include <sstream>
#include <string.h>
#include <ibrcommon/Logger.h>
#include <ibrcommon/net/MulticastSocket.h>
#include <ibrcommon/net/BroadcastSocket.h>
#include <typeinfo>

namespace dtn
{
	namespace net
	{
		IPNDAgent::IPNDAgent(int port, std::string address)
		 : _socket(NULL), _destination(address), _port(port)
		{
			if (ibrcommon::MulticastSocket::isMulticast(_destination))
			{
				IBRCOMMON_LOGGER(info) << "DiscoveryAgent: multicast mode " << address << ":" << port << IBRCOMMON_LOGGER_ENDL;
				_socket = new ibrcommon::MulticastSocket();
			}
			else
			{
				IBRCOMMON_LOGGER(info) << "DiscoveryAgent: broadcast mode " << address << ":" << port << IBRCOMMON_LOGGER_ENDL;
				_socket = new ibrcommon::BroadcastSocket();
			}
		}

		IPNDAgent::~IPNDAgent()
		{
			delete _socket;
		}

		void IPNDAgent::bind(const ibrcommon::NetInterface &net)
		{
			IBRCOMMON_LOGGER(info) << "DiscoveryAgent: bind to interface " << net.getSystemName() << IBRCOMMON_LOGGER_ENDL;
			_interfaces.push_back(net);
		}

		void IPNDAgent::send(DiscoveryAnnouncement &announcement)
		{
			// update service blocks
			announcement.updateServices();

			stringstream ss;
			ss << announcement;

			string data = ss.str();

			ibrcommon::udpsocket::peer p = _socket->getPeer(_destination, _port);
			p.send(data.c_str(), data.length());
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
					ibrcommon::NetInterface &net = _interfaces.front();
					sock.setInterface(net);

					for (std::list<ibrcommon::NetInterface>::const_iterator iter = _interfaces.begin(); iter != _interfaces.end(); iter++)
					{
						sock.joinGroup(_destination, (*iter));
					}
				}
			} catch (std::bad_cast) {

			}

			try {
				ibrcommon::BroadcastSocket &sock = dynamic_cast<ibrcommon::BroadcastSocket&>(*_socket);

				if (_interfaces.empty())
				{
					sock.bind(_port);
				}
				else
				{
					ibrcommon::NetInterface &net = _interfaces.front();
					sock.bind(net);
				}
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
			_running = true;

			while (_running)
			{
				DiscoveryAnnouncement announce;

				char data[1500];

				int len = _socket->receive(data, 1500);

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
	}
}

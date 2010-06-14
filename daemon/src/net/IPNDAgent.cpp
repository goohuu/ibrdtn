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

namespace dtn
{
	namespace net
	{
		IPNDAgent::IPNDAgent(ibrcommon::NetInterface net, std::string address, int port)
		 : _socket(net, address, port)
		{
			IBRCOMMON_LOGGER(info) << "DiscoveryAgent listen to " << address << ":" << port << " on interface " << net.getSystemName() << IBRCOMMON_LOGGER_ENDL;
		}

		IPNDAgent::IPNDAgent(std::string address, int port)
		 : _socket(address, port)
		{
			IBRCOMMON_LOGGER(info) << "DiscoveryAgent listen to " << address << ":" << port << IBRCOMMON_LOGGER_ENDL;
		}

		IPNDAgent::IPNDAgent(ibrcommon::NetInterface net)
		 : _socket(net, true)
		{
			IBRCOMMON_LOGGER(info) << "DiscoveryAgent listen to port " << net.getPort() << IBRCOMMON_LOGGER_ENDL;
		}

		IPNDAgent::~IPNDAgent()
		{
		}

		void IPNDAgent::send(DiscoveryAnnouncement &announcement)
		{
			// update service blocks
			announcement.updateServices();

			stringstream ss;
			ss << announcement;

			string data = ss.str();

			_socket.send(data.c_str(), data.length());
		}

		void IPNDAgent::componentUp()
		{
			DiscoveryAgent::componentUp();

			// bind to interface and port
			_socket.bind();
		}

		void IPNDAgent::componentDown()
		{
			_socket.shutdown();
			DiscoveryAgent::componentDown();
		}

		void IPNDAgent::componentRun()
		{
			_running = true;

			while (_running)
			{
				DiscoveryAnnouncement announce;

				char data[1500];

				int len = _socket.receive(data, 1500);

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

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

namespace dtn
{
	namespace net
	{
		IPNDAgent::IPNDAgent(ibrcommon::NetInterface net)
		 : _socket(net, true)
		{
			// bind to interface and port
			_socket.bind();

			cout << "DiscoveryAgent listen to port " << net.getPort() << endl;
		}

		IPNDAgent::~IPNDAgent()
		{
			_socket.shutdown();
		}

		void IPNDAgent::send(DiscoveryAnnouncement &announcement)
		{
			// update service blocks
			announcement.updateServices();

			stringstream ss;
			ss << announcement;

			string data = ss.str();

			int ret = _socket.send(data.c_str(), data.length());
		}

		void IPNDAgent::run()
		{
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
				} catch (dtn::exceptions::InvalidDataException ex) {
				} catch (dtn::exceptions::IOException ex) {
				}

				yield();
			}
		}
	}
}

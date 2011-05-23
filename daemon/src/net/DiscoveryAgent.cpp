/*
 * DiscoveryAgent.cpp
 *
 *  Created on: 14.09.2009
 *      Author: morgenro
 */

#include "net/DiscoveryAgent.h"
#include "net/DiscoveryService.h"
#include "net/DiscoveryAnnouncement.h"
#include "core/TimeEvent.h"
#include "core/NodeEvent.h"
#include "core/Node.h"
#include <ibrdtn/utils/Utils.h>
#include "Configuration.h"

using namespace dtn::core;

namespace dtn
{
	namespace net
	{
		DiscoveryAgent::DiscoveryAgent(const dtn::daemon::Configuration::Discovery &config)
		 : _config(config), _sn(0), _clock(*this, 0)
		{
		}

		DiscoveryAgent::~DiscoveryAgent()
		{
		}

		void DiscoveryAgent::componentUp()
		{
			_clock.set(1);
			_clock.start();
		}

		void DiscoveryAgent::componentDown()
		{
			_clock.remove();

			stop();
			join();
		}

		void DiscoveryAgent::addService(string name, string parameters)
		{
			DiscoveryService service(name, parameters);
			_services.push_back(service);
		}

		void DiscoveryAgent::addService(DiscoveryServiceProvider *provider)
		{
			DiscoveryService service(provider);
			_services.push_back(service);
		}

		void DiscoveryAgent::received(const DiscoveryAnnouncement &announcement)
		{
			// calculating RTT with the service timestamp
			size_t rtt = 2700;

			// convert the announcement into NodeEvents
			Node n(Node::NODE_FLOATING, rtt);

			// set the EID and some parameters of this Node
			n.setEID(announcement.getEID());

			// get configured timeout value
			n.setTimeout( _config.timeout() );

			const list<DiscoveryService> services = announcement.getServices();
			for (list<DiscoveryService>::const_iterator iter = services.begin(); iter != services.end(); iter++)
			{
				const DiscoveryService &s = (*iter);

				if (s.getName() == "tcpcl")
				{
					n.add(Node::URI(s.getParameters(), Node::CONN_TCPIP));
				}
				else if (s.getName() == "udpcl")
				{
					n.add(Node::URI(s.getParameters(), Node::CONN_UDPIP));
				}
				else
				{
					n.add(Node::Attribute(s.getName(), s.getParameters()));
				}
			}

			// create and raise a new event
			dtn::core::NodeEvent::raise(n, NODE_INFO_UPDATED);
		}

		size_t DiscoveryAgent::timeout(size_t)
		{
			// check if announcements are enabled
			if (_config.announce())
			{
				sendAnnoucement(_sn, _services);

				// increment sequencenumber
				_sn++;
			}

			return 1;
		}
	}
}

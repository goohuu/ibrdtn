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
#include <ibrdtn/utils/Utils.h>
#include "Configuration.h"
#include <ibrcommon/Logger.h>

using namespace dtn::core;

namespace dtn
{
	namespace net
	{
		DiscoveryAgent::DiscoveryAgent(const dtn::daemon::Configuration::Discovery &config)
		 : _config(config), _sn(0)
		{
		}

		DiscoveryAgent::~DiscoveryAgent()
		{
		}

		void DiscoveryAgent::addService(std::string name, std::string parameters)
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

		void DiscoveryAgent::timeout()
		{
			// check if announcements are enabled
			if (_config.announce())
			{
				IBRCOMMON_LOGGER_DEBUG(25) << "send discovery announcement" << IBRCOMMON_LOGGER_ENDL;

				sendAnnoucement(_sn, _services);

				// increment sequencenumber
				_sn++;
			}
		}
	}
}

/*
 * DiscoveryAgent.cpp
 *
 *  Created on: 14.09.2009
 *      Author: morgenro
 */

#include "Configuration.h"
#include "net/DiscoveryAgent.h"
#include "net/DiscoveryService.h"
#include "net/DiscoveryAnnouncement.h"
#include "core/TimeEvent.h"
#include "core/NodeEvent.h"
#include <ibrdtn/utils/Utils.h>
#include <ibrdtn/utils/Clock.h>
#include <ibrcommon/Logger.h>

using namespace dtn::core;

namespace dtn
{
	namespace net
	{
		DiscoveryAgent::DiscoveryAgent(const dtn::daemon::Configuration::Discovery &config)
		 : _config(config), _sn(0), _last_announce_sent(0)
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

		void DiscoveryAgent::received(const DiscoveryAnnouncement &announcement, size_t timeout)
		{
			// convert the announcement into NodeEvents
			Node n(announcement.getEID());

			const std::list<DiscoveryService> services = announcement.getServices();
			for (std::list<DiscoveryService>::const_iterator iter = services.begin(); iter != services.end(); iter++)
			{
				size_t to_value = (timeout == 0) ? _config.timeout() : timeout;

				const DiscoveryService &s = (*iter);

				if (s.getName() == "tcpcl")
				{
					n.add(Node::URI(Node::NODE_DISCOVERED, Node::CONN_TCPIP, s.getParameters(), to_value));
				}
				else if (s.getName() == "udpcl")
				{
					n.add(Node::URI(Node::NODE_DISCOVERED, Node::CONN_UDPIP, s.getParameters(), to_value));
				}
				else if (s.getName() == "lowpancl")
				{
					n.add(Node::URI(Node::NODE_DISCOVERED, Node::CONN_LOWPAN, s.getParameters(), to_value));
				}
				else
				{
					n.add(Node::Attribute(Node::NODE_DISCOVERED, s.getName(), s.getParameters(), to_value));
				}
			}

			// create and raise a new event
			dtn::core::NodeEvent::raise(n, NODE_INFO_UPDATED);

			// if continuous announcements are disabled, then reply to this message
			if (!_config.announce())
			{
				// first check if another announcement was sent during the same seconds
				if (_last_announce_sent != dtn::utils::Clock::getTime())
				{
					IBRCOMMON_LOGGER_DEBUG(25) << "reply with discovery announcement" << IBRCOMMON_LOGGER_ENDL;

					sendAnnoucement(_sn, _services);

					// increment sequencenumber
					_sn++;

					// save the time of the last sent announcement
					_last_announce_sent = dtn::utils::Clock::getTime();
				}
			}
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

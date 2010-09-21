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
#include <ibrcommon/thread/MutexLock.h>
#include "Configuration.h"

using namespace dtn::core;

namespace dtn
{
	namespace net
	{
		DiscoveryAgent::DiscoveryAgent(const dtn::daemon::Configuration::Discovery &config)
		 : _config(config), _running(false), _sn(0)
		{
		}

		DiscoveryAgent::~DiscoveryAgent()
		{
		}

		void DiscoveryAgent::componentUp()
		{
			bindEvent(TimeEvent::className);
		}

		void DiscoveryAgent::componentDown()
		{
			unbindEvent(TimeEvent::className);

			_running = false;
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
			n.setURI(announcement.getEID().getString());

			// get configured timeout value
			n.setTimeout( _config.timeout() );

			const list<DiscoveryService> services = announcement.getServices();
			for (list<DiscoveryService>::const_iterator iter = services.begin(); iter != services.end(); iter++)
			{
				const DiscoveryService &s = (*iter);

				if (s.getName() == "tcpcl")
				{
					n.setProtocol(Node::CONN_TCPIP);
				}
				else if (s.getName() == "udpcl")
				{
					n.setProtocol(Node::CONN_UDPIP);
				}
				else
				{
					continue;
				}

				// parse parameters
				std::vector<string> parameters = dtn::utils::Utils::tokenize(";", s.getParameters());
				std::vector<string>::const_iterator param_iter = parameters.begin();

				while (param_iter != parameters.end())
				{
					std::vector<string> p = dtn::utils::Utils::tokenize("=", (*param_iter));

					if (p[0].compare("ip") == 0)
					{
						n.setAddress(p[1]);
					}

					if (p[0].compare("port") == 0)
					{
						int port_number = 0;
						stringstream port_stream;
						port_stream << p[1];
						port_stream >> port_number;
						n.setPort(port_number);
					}

					param_iter++;
				}

				// create and raise a new event
				dtn::core::NodeEvent::raise(n, NODE_INFO_UPDATED);
			}
		}

		void DiscoveryAgent::raiseEvent(const dtn::core::Event *evt)
		{
			try {
				dynamic_cast<const dtn::core::TimeEvent&>(*evt);

				// check if announcements are enabled
				if (_config.announce())
				{
					static ibrcommon::Mutex mutex;
					ibrcommon::MutexLock l(mutex);

					// update all services
					for (std::list<DiscoveryService>::iterator iter = _services.begin(); iter != _services.end(); iter++)
					{
						(*iter).update();
					}

					sendAnnoucement(_sn, _services);

					// increment sequencenumber
					_sn++;
				}
			} catch (std::bad_cast) {

			}
		}
	}
}

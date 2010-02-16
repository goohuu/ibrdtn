/*
 * DiscoveryAgent.cpp
 *
 *  Created on: 14.09.2009
 *      Author: morgenro
 */

#include "net/DiscoveryAgent.h"
#include "net/DiscoveryService.h"
#include "core/TimeEvent.h"
#include "core/BundleCore.h"
#include "core/NodeEvent.h"
#include "core/Node.h"
#include "ibrdtn/utils/Utils.h"

using namespace dtn::core;

namespace dtn
{
	namespace net
	{
		DiscoveryAgent::DiscoveryAgent()
		 : _running(true), _self_announce(false, dtn::core::BundleCore::local)
		{
			bindEvent(TimeEvent::className);
		}

		DiscoveryAgent::~DiscoveryAgent()
		{
			unbindEvent(TimeEvent::className);

			_running = false;
			join();
		}

		void DiscoveryAgent::addService(string name, string parameters)
		{
			DiscoveryService service(name, parameters);
			_self_announce.addService(service);
		}

                void DiscoveryAgent::addService(DiscoveryServiceProvider *provider)
                {
                        DiscoveryService service(provider);
                        _self_announce.addService(service);
                }

		void DiscoveryAgent::received(const DiscoveryAnnouncement &announcement)
		{
			// calculating RTT with the service timestamp
			size_t rtt = 2700;

			// convert the announcement into NodeEvents
			Node n(FLOATING, rtt);

			// set the EID and some parameters of this Node
			n.setURI(announcement.getEID().getString());
			n.setTimeout(5);

			const list<DiscoveryService> services = announcement.getServices();
			list<DiscoveryService>::const_iterator iter = services.begin();

			while (iter != services.end())
			{
				const DiscoveryService &s = (*iter);

				if (s.getName() == "tcpcl") n.setProtocol(TCP_CONNECTION);
				if (s.getName() == "udpcl") n.setProtocol(UDP_CONNECTION);

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

				iter++;
			}
		}

		void DiscoveryAgent::raiseEvent(const dtn::core::Event *evt)
		{
			static ibrcommon::Mutex mutex;
			ibrcommon::MutexLock l(mutex);

			const dtn::core::TimeEvent *time = dynamic_cast<const dtn::core::TimeEvent*>(evt);
			if (time == NULL) return;

			this->send(_self_announce);
		}
	}
}

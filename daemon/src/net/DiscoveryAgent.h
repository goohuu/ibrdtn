/*
 * DiscoveryAgent.h
 *
 *  Created on: 09.09.2009
 *      Author: morgenro
 */

#ifndef DISCOVERYAGENT_H_
#define DISCOVERYAGENT_H_

#include "Component.h"

#include "core/Node.h"
#include "core/EventReceiver.h"

#include "net/Neighbor.h"
#include "net/DiscoveryAnnouncement.h"
#include "net/DiscoveryService.h"
#include "Configuration.h"
#include <ibrcommon/thread/Timer.h>

using namespace dtn::data;

#include <list>

namespace dtn
{
	namespace net
	{
		class DiscoveryAgent : public dtn::daemon::IndependentComponent, public dtn::core::EventReceiver, public ibrcommon::SimpleTimerCallback
		{
		public:
			DiscoveryAgent(const dtn::daemon::Configuration::Discovery &config);
			virtual ~DiscoveryAgent() = 0;

			void received(const DiscoveryAnnouncement &announcement);
			void raiseEvent(const dtn::core::Event *evt);

			void addService(string name, string parameters);
			void addService(DiscoveryServiceProvider *provider);

			virtual size_t timeout(size_t identifier);

		protected:
			virtual void componentUp();
			virtual void componentDown();
			virtual void componentRun() = 0;
			virtual void sendAnnoucement(const u_int16_t &sn, const std::list<DiscoveryService> &services) = 0;

			const dtn::daemon::Configuration::Discovery &_config;

		private:
			list<Neighbor> _neighbors;
			u_int16_t _sn;
			std::list<DiscoveryService> _services;
			ibrcommon::SimpleTimer _clock;
		};
	}
}

#endif /* DISCOVERYAGENT_H_ */

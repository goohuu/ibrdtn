/*
 * DiscoveryAgent.h
 *
 *  Created on: 09.09.2009
 *      Author: morgenro
 */

#ifndef DISCOVERYAGENT_H_
#define DISCOVERYAGENT_H_

#include "ibrdtn/config.h"
#include "ibrcommon/thread/Thread.h"

#include "core/Node.h"
#include "core/EventReceiver.h"

#include "net/Neighbor.h"
#include "net/DiscoveryAnnouncement.h"

using namespace dtn::data;

#include <list>

namespace dtn
{
	namespace net
	{
		class DiscoveryAgent : public ibrcommon::JoinableThread, public dtn::core::EventReceiver
		{
		public:
			DiscoveryAgent();
			virtual ~DiscoveryAgent();

			void received(const DiscoveryAnnouncement &announcement);
			void raiseEvent(const dtn::core::Event *evt);

			void addService(string name, string parameters);
                        void addService(DiscoveryServiceProvider *provider);

		protected:
			virtual void run() = 0;
			virtual void send(DiscoveryAnnouncement &announcement) = 0;
			bool _running;

		private:
			DiscoveryAnnouncement _self_announce;
			list<Neighbor> _neighbors;
		};
	}
}

#endif /* DISCOVERYAGENT_H_ */

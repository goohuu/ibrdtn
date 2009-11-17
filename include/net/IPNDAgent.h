/*
 * IPNDAgent.h
 *
 *  Created on: 09.09.2009
 *      Author: morgenro
 *
 * IPND is based on the Internet-Draft for DTN-IPND.
 *
 * MUST support multicast
 * SHOULD support broadcast and unicast
 *
 *
 *
 */

#ifndef IPNDAGENT_H_
#define IPNDAGENT_H_

#include "ibrdtn/config.h"
#include "net/DiscoveryAgent.h"
#include "net/DiscoveryAnnouncement.h"

using namespace dtn::data;

namespace dtn
{
	namespace net
	{
		class IPNDAgent : public DiscoveryAgent
		{
		public:
			IPNDAgent(string broadcast_ip = "255.255.255.255", size_t port = 4551);
			virtual ~IPNDAgent();

		protected:
			void send(DiscoveryAnnouncement &announcement);
			void run();

		private:
			int _socket;
			int _port;
			string _ip;
		};
	}
}

#endif /* IPNDAGENT_H_ */

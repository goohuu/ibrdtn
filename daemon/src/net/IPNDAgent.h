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

#include "net/DiscoveryAgent.h"
#include "net/DiscoveryAnnouncement.h"
#include <ibrcommon/net/NetInterface.h>
#include <ibrcommon/net/udpsocket.h>

using namespace dtn::data;

namespace dtn
{
	namespace net
	{
		class IPNDAgent : public DiscoveryAgent
		{
		public:
			IPNDAgent(ibrcommon::NetInterface net, std::string address, int port);
			IPNDAgent(std::string address, int port);
			IPNDAgent(ibrcommon::NetInterface net);
			virtual ~IPNDAgent();

		protected:
			void send(DiscoveryAnnouncement &announcement);
			virtual void componentRun();
			virtual void componentUp();
			virtual void componentDown();

		private:
			ibrcommon::udpsocket _socket;
		};
	}
}

#endif /* IPNDAGENT_H_ */

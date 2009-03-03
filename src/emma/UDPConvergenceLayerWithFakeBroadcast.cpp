#include "emma/UDPConvergenceLayerWithFakeBroadcast.h"
#include "utils/Utils.h"

using namespace dtn::data;

namespace emma
{
	UDPConvergenceLayerWithFakeBroadcast::UDPConvergenceLayerWithFakeBroadcast(string bind_addr, unsigned short port, bool broadcast, unsigned int mtu)
		:	UDPConvergenceLayer(bind_addr, port, broadcast, mtu)
	{
		// get configuration object
		//Configuration &conf = Configuration::getInstance();
		//m_nodes = conf.getFakeBroadcastNodes();
	}

	TransmitReport UDPConvergenceLayerWithFakeBroadcast::transmit(Bundle *b)
	{
		list<Node>::iterator iter = m_nodes.begin();

		while (iter != m_nodes.end())
		{
			UDPConvergenceLayer::transmit(b, (*iter));
			iter++;
		}

		return TRANSMIT_SUCCESSFUL;
	}
}

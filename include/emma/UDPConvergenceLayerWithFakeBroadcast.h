#ifndef UDPCONVERGENCELAYERWITHFAKEBROADCAST_H_
#define UDPCONVERGENCELAYERWITHFAKEBROADCAST_H_

#include "core/UDPConvergenceLayer.h"
#include "data/Bundle.h"
#include "core/Node.h"
#include <list>

using namespace dtn::data;
using namespace dtn::core;

namespace emma
{
	/**
	 * Diese Klasse implementiert einen ConvergenceLayer für UDP/IP
	 * mit einem Fake-Broadcast Modul.
	 */
	class UDPConvergenceLayerWithFakeBroadcast : public UDPConvergenceLayer
	{
	public:
		/**
		 * Constructor
		 * @param[in] bind_addr The own eid for broadcasting discovery bundles.
		 * @param[in] port The udp port to use.
		 * @param[in] broadcast If true, the broadcast feature for this socket is enabled.
		 * @param[in] mtu The maximum bundle size.
		 */
		UDPConvergenceLayerWithFakeBroadcast(string bind_addr, unsigned short port, bool broadcast = false, unsigned int mtu = 1280);

		/**
		 * @sa protocol::ConvergenceLayer::transmit(Bundle *b)
		 */
		virtual TransmitReport transmit(Bundle *b);

	private:
		list<Node> m_nodes;
	};
}

#endif /*UDPCONVERGENCELAYERWITHFAKEBROADCAST_H_*/

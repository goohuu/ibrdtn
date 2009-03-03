#ifndef UDPCONVERGENCELAYER_H_
#define UDPCONVERGENCELAYER_H_

#include "ConvergenceLayer.h"
#include "utils/Service.h"
#include "utils/MutexLock.h"
#include "utils/Mutex.h"

using namespace dtn::data;
using namespace dtn::utils;

namespace dtn
{
	namespace core
	{
		/**
		 * Diese Klasse implementiert einen ConvergenceLayer für UDP/IP
		 */
		class UDPConvergenceLayer : public Service, public ConvergenceLayer
		{
		public:
			/**
			 * Constructor
			 * @param[in] bind_addr The own eid for broadcasting discovery bundles.
			 * @param[in] port The udp port to use.
			 * @param[in] broadcast If true, the broadcast feature for this socket is enabled.
			 * @param[in] mtu The maximum bundle size.
			 */
			UDPConvergenceLayer(string bind_addr, unsigned short port, bool broadcast = false, unsigned int mtu = 1280);

			/**
			 * Desktruktor
			 */
			virtual ~UDPConvergenceLayer();

			/**
			 * @sa protocol::ConvergenceLayer::transmit(Bundle *b)
			 */
			virtual TransmitReport transmit(Bundle *b);

			/**
			 * @sa protocol::ConvergenceLayer::transmit(Bundle *b, Node &node)
			 */
			virtual TransmitReport transmit(Bundle *b, const Node &node);

			/**
			 * @sa Service::tick()
			 */
			virtual void tick();

		private:
			/**
			 * Empfängt ein Bundle aus der Netzwerkschicht
			 */
			void receiveBundle();

			int m_socket;

			//static const int MAX_SIZE;
			static const int DEFAULT_PORT;

			unsigned int m_maxmsgsize;

			string m_selfaddr;
			unsigned int m_selfport;

			Mutex m_writelock;

			//int m_fakeloss;
		};
	}
}

#endif /*UDPCONVERGENCELAYER_H_*/

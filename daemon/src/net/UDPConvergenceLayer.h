#ifndef UDPCONVERGENCELAYER_H_
#define UDPCONVERGENCELAYER_H_

#include "Component.h"
#include "net/ConvergenceLayer.h"
#include "ibrcommon/Exceptions.h"
#include "net/DiscoveryServiceProvider.h"
#include "ibrcommon/net/NetInterface.h"
#include "ibrcommon/net/udpsocket.h"

using namespace dtn::data;


namespace dtn
{
	namespace net
	{
		/**
		 * This class implement a ConvergenceLayer for UDP/IP.
		 * Each bundle is sent in exact one UDP datagram.
		 */
		class UDPConvergenceLayer : public ConvergenceLayer, public dtn::daemon::IndependentComponent, public DiscoveryServiceProvider
		{
		public:
			/**
			 * Constructor
			 * @param[in] bind_addr The address to bind.
			 * @param[in] port The udp port to use.
			 * @param[in] broadcast If true, the broadcast feature for this socket is enabled.
			 * @param[in] mtu The maximum bundle size.
			 */
			UDPConvergenceLayer(ibrcommon::NetInterface net, bool broadcast = false, unsigned int mtu = 1280);

			/**
			 * Desktruktor
			 */
			virtual ~UDPConvergenceLayer();

			virtual void update(std::string &name, std::string &data);
			virtual bool onInterface(const ibrcommon::NetInterface &net) const;

			dtn::core::NodeProtocol getDiscoveryProtocol() const;

			void queue(const dtn::core::Node &n, const ConvergenceLayer::Job &job);

			UDPConvergenceLayer& operator>>(dtn::data::Bundle&);

		protected:
			virtual void componentUp();
			virtual void componentRun();
			virtual void componentDown();

		private:
			ibrcommon::udpsocket *_socket;

			ibrcommon::NetInterface _net;
			int m_socket;

			static const int DEFAULT_PORT;

			unsigned int m_maxmsgsize;

			ibrcommon::Mutex m_writelock;
			ibrcommon::Mutex m_readlock;

			bool _running;


		};
	}
}

#endif /*UDPCONVERGENCELAYER_H_*/

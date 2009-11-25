#ifndef UDPCONVERGENCELAYER_H_
#define UDPCONVERGENCELAYER_H_

#include "net/ConvergenceLayer.h"
#include "net/BundleConnection.h"
#include "ibrdtn/data/Exceptions.h"
#include "net/DiscoveryServiceProvider.h"
#include "ibrdtn/utils/NetInterface.h"

using namespace dtn::data;


namespace dtn
{
	namespace net
	{
		/**
		 * This class implement a ConvergenceLayer for UDP/IP.
		 * Each bundle is sent in exact one UDP datagram.
		 */
		class UDPConvergenceLayer : public ConvergenceLayer, public dtn::utils::JoinableThread, public DiscoveryServiceProvider
		{
		public:
			class SocketException : public dtn::exceptions::Exception
			{
			public:
				SocketException(string error) : Exception(error)
				{};
			};

			/**
			 * Constructor
			 * @param[in] bind_addr The address to bind.
			 * @param[in] port The udp port to use.
			 * @param[in] broadcast If true, the broadcast feature for this socket is enabled.
			 * @param[in] mtu The maximum bundle size.
			 */
			UDPConvergenceLayer(NetInterface net, bool broadcast = false, unsigned int mtu = 1280);

			/**
			 * Desktruktor
			 */
			virtual ~UDPConvergenceLayer();

			/**
			 * @sa protocol::ConvergenceLayer::transmit(Bundle *b)
			 */
			virtual TransmitReport transmit(const dtn::data::Bundle &b);

			/**
			 * @sa protocol::ConvergenceLayer::transmit(Bundle *b, Node &node)
			 */
			virtual TransmitReport transmit(const dtn::data::Bundle &b, const dtn::core::Node &node);

			void receive(dtn::data::Bundle &bundle);

			BundleConnection* getConnection(const dtn::core::Node &n);

                        virtual void update(std::string &name, std::string &data);

		protected:
				void run();

		private:
			NetInterface _net;
			int m_socket;

			static const int DEFAULT_PORT;

			unsigned int m_maxmsgsize;

			dtn::utils::Mutex m_writelock;
			dtn::utils::Mutex m_readlock;

			bool _running;
		};
	}
}

#endif /*UDPCONVERGENCELAYER_H_*/

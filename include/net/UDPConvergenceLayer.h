#ifndef UDPCONVERGENCELAYER_H_
#define UDPCONVERGENCELAYER_H_

#include "net/ConvergenceLayer.h"
#include "net/BundleConnection.h"
#include "ibrdtn/data/Exceptions.h"
#include "net/DiscoveryServiceProvider.h"
#include "ibrcommon/net/NetInterface.h"

using namespace dtn::data;


namespace dtn
{
	namespace net
	{
		/**
		 * This class implement a ConvergenceLayer for UDP/IP.
		 * Each bundle is sent in exact one UDP datagram.
		 */
		class UDPConvergenceLayer : public ConvergenceLayer, public ibrcommon::JoinableThread, public DiscoveryServiceProvider
		{
		public:
			class SocketException : public dtn::exceptions::Exception
			{
			public:
				SocketException(string error) : Exception(error)
				{};
			};

			/**
			 * UDP connection class
			 */
			class UDPConnection : public BundleConnection
			{
			public:
				UDPConnection(UDPConvergenceLayer &cl, const dtn::core::Node &node);
				virtual ~UDPConnection();

				void write(const dtn::data::Bundle &bundle);
				void read(dtn::data::Bundle &bundle);

				void shutdown();

			private:
				UDPConvergenceLayer &_cl;
				dtn::core::Node _node;
			};

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

			/**
			 * @sa protocol::ConvergenceLayer::transmit(Bundle *b, Node &node)
			 */
			virtual void transmit(const dtn::data::Bundle &b, const dtn::core::Node &node);

			void receive(dtn::data::Bundle &bundle);

			BundleConnection* getConnection(const dtn::core::Node &n);

			virtual void update(std::string &name, std::string &data);

		protected:
				void run();

		private:
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

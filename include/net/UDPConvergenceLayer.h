#ifndef UDPCONVERGENCELAYER_H_
#define UDPCONVERGENCELAYER_H_

#include "net/ConvergenceLayer.h"
#include "net/BundleConnection.h"
#include "ibrdtn/data/Exceptions.h"

using namespace dtn::data;


namespace dtn
{
	namespace net
	{
		/**
		 * This class implement a ConvergenceLayer for UDP/IP.
		 * Each bundle is sent in exact one UDP datagram.
		 */
		class UDPConvergenceLayer : public ConvergenceLayer, public dtn::utils::JoinableThread
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
			UDPConvergenceLayer(string bind_addr = "0.0.0.0", unsigned short port = 4556, bool broadcast = false, unsigned int mtu = 1280);

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

		protected:
				void run();

		private:

			int m_socket;

			static const int DEFAULT_PORT;

			unsigned int m_maxmsgsize;

			string m_selfaddr;
			unsigned int m_selfport;

			dtn::utils::Mutex m_writelock;
			dtn::utils::Mutex m_readlock;

			bool _running;
		};
	}
}

#endif /*UDPCONVERGENCELAYER_H_*/

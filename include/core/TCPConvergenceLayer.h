/**
 *
 * A TCPConvergenceLayer implements a convergence layer for TCP/IP
 * according to draft-irtf-dtnrg-tcp-clayer-02.
 *
 * The TCPConnection class is used to manage multiple concurrent
 * connections.
 *
 */

#ifndef TCPCONVERGENCELAYER_H_
#define TCPCONVERGENCELAYER_H_

#include "ConvergenceLayer.h"
#include "utils/Service.h"
#include "utils/MutexLock.h"
#include "utils/Mutex.h"
#include "data/NetworkFrame.h"
#include <list>

using namespace dtn::data;
using namespace dtn::utils;

namespace dtn
{
	namespace core
	{
		class TCPConnection;

		/**
		 * This class implements a ConvergenceLayer for TCP/IP.
		 */
		class TCPConvergenceLayer : public Service, public ConvergenceLayer
		{
			friend class TCPConnection;

		public:
			/**
			 * Constructor
			 * @param bind_addr The IP address to listen on for the TCP communication.
			 * @param port The port to listen on for the TCP communication.
			 */
			TCPConvergenceLayer(string localeid, string bind_addr, unsigned short port);

			/**
			 * Destructor
			 */
			virtual ~TCPConvergenceLayer();

			/**
			 * @sa protocol::ConvergenceLayer::transmit(Bundle *b)
			 */
			virtual TransmitReport transmit(const Bundle &b);

			/**
			 * @sa protocol::ConvergenceLayer::transmit(Bundle *b, Node &node)
			 */
			virtual TransmitReport transmit(const Bundle &b, const Node &node);

			/**
			 * @sa Service::tick()
			 */
			virtual void tick();

		protected:
			void callbackBundleReceived(const TCPConnection &conn, Bundle &b);

		private:
			TransmitReport transmit(TCPConnection *conn, const Bundle &b);
			TCPConnection* newConnection(int socket);
			bool build_select_list();
			void setnonblocking(int sock);

			int m_socket;

			static const int DEFAULT_PORT;

			string m_selfaddr;
			unsigned int m_selfport;

			Mutex m_writelock;
			Mutex m_mutex_connections;

			list<TCPConnection*> m_connections;

			fd_set m_socks_recv;
			fd_set m_socks_send;

			int m_highsock;

			NetworkFrame m_contactheader;

			string m_localeid;
		};
	}
}

#endif /*TCPCONVERGENCELAYER_H_*/

/**
 *
 * TCPConvergenceLayer überträgt ein Bündel über TCP/IP an einen
 * anderen TCPConvergenceLayer.
 *
 * Für jedes Ziel (Node) wird genau eine Verbindung aufgebaut, über die
 * Bündel übertragen werden können. Verbindungen bleiben prinzipiell offen
 * solange es möglich ist.
 *
 * Rahmenformat:
 * | Länge (SDNV) | EID | Länge (SDNV) | Bündel | Länge (SDNV) | Bündel | usw.
 *
 * Tritt ein Dekodierungsfehler bei der Bündeldekodierung auf wird die
 * Verbindung abgebrochen.
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
		 * Diese Klasse implementiert einen ConvergenceLayer für TCP/IP
		 */
		class TCPConvergenceLayer : public Service, public ConvergenceLayer
		{
			friend class TCPConnection;

		public:
			/**
			 * Konstruktor
			 * @param bind_addr IP Adresse des Interface, welche zur Kommunikation genutzt werden soll
			 * @param port Zu belegender TCP Port für die Kommunikation
			 */
			TCPConvergenceLayer(string bind_addr, unsigned short port);

			/**
			 * Desktruktor
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
			void callbackBundleReceived(TCPConnection *conn, Bundle *b);

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
		};
	}
}

#endif /*TCPCONVERGENCELAYER_H_*/

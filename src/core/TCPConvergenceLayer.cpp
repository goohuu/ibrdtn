#include "config.h"
#include "core/TCPConvergenceLayer.h"
#include "data/BundleFactory.h"
#include "core/TCPConnection.h"

#include "utils/Utils.h"
#include <sys/socket.h>
#include <poll.h>
#include <errno.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include "utils/Utils.h"
#include "data/SDNV.h"

#include <iostream>

using namespace dtn::data;
using namespace dtn::utils;


namespace dtn
{
	namespace core
	{
		const int TCPConvergenceLayer::DEFAULT_PORT = 4556;

		TCPConvergenceLayer::TCPConvergenceLayer(string localeid, string bind_addr, unsigned short port)
			: Service("TCPConvergenceLayer"), m_selfaddr(bind_addr), m_selfport(port), m_highsock(0), m_localeid(localeid)
		{
			/**
			 * create the contact header
			 *
			 * magic = 4 byte
			 * version = 1 byte
			 * flags = 1 byte
			 * keepalive_interval = 2 byte
			 * eid length = SDNV
			 * eid = eid length
			 */
			char magic[4] = { 'd', 't', 'n', '!' };
			m_contactheader.append((unsigned char*)&magic, 4);

			m_contactheader.append(BUNDLE_VERSION);

			unsigned int flags = 1;
			m_contactheader.append((unsigned char*)&flags, 1);

			unsigned int keepalive = 0;
			m_contactheader.append((unsigned char*)&keepalive, 2);

			// put local eid to contact header
			m_contactheader.append(m_localeid.length());
			m_contactheader.append((unsigned char*)m_localeid.data(), m_localeid.length());

			/** header complete **/

			struct sockaddr_in address;

			address.sin_family = AF_INET;
			address.sin_addr.s_addr = inet_addr(bind_addr.c_str());
		  	address.sin_port = htons(port);

			// Create socket for listening for client connection requests.
			m_socket = socket(AF_INET, SOCK_STREAM, 0);

			if (m_socket < 0)
			{
				cerr << "TCPConvergenceLayer: cannot create listen socket" << endl;
				exit(1);
			}

			setnonblocking(m_socket);

			address.sin_family = AF_INET;

			if ( bind(m_socket, (struct sockaddr *) &address, sizeof(address)) < 0 )
			{
				cerr << "TCPConvergenceLayer: cannot bind socket" << endl;
			    exit(1);
			}

			listen (m_socket, 5);

			m_connections.clear();
		}

		TCPConvergenceLayer::~TCPConvergenceLayer()
		{
			MutexLock l(m_mutex_connections);

			// close all existing connections
			list<TCPConnection*>::iterator iter = m_connections.begin();

			while (iter != m_connections.end())
			{
				delete (*iter);
				iter++;
			}

			// close the listen socket
			close(m_socket);
		}

		TransmitReport TCPConvergenceLayer::transmit(const Bundle &b)
		{
		    return NO_ROUTE_FOUND;
		}

		TransmitReport TCPConvergenceLayer::transmit(const Bundle &b, const Node &node)
		{
			// if we're not running, just do nothing.
			if (!this->isRunning()) return CONVERGENCE_LAYER_BUSY;

			// search for a existing connection
			TCPConnection *conn = NULL;

			MutexLock l(m_mutex_connections);

			list<TCPConnection*>::iterator iter = m_connections.begin();

			while (iter != m_connections.end())
			{
				if ( ((*iter)->getRemoteURI() == node.getURI()) && ((*iter)->isClosed() == false) )
				{
					conn = (*iter);
					break;
				}

				iter++;
			}

			l.unlock();

			if (conn == NULL)
			{
				int sock;
				struct sockaddr_in address;

				sock = socket(AF_INET, SOCK_STREAM, 0);

				if (sock <= 0)
				{
					// error
					return CONVERGENCE_LAYER_BUSY;
				}

				address.sin_family = AF_INET;
				address.sin_addr.s_addr = inet_addr(node.getAddress().c_str());
				address.sin_port = htons(node.getPort());

				if (connect ( sock, (struct sockaddr *) &address, sizeof (address)) != 0)
				{
					// error
					return CONVERGENCE_LAYER_BUSY;
				}

				conn = newConnection(sock);
			}

			if (conn == NULL)
			{
				return CONVERGENCE_LAYER_BUSY;
			}

			return transmit(conn, b);
		}

		/**
		 *
		 */
		TransmitReport TCPConvergenceLayer::transmit(TCPConnection *conn, const Bundle &b)
		{
			// send bundle data
			size_t sent = conn->transmit((char*)b.getData(), b.getLength());

			// wait for transfer complete
			if ( sent != b.getLength() )
			{
				// connection is closed and data may incomplete
				return CONVERGENCE_LAYER_BUSY;
			}

//			// is anything transferred?
//			if (m_outgoing.remain >= m_outgoing.payloadsize)
//			{
//				// no, just free the array
//				free(m_outgoing.data);
//				m_outgoing.data = NULL;
//
//				// return to idle state
//				setState(m_outgoing, STATE_IDLE);
//
//				// and return with BUSY
//				return CONVERGENCE_LAYER_BUSY;
//			}
//
//			/**
//			 * reactive fragmentation
//			 *
//			 * fragmentation on both sides is only possible if the complete
//			 * header is transferred. check this in first!
//			 */
//
//			// get the size of successful transferred data
//			unsigned int offset = m_outgoing.payloadsize - m_outgoing.remain;
//
//			if ( offset < b->getOverhead() )
//			{
//				// not even the header is transferred successful, just abort
//				free(m_outgoing.data);
//				m_outgoing.data = NULL;
//
//				// return to idle state
//				setState(m_outgoing, STATE_IDLE);
//
//				// and return with BUSY
//				return CONVERGENCE_LAYER_BUSY;
//			}
//
//			// reduce to the remaining bundle payload
//			offset -= b->getOverhead();
//
//			// create a fragment of the remaining data
//			Bundle *fragment = BundleFactory::cut(b, UINT_MAX, offset);
//
//			// TODO: return the fragment to the BundleCore
//			cout << "outgoing fragment created" << fragment->toString() << endl;
//
//			delete fragment;
//
//			// free the array
//			free(m_outgoing.data);
//			m_outgoing.data = NULL;
//
//			// return to idle state
//			setState(m_outgoing, STATE_IDLE);

			return TRANSMIT_SUCCESSFUL;
		}

		/**
		 * Build the list of waiting sockets
		 * @returns returns true if data is waiting to sent, else false
		 */
		bool TCPConvergenceLayer::build_select_list()
		{
			/* First put together fd_set for select(), which will
			   consist of the sock veriable in case a new connection
			   is coming in, plus all the sockets we have already
			   accepted. */
			bool ret = false;


			/* FD_ZERO() clears out the fd_set called socks, so that
				it doesn't contain any file descriptors. */

			FD_ZERO(&m_socks_recv);
			FD_ZERO(&m_socks_send);

			/* FD_SET() adds the file descriptor "sock" to the fd_set,
				so that select() will return if a connection comes in
				on that socket (which means you have to do accept(), etc. */

			FD_SET(m_socket, &m_socks_recv);
			FD_SET(m_socket, &m_socks_send);

			/* Loops through all the possible connections and adds
				those sockets to the fd_set */

			MutexLock l(m_mutex_connections);
			list<TCPConnection*>::iterator iter = m_connections.begin();

			while (iter != m_connections.end())
			{
				TCPConnection *conn = (*iter);

				if ( conn->isClosed() )
				{
#ifdef DO_DEBUG_OUTPUT
					// Wegräumen
					cout << "remove closed connection to " << conn->getRemoteURI() << endl;
#endif

					delete conn;
					m_connections.erase(iter++);
				}
				else
				{
					// add to read sockets
					FD_SET(conn->getSocket(),&m_socks_recv);

					// trigger idle checks
					conn->idlechecking();

					// add to send sockets
					if (conn->datawaiting())
					{
						FD_SET(conn->getSocket(),&m_socks_send);
						ret = true;
					}
					iter++;
				}
			}

			return ret;
		}

		void TCPConvergenceLayer::setnonblocking(int sock)
		{
			int opts;

			opts = fcntl(sock,F_GETFL);
			if (opts < 0) {
				perror("fcntl(F_GETFL)");
				exit(EXIT_FAILURE);
			}
			opts = (opts | O_NONBLOCK);
			if (fcntl(sock,F_SETFL,opts) < 0) {
				perror("fcntl(F_SETFL)");
				exit(EXIT_FAILURE);
			}
			return;
		}

		void TCPConvergenceLayer::tick()
		{
			int readsocks;	     /* Number of sockets ready for reading */
			struct timeval timeout;  /* Timeout for select */

			if (m_highsock < m_socket) m_highsock = m_socket;

			while (this->isRunning())
			{
				if (build_select_list())
				{
					timeout.tv_sec = 0;
					timeout.tv_usec = 10000;
				}
				else
				{
					timeout.tv_sec = 0;
					timeout.tv_usec = 1000;
				}

				readsocks = select(m_highsock+1, &m_socks_recv, &m_socks_send, NULL, &timeout);

				if (readsocks < 0)
				{
					perror("select");
					exit(EXIT_FAILURE);
				}

				if (readsocks == 0)
				{
					/* No socket ready to read or write */
				}
				else
				{
					if ( FD_ISSET(m_socket,&m_socks_recv) )
					{
						int new_socket;
						struct sockaddr_in address;

						address.sin_family = AF_INET;
						address.sin_addr.s_addr = INADDR_ANY;
						address.sin_port = 0;

						socklen_t addrlen = sizeof (struct sockaddr_in);

						new_socket = accept ( m_socket, (struct sockaddr *) &address, &addrlen );

						// create a new connection object
						newConnection(new_socket);
					}
					else
					{
						MutexLock l(m_mutex_connections);
						list<TCPConnection*>::iterator iter = m_connections.begin();

						while (iter != m_connections.end())
						{
							TCPConnection *conn = (*iter);

							// send data if the socket is ready
							if ( FD_ISSET(conn->getSocket(),&m_socks_send) )
							{
								conn->transmit();
							}

							// receive data if the socket has data
							if ( FD_ISSET(conn->getSocket(),&m_socks_recv) )
							{
								conn->receive();
							}

							iter++;
						}
					}
				}
			}
		}

		TCPConnection* TCPConvergenceLayer::newConnection(int socket)
		{
			// switch to non-blocking
			setnonblocking(socket);

			if (socket > m_highsock) m_highsock = socket;

			// create a new connection
			TCPConnection *conn = new TCPConnection(*this, socket, 512);

			// exchange contactheader
			conn->handshake(m_contactheader.getData(), m_contactheader.getSize());

			if (conn != NULL)
			{
				MutexLock l(m_mutex_connections);

				// add the ConvergenceLayer to the map
				m_connections.push_back(conn);
			}

			return conn;
		}

		void TCPConvergenceLayer::callbackBundleReceived(const TCPConnection &conn, Bundle &b)
		{
			eventBundleReceived(b);
		}
	}
}

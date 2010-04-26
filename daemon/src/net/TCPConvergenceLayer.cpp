/*
 * TCPConvergenceLayer.cpp
 *
 *  Created on: 05.08.2009
 *      Author: morgenro
 */

#include "net/TCPConvergenceLayer.h"
#include "core/BundleCore.h"
#include "core/NodeEvent.h"
#include "core/GlobalEvent.h"
#include "ibrcommon/net/NetInterface.h"
#include "net/ConnectionEvent.h"
#include <ibrcommon/thread/MutexLock.h>

#include <sys/socket.h>
#include <streambuf>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <functional>
#include <list>
#include <algorithm>

using namespace ibrcommon;

namespace dtn
{
	namespace net
	{
		/*
		 * class TCPConvergenceLayer
		 */
		const int TCPConvergenceLayer::DEFAULT_PORT = 4556;

		TCPConvergenceLayer::TCPConvergenceLayer(ibrcommon::NetInterface net)
		 : _net(net), _server(net)
		{
		}

		TCPConvergenceLayer::~TCPConvergenceLayer()
		{
		}

		const dtn::core::NodeProtocol TCPConvergenceLayer::getDiscoveryProtocol() const
		{
			return dtn::core::TCP_CONNECTION;
		}

		void TCPConvergenceLayer::initialize()
		{
			_server.initialize();
		}

		void TCPConvergenceLayer::startup()
		{
			_server.startup();
		}

		void TCPConvergenceLayer::terminate()
		{
			_server.terminate();
		}

		void TCPConvergenceLayer::update(std::string& name, std::string& data)
		{
			// TODO: update address and port
		}

		void TCPConvergenceLayer::queue(const dtn::core::Node &n, const ConvergenceLayer::Job &job)
		{
			_server.queue(n, job);
		}

		void TCPConvergenceLayer::Server::queue(const dtn::core::Node &n, const ConvergenceLayer::Job &job)
		{
			{
				// search for an existing connection
				ibrcommon::MutexLock l(_connection_lock);
				for (std::list<TCPConvergenceLayer::TCPConnection*>::iterator iter = _connections.begin(); iter != _connections.end(); iter++)
				{
					TCPConvergenceLayer::TCPConnection *conn = (*iter);

					// do not use the connection if it is marked as "free"
					if (conn->free()) continue;

					if (EID(conn->getNode().getURI()) == job._destination)
					{
						conn->queue(job._bundle);
						return;
					}
				}
			}

			TCPConvergenceLayer::TCPConnection *conn = getConnection(n);
			conn->queue(job._bundle);

			// add the connection to the connection list
			add(conn);
		}

		TCPConvergenceLayer::TCPConnection* TCPConvergenceLayer::Server::getConnection(const dtn::core::Node &n)
		{
			struct sockaddr_in sock_address;
			int sock = socket(AF_INET, SOCK_STREAM, 0);

			if (sock <= 0)
			{
				// error
				throw ibrcommon::tcpserver::SocketException("Could not create a socket.");
			}

			sock_address.sin_family = AF_INET;
			sock_address.sin_addr.s_addr = inet_addr(n.getAddress().c_str());
			sock_address.sin_port = htons(n.getPort());

			if (connect ( sock, (struct sockaddr *) &sock_address, sizeof (sock_address)) != 0)
			{
				// error
				throw ibrcommon::tcpserver::SocketException("Could not connect to the server.");
			}

			// create a connection
			TCPConnection *conn = new TCPConnection(new ibrcommon::tcpstream(sock));

			// raise setup event
			EID eid(n.getURI());
			ConnectionEvent::raise(ConnectionEvent::CONNECTION_SETUP, eid);

			// start the ClientHandler (service)
			conn->initialize(dtn::core::BundleCore::local, 10);

			return conn;
		}

		TCPConvergenceLayer::Server::Server(ibrcommon::NetInterface net)
		 : dtn::net::GenericServer<TCPConvergenceLayer::TCPConnection>(), _tcpsrv(net)
		{
		}

		TCPConvergenceLayer::Server::~Server()
		{

		}

		void TCPConvergenceLayer::Server::connectionUp(TCPConvergenceLayer::TCPConnection *conn)
		{
			ibrcommon::MutexLock l(_connection_lock);
			_connections.push_back(conn);
		}

		void TCPConvergenceLayer::Server::connectionDown(TCPConvergenceLayer::TCPConnection *conn)
		{
			ibrcommon::MutexLock l(_connection_lock);
			for (std::list<TCPConnection*>::iterator iter = _connections.begin(); iter != _connections.end(); iter++)
			{
				if (conn == (*iter))
				{
					_connections.erase(iter);
					return;
				}
			}
		}

		TCPConvergenceLayer::TCPConnection* TCPConvergenceLayer::Server::accept()
		{
			try {
				// wait for incoming connections
				TCPConnection *conn = new TCPConnection(_tcpsrv.accept());
				conn->initialize(dtn::core::BundleCore::local, 10);
				return conn;
			} catch (ibrcommon::tcpserver::SocketException ex) {
				// socket is closed
				return NULL;
			}
		}

		void TCPConvergenceLayer::Server::listen()
		{

		}

		void TCPConvergenceLayer::Server::shutdown()
		{
			shutdownAll();
		}
	}
}

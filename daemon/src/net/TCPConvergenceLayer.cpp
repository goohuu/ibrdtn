/*
 * TCPConvergenceLayer.cpp
 *
 *  Created on: 05.08.2009
 *      Author: morgenro
 */

#include "net/TCPConvergenceLayer.h"
#include "core/BundleCore.h"
#include "core/GlobalEvent.h"
#include "ibrcommon/net/NetInterface.h"
#include "net/ConnectionEvent.h"
#include <ibrcommon/thread/MutexLock.h>
#include "routing/RequeueBundleEvent.h"
#include <ibrcommon/Logger.h>

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

		TCPConvergenceLayer::TCPConvergenceLayer(ibrcommon::NetInterface net, int port)
		 : _net(net), _port(port), _server(net, port)
		{
		}

		TCPConvergenceLayer::~TCPConvergenceLayer()
		{
		}

		dtn::core::NodeProtocol TCPConvergenceLayer::getDiscoveryProtocol() const
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

		void TCPConvergenceLayer::update(std::string &name, std::string &params)
		{
			name = "tcpcl";

			stringstream service; service << "ip=" << _net.getAddress() << ";port=" << _port << ";";
			params = service.str();
		}

		bool TCPConvergenceLayer::onInterface(const ibrcommon::NetInterface &net) const
		{
			if (_net.getInterface() == net.getInterface()) return true;
			return false;
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

				for (std::list<TCPConvergenceLayer::Server::Connection>::iterator iter = _connections.begin(); iter != _connections.end(); iter++)
				{
					TCPConvergenceLayer::Server::Connection &conn = (*iter);

					// do not use the connection if it is marked as "free"
					if ((*conn).free()) continue;

					if (conn.match(job._destination))
					{
						if (!conn._active)
						{
							IBRCOMMON_LOGGER(warning) << "putting bundle on pending connection" << IBRCOMMON_LOGGER_ENDL;
						}

						(*conn).queue(job._bundle);
						return;
					}
				}
			}

			try {
				TCPConvergenceLayer::TCPConnection *conn = getConnection(n);

				// add the connection to the connection list
				add(conn);

				conn->queue(job._bundle);
			} catch (ibrcommon::tcpserver::SocketException ex) {
				// signal interruption of the transfer
				dtn::routing::RequeueBundleEvent::raise(job._destination, job._bundle);
			}
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

			// add connection as pending
			_connections.push_back( Connection( conn, n ) );

			// raise setup event
			EID eid(n.getURI());
			ConnectionEvent::raise(ConnectionEvent::CONNECTION_SETUP, eid);

			// start the ClientHandler (service)
			conn->initialize(dtn::core::BundleCore::local, 10);

			return conn;
		}

		TCPConvergenceLayer::Server::Server(ibrcommon::NetInterface net, int port)
		 : dtn::net::GenericServer<TCPConvergenceLayer::TCPConnection>(), _tcpsrv(net, port)
		{
			bindEvent(NodeEvent::className);
		}

		TCPConvergenceLayer::Server::~Server()
		{
			unbindEvent(NodeEvent::className);
		}

		void TCPConvergenceLayer::Server::raiseEvent(const Event *evt)
		{
			const NodeEvent *node = dynamic_cast<const NodeEvent*>(evt);

			if (node != NULL)
			{
				if (node->getAction() == NODE_UNAVAILABLE)
				{
					// search for an existing connection
					ibrcommon::MutexLock l(_connection_lock);
					for (std::list<TCPConvergenceLayer::Server::Connection>::iterator iter = _connections.begin(); iter != _connections.end(); iter++)
					{
						TCPConvergenceLayer::Server::Connection &conn = (*iter);

						// do not use the connection if it is marked as "free"
						if ( (*conn).free()) continue;

						if (conn.match(*node))
						{
							(*conn).shutdown();
						}
					}
				}
			}
		}

		void TCPConvergenceLayer::Server::connectionUp(TCPConvergenceLayer::TCPConnection *conn)
		{
			ibrcommon::MutexLock l(_connection_lock);

			for (std::list<TCPConvergenceLayer::Server::Connection>::iterator iter = _connections.begin(); iter != _connections.end(); iter++)
			{
				TCPConvergenceLayer::Server::Connection &item = (*iter);

				if (conn == item._connection)
				{
					// put pending connection to the active connections
					item._active = true;
					return;
				}
			}

			_connections.push_back( Connection( conn, conn->getNode(), true ) );
		}

		void TCPConvergenceLayer::Server::connectionDown(TCPConvergenceLayer::TCPConnection *conn)
		{
			ibrcommon::MutexLock l(_connection_lock);
			for (std::list<TCPConvergenceLayer::Server::Connection>::iterator iter = _connections.begin(); iter != _connections.end(); iter++)
			{
				TCPConvergenceLayer::Server::Connection &item = (*iter);

				if (conn == item._connection)
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

				{
					ibrcommon::MutexLock l(_connection_lock);

					// add connection as pending
					_connections.push_back( Connection( conn, conn->getNode() ) );
				}

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

		TCPConvergenceLayer::Server::Connection::Connection(TCPConvergenceLayer::TCPConnection *conn, const dtn::core::Node &node, const bool &active)
		 : _connection(conn), _peer(node), _active(active)
		{

		}

		TCPConvergenceLayer::Server::Connection::~Connection()
		{

		}

		TCPConvergenceLayer::TCPConnection& TCPConvergenceLayer::Server::Connection::operator*()
		{
			return *_connection;
		}

		bool TCPConvergenceLayer::Server::Connection::match(const dtn::data::EID &destination) const
		{
			if (_peer.getURI() == destination.getNodeEID()) return true;
			if (_connection->getNode().getURI() == destination.getNodeEID()) return true;

			return false;
		}

		bool TCPConvergenceLayer::Server::Connection::match(const NodeEvent &evt) const
		{
			const dtn::core::Node &n = evt.getNode();

			if (_peer.getURI() == n.getURI()) return true;
			if (_connection->getNode().getURI() == n.getURI()) return true;

			return false;
		}
	}
}

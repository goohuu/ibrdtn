/*
 * TCPConvergenceLayer.cpp
 *
 *  Created on: 05.08.2009
 *      Author: morgenro
 */

#include "net/TCPConvergenceLayer.h"
#include "core/BundleCore.h"
#include "core/GlobalEvent.h"
#include <ibrcommon/net/vinterface.h>
#include "net/ConnectionEvent.h"
#include <ibrcommon/thread/MutexLock.h>
#include "routing/RequeueBundleEvent.h"
#include <ibrcommon/Logger.h>
#include <ibrcommon/net/tcpclient.h>
#include <streambuf>

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

		TCPConvergenceLayer::TCPConvergenceLayer(const ibrcommon::vinterface &net, int port)
		 : _net(net), _port(port), _server(net, port)
		{
		}

		TCPConvergenceLayer::~TCPConvergenceLayer()
		{
		}

		dtn::core::Node::Protocol TCPConvergenceLayer::getDiscoveryProtocol() const
		{
			return Node::CONN_TCPIP;
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
			stringstream service;

			try {
				std::list<vaddress> list = _net.getAddresses();
				if (!list.empty())
				{
					 service << "ip=" << list.front().get(false) << ";port=" << _port << ";";
				}
				else
				{
					service << "port=" << _port << ";";
				}
			} catch (const ibrcommon::vinterface::interface_not_set&) {
				service << "port=" << _port << ";";
			};

			params = service.str();
		}

		bool TCPConvergenceLayer::onInterface(const ibrcommon::vinterface &net) const
		{
			if (_net == net) return true;
			return false;
		}

		void TCPConvergenceLayer::queue(const dtn::core::Node &n, const ConvergenceLayer::Job &job)
		{
			_server.queue(n, job);
		}

		void TCPConvergenceLayer::open(const dtn::core::Node &n)
		{
			_server.open(n);
		}

		const std::string TCPConvergenceLayer::getName() const
		{
			return "TCPConvergenceLayer";
		}

		void TCPConvergenceLayer::Server::open(const dtn::core::Node &n)
		{
			{
				// search for an existing connection
				ibrcommon::MutexLock l(_lock);

				for (std::list<TCPConvergenceLayer::Server::Connection>::iterator iter = _connections.begin(); iter != _connections.end(); iter++)
				{
					TCPConvergenceLayer::Server::Connection &conn = (*iter);

					if (conn.match(EID(n.getURI())))
					{
						return;
					}
				}
			}

			// create a connection
			TCPConnection *conn = new TCPConnection((GenericServer<TCPConnection>&)*this, n, dtn::core::BundleCore::local, 10);

			// raise setup event
			ConnectionEvent::raise(ConnectionEvent::CONNECTION_SETUP, n);

			ibrcommon::MutexLock l(_lock);

			// add connection as pending
			_connections.push_back( Connection( conn, n ) );

			// add the connection to the connection list
			add(conn);

			// start the ClientHandler (service)
			conn->initialize();
		}

		void TCPConvergenceLayer::Server::queue(const dtn::core::Node &n, const ConvergenceLayer::Job &job)
		{
			{
				// search for an existing connection
				ibrcommon::MutexLock l(_lock);

				for (std::list<TCPConvergenceLayer::Server::Connection>::iterator iter = _connections.begin(); iter != _connections.end(); iter++)
				{
					TCPConvergenceLayer::Server::Connection &conn = (*iter);

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

			// create a connection
			TCPConnection *conn = new TCPConnection((GenericServer<TCPConnection>&)*this, n, dtn::core::BundleCore::local, 10);

			// raise setup event
			ConnectionEvent::raise(ConnectionEvent::CONNECTION_SETUP, n);

			ibrcommon::MutexLock l(_lock);

			// add connection as pending
			_connections.push_back( Connection( conn, n ) );

			// add the connection to the connection list
			add(conn);

			// start the ClientHandler (service)
			conn->initialize();

			conn->queue(job._bundle);
		}

		TCPConvergenceLayer::Server::Server(const ibrcommon::vinterface &net, int port)
		 : dtn::net::GenericServer<TCPConvergenceLayer::TCPConnection>(), _tcpsrv(net, port)
		{
			bindEvent(NodeEvent::className);
		}

		TCPConvergenceLayer::Server::~Server()
		{
			unbindEvent(NodeEvent::className);
			join();
		}

		void TCPConvergenceLayer::Server::raiseEvent(const Event *evt)
		{
			const NodeEvent *node = dynamic_cast<const NodeEvent*>(evt);

			if (node != NULL)
			{
				if (node->getAction() == NODE_UNAVAILABLE)
				{
					// search for an existing connection
					ibrcommon::MutexLock l(_lock);
					for (std::list<TCPConvergenceLayer::Server::Connection>::iterator iter = _connections.begin(); iter != _connections.end(); iter++)
					{
						TCPConvergenceLayer::Server::Connection &conn = (*iter);

						if (conn.match(*node))
						{
							// close the connection immediately
							(*conn).shutdown();
						}
					}
				}
			}
		}

		const std::string TCPConvergenceLayer::Server::getName() const
		{
			return "TCPConvergenceLayer::Server";
		}

		void TCPConvergenceLayer::Server::connectionUp(TCPConvergenceLayer::TCPConnection *conn)
		{
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
			// wait for incoming connections
			TCPConnection *conn = new TCPConnection(*this, _tcpsrv.accept(), dtn::core::BundleCore::local, 10);
			{
				ibrcommon::MutexLock l(_lock);

				// add connection as pending
				_connections.push_back( Connection( conn, conn->getNode() ) );
			}

			return conn;
		}

		void TCPConvergenceLayer::Server::listen()
		{

		}

		void TCPConvergenceLayer::Server::shutdown()
		{
			_tcpsrv.shutdown();
			_tcpsrv.close();
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

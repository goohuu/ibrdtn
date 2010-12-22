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
		 : _net(net), _port(port), _tcpsrv(net, port)
		{
			bindEvent(NodeEvent::className);
		}

		TCPConvergenceLayer::~TCPConvergenceLayer()
		{
			unbindEvent(NodeEvent::className);
			join();
		}

		dtn::core::Node::Protocol TCPConvergenceLayer::getDiscoveryProtocol() const
		{
			return Node::CONN_TCPIP;
		}

		void TCPConvergenceLayer::update(std::string &name, std::string &params)
		{
			name = "tcpcl";
			stringstream service;

			try {
				std::list<vaddress> list = _net.getAddresses(ibrcommon::vaddress::VADDRESS_INET);
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

		const std::string TCPConvergenceLayer::getName() const
		{
			return "TCPConvergenceLayer";
		}

		void TCPConvergenceLayer::open(const dtn::core::Node &n)
		{
			// search for an existing connection
			ibrcommon::MutexLock l(_connections_lock);

			for (std::list<TCPConnection*>::iterator iter = _connections.begin(); iter != _connections.end(); iter++)
			{
				TCPConnection &conn = *(*iter);

				if (conn.match(n))
				{
					return;
				}
			}

			// create a connection
			TCPConnection *conn = new TCPConnection(*this, n, dtn::core::BundleCore::local, 10);

			// raise setup event
			ConnectionEvent::raise(ConnectionEvent::CONNECTION_SETUP, n);

			// add connection as pending
			_connections.push_back( conn );

			// start the ClientHandler (service)
			conn->initialize();

			return;
		}

		void TCPConvergenceLayer::queue(const dtn::core::Node &n, const ConvergenceLayer::Job &job)
		{
			// search for an existing connection
			ibrcommon::MutexLock l(_connections_lock);

			for (std::list<TCPConnection*>::iterator iter = _connections.begin(); iter != _connections.end(); iter++)
			{
				TCPConnection &conn = *(*iter);

				if (conn.match(n))
				{
					conn.queue(job._bundle);
					return;
				}
			}

			// create a connection
			TCPConnection *conn = new TCPConnection(*this, n, dtn::core::BundleCore::local, 10);

			// raise setup event
			ConnectionEvent::raise(ConnectionEvent::CONNECTION_SETUP, n);

			// add connection as pending
			_connections.push_back( conn );

			// start the ClientHandler (service)
			conn->initialize();

			// queue the bundle
			conn->queue(job._bundle);
		}

		void TCPConvergenceLayer::raiseEvent(const Event *evt)
		{
			const NodeEvent *node = dynamic_cast<const NodeEvent*>(evt);

			if (node != NULL)
			{
				if (node->getAction() == NODE_UNAVAILABLE)
				{
					// search for an existing connection
					ibrcommon::MutexLock l(_connections_lock);
					for (std::list<TCPConnection*>::iterator iter = _connections.begin(); iter != _connections.end(); iter++)
					{
						TCPConnection &conn = *(*iter);

						if (conn.match(*node))
						{
							// close the connection immediately
							conn.shutdown();
						}
					}
				}
			}
		}

		void TCPConvergenceLayer::connectionUp(TCPConnection *conn)
		{
			ibrcommon::MutexLock l(_connections_lock);
			for (std::list<TCPConnection*>::iterator iter = _connections.begin(); iter != _connections.end(); iter++)
			{
				if (conn == (*iter))
				{
					// put pending connection to the active connections
					return;
				}
			}

			_connections.push_back( conn );
		}

		void TCPConvergenceLayer::connectionDown(TCPConnection *conn)
		{
			ibrcommon::MutexLock l(_connections_lock);
			for (std::list<TCPConnection*>::iterator iter = _connections.begin(); iter != _connections.end(); iter++)
			{
				if (conn == (*iter))
				{
					_connections.erase(iter);
					return;
				}
			}
		}

		void TCPConvergenceLayer::componentRun()
		{
			// enable interruption in this thread
			Thread::enable_interruption();

			try {
				while (true)
				{
					// wait for incoming connections
					tcpstream *stream = _tcpsrv.accept();

					// create a new TCPConnection and return the pointer
					TCPConnection *obj = new TCPConnection(*this, stream, dtn::core::BundleCore::local, 10);

					// add the connection to the connection list
					connectionUp(obj);

					// initialize the object
					obj->initialize();

					// breakpoint
					ibrcommon::Thread::yield();
				}
			} catch (std::exception) {
				// ignore all errors
				return;
			}
		}

		bool TCPConvergenceLayer::__cancellation()
		{
			_tcpsrv.shutdown();
			_tcpsrv.close();
			Thread::interrupt();
			return true;
		}

		void TCPConvergenceLayer::componentUp()
		{
			// server is listening on constuctor call
		}

		void TCPConvergenceLayer::componentDown()
		{
			while (true)
			{
				TCPConnection* client = NULL;
				{
					ibrcommon::MutexLock l(_connections_lock);
					if (_connections.empty()) break;
					client = _connections.front();
					_connections.remove(client);
				}

				client->shutdown();
			}

			_tcpsrv.shutdown();
			_tcpsrv.close();
			Thread::interrupt();
		}
	}
}

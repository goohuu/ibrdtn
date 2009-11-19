/*
 * TCPConvergenceLayer.cpp
 *
 *  Created on: 05.08.2009
 *      Author: morgenro
 */

#include "net/TCPConvergenceLayer.h"
#include "core/BundleCore.h"
#include "core/EventSwitch.h"
#include "core/NodeEvent.h"
#include "core/GlobalEvent.h"

#include <sys/socket.h>
#include <streambuf>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <functional>
#include <list>
#include <algorithm>

namespace dtn
{
	namespace net
	{
		TCPConvergenceLayer::TCPConnection::TCPConnection(TCPConvergenceLayer &cl, int socket, tcpstream::stream_direction d)
		 : _stream(socket, d), StreamConnection(_stream), _cl(cl), _connected(false)
		{
			_node.setAddress(_stream.getAddress());
			_node.setPort(_stream.getPort());
			_node.setProtocol(TCP_CONNECTION);
			_cl.add(this);

#ifdef DO_EXTENDED_DEBUG_OUTPUT
			cout << "new tcpconnection, " << _node.getAddress() << ":" << _node.getPort() << endl;
#endif

			EventSwitch::registerEventReceiver(GlobalEvent::className, this);
			EventSwitch::registerEventReceiver(NodeEvent::className, this);
		}

		TCPConvergenceLayer::TCPConnection::~TCPConnection()
		{
			BundleConnection::waitFor();
			StreamConnection::waitFor();

			_cl.remove(this);

#ifdef DO_EXTENDED_DEBUG_OUTPUT
			cout << "removed tcpconnection, " << _node.getAddress() << ":" << _node.getPort() << endl;
#endif

			EventSwitch::unregisterEventReceiver(GlobalEvent::className, this);
			EventSwitch::unregisterEventReceiver(NodeEvent::className, this);
		}

		void TCPConvergenceLayer::TCPConnection::embalm()
		{
		}

		void TCPConvergenceLayer::TCPConnection::raiseEvent(const Event *evt)
		{
			static dtn::utils::Mutex mutex;
			dtn::utils::MutexLock l(mutex);

			const GlobalEvent *global = dynamic_cast<const GlobalEvent*>(evt);

			if (global != NULL)
			{
				if (global->getAction() == GlobalEvent::GLOBAL_SHUTDOWN)
				{
					shutdown();
				}
			}

			const NodeEvent *node = dynamic_cast<const NodeEvent*>(evt);

			if (node != NULL)
			{
				if (node->getAction() == NODE_UNAVAILABLE)
				{
					if (node->getNode().equals(_node))
					{
						shutdown();
					}
				}
			}
		}

		const StreamContactHeader TCPConvergenceLayer::TCPConnection::getHeader() const
		{
			return _in_header;
		}

		void TCPConvergenceLayer::TCPConnection::initialize(dtn::streams::StreamContactHeader header)
		{
			_out_header = header;

			// send the header
			(*this) << _out_header;
			StreamConnection::flush();

                        try {
                            // get the header
                            (*this) >> _in_header;
                            _node.setURI(_in_header.getEID().getString());
                            _connected = true;

                            BundleConnection::initialize();
                            StreamConnection::start();

                            // set the timer for this connection
                            StreamConnection::setTimer(header._keepalive, _in_header._keepalive - 5);

                        } catch (dtn::exceptions::InvalidDataException ex) {

                            // return with shutdown, if the stream is wrong
                            (*this) << StreamDataSegment(StreamDataSegment::MSG_SHUTDOWN_VERSION_MISSMATCH); flush();

                            StreamConnection::setState(StreamConnection::CONNECTION_SHUTDOWN);

                            // close the stream
                            _stream.close();

                            bury();
                        }
		}

		void TCPConvergenceLayer::TCPConnection::shutdown()
		{
			BundleConnection::shutdown();
			StreamConnection::shutdown();

                        // wait for the closed connection
                        StreamConnection::waitState(StreamConnection::CONNECTION_CLOSED);

                        // close the stream
			_stream.close();

                        // send myself to the graveyard
			bury();
		}

		const dtn::core::Node& TCPConvergenceLayer::TCPConnection::getNode() const
		{
			return _node;
		}

		bool TCPConvergenceLayer::TCPConnection::isConnected()
		{
			return _connected;
		}

		void TCPConvergenceLayer::TCPConnection::read(dtn::data::Bundle &bundle)
		{
			static dtn::utils::Mutex mutex;
			dtn::utils::MutexLock l(mutex);

			try {
				(*this) >> bundle;
				if (!good()) throw dtn::exceptions::IOException("read from stream failed");
			} catch (dtn::exceptions::InvalidDataException ex) {
				throw dtn::exceptions::IOException("read from stream failed");
			}
		}

		void TCPConvergenceLayer::TCPConnection::write(const dtn::data::Bundle &bundle)
		{
			static dtn::utils::Mutex mutex;
			dtn::utils::MutexLock l(mutex);

			(*this) << bundle; flush();

                        // wait until all ACKs are received or the connection is closed
                        if (!waitCompleted())
                        {
                            // connection is closed before all ACKs are received
                            // if reactive fragmentation is enabled
                            if (_in_header._flags & 0x02)
                            {
                                // then throw a exception to activate the fragmentation process
                                throw BundleConnection::ConnectionInterruptedException(StreamConnection::_ack_size);
                            }
                            else
                            {
                                // if no fragmentation is possible, requeue the bundle
                                throw dtn::exceptions::IOException("write to stream failed");
                            }
                        }

                        StreamConnection::_ack_size = 0;
		}

		const int TCPConvergenceLayer::DEFAULT_PORT = 4556;

		void TCPConvergenceLayer::add(TCPConnection *conn)
		{
			dtn::utils::MutexLock l(_connection_lock);

#ifdef DO_EXTENDED_DEBUG_OUTPUT
			cout << "TCPConvergenceLayer: connection added" << endl;
#endif

			// remove the connection out of the list
			_connections.push_back( conn );
		}

		void TCPConvergenceLayer::remove(TCPConnection *conn)
		{
			dtn::utils::MutexLock l(_connection_lock);

#ifdef DO_EXTENDED_DEBUG_OUTPUT
			cout << "TCPConvergenceLayer: connection removed" << endl;
#endif

			// remove the connection out of the list
			_connections.remove( conn );
		}

		TCPConvergenceLayer::TCPConvergenceLayer(string bind_addr, unsigned short port)
		 : tcpserver(bind_addr, port), _running(true), _header(dtn::core::BundleCore::local)
		{
			_header._keepalive = 10;
		}

		TCPConvergenceLayer::~TCPConvergenceLayer()
		{
			// stop the tcpserver
			tcpserver::close();

			// wait until all connections are closed
			while (!_connections.empty()) { usleep(100); }

			_running = false;
			join();
		}

                void TCPConvergenceLayer::update(std::string& name, std::string& data)
                {
                    // TODO: update address and port
                }

		TCPConvergenceLayer::TCPConnection* TCPConvergenceLayer::openConnection(const dtn::core::Node &n)
		{
			struct sockaddr_in sock_address;
			int sock = socket(AF_INET, SOCK_STREAM, 0);

			if (sock <= 0)
			{
				// error
				throw SocketException("Could not create a socket.");
			}

			sock_address.sin_family = AF_INET;
			sock_address.sin_addr.s_addr = inet_addr(n.getAddress().c_str());
			sock_address.sin_port = htons(n.getPort());

			if (connect ( sock, (struct sockaddr *) &sock_address, sizeof (sock_address)) != 0)
			{
				// error
				throw SocketException("Could not connect to the server.");
			}

			// create a connection
			TCPConnection *conn = new TCPConnection(*this, sock, tcpstream::STREAM_OUTGOING);

			// start the ClientHandler (service)
			conn->initialize(_header);

			return conn;
		}

		// search a matching Connection or create a new one
		BundleConnection* TCPConvergenceLayer::getConnection(const dtn::core::Node &n)
		{
			if (n.getProtocol() != TCP_CONNECTION) return NULL;

			_connection_lock.enter();
			std::list<TCPConnection*>::iterator iter = _connections.begin();

			EID node_eid(n.getURI());

#ifdef DO_EXTENDED_DEBUG_OUTPUT
			cout << "search for '" << node_eid.getString() << "' in active connections" << endl;
#endif

			while (iter != _connections.end())
			{
				if ( (*iter)->isConnected() )
				{
#ifdef DO_EXTENDED_DEBUG_OUTPUT
					cout << "check node '" << (*iter)->getNode().getURI() << "'" << endl;
#endif
					EID eid((*iter)->getNode().getURI());

					if (eid == node_eid)
					{
						break;
					}
				}

				iter++;
			}
			_connection_lock.leave();

			if (iter == _connections.end())
			{
				return openConnection(n);
			}

			if ( (*iter)->good() )
			{
				return (*iter);
			}

			return openConnection(n);
		}

		void TCPConvergenceLayer::run()
		{
			while (_running)
			{
				// wait for incoming connections
				if (waitPending())
				{
					// create a new Connection
					TCPConnection *conn = new TCPConnection(*this, accept(), tcpstream::STREAM_INCOMING);

					// start the ClientHandler (service)
					conn->initialize(_header);
				}

				// breakpoint to stop this thread
				yield();
			}
		}
	}
}

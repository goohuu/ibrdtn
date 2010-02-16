/*
 * TCPConvergenceLayer.cpp
 *
 *  Created on: 05.08.2009
 *      Author: morgenro
 */

#include "net/TCPConvergenceLayer.h"
#include "core/BundleCore.h"
#include "net/BundleReceivedEvent.h"
#include "core/NodeEvent.h"
#include "core/GlobalEvent.h"
#include "ibrcommon/net/NetInterface.h"
#include "net/ConnectionEvent.h"

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
		 * class TCPBundleStream
		 */
		TCPConvergenceLayer::TCPConnection::TCPBundleStream::TCPBundleStream(TCPConnection &conn, int socket, ibrcommon::tcpstream::stream_direction d)
		 : _conn(conn), _stream(socket, d), StreamConnection(_stream), _reactive_fragmentation(false)
		{
			_node.setAddress(_stream.getAddress());
			_node.setPort(_stream.getPort());
			_node.setProtocol(TCP_CONNECTION);

#ifdef DO_EXTENDED_DEBUG_OUTPUT
			cout << "new tcpconnection, " << _node.getAddress() << ":" << _node.getPort() << endl;
#endif
		}

		TCPConvergenceLayer::TCPConnection::TCPBundleStream::~TCPBundleStream()
		{
#ifdef DO_EXTENDED_DEBUG_OUTPUT
			cout << "removed tcpconnection, " << _node.getAddress() << ":" << _node.getPort() << endl;
#endif
		}

		const dtn::core::Node& TCPConvergenceLayer::TCPConnection::TCPBundleStream::getNode() const
		{
			return _node;
		}

		void TCPConvergenceLayer::TCPConnection::TCPBundleStream::shutdown()
		{
			// call underlaying shutdown
			StreamConnection::shutdown();

			// close the stream
			_stream.close();

			// wait for the closed connection
			StreamConnection::waitState(StreamConnection::CONNECTION_CLOSED);

			_conn.shutdown();
		}

		bool TCPConvergenceLayer::TCPConnection::TCPBundleStream::waitCompleted()
		{
			// wait until all ACKs are received or the connection is closed
			if (!StreamConnection::waitCompleted())
			{
				// connection is closed before all ACKs are received
				// if reactive fragmentation is enabled
				if (_reactive_fragmentation)
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

			return true;
		}

		void TCPConvergenceLayer::TCPConnection::TCPBundleStream::handshake(dtn::streams::StreamContactHeader &in, dtn::streams::StreamContactHeader &out)
		{
			// send the header
			(*this) << out; (*this).flush();

			try {
				// get the header
				(*this) >> in;
				_node.setURI(in.getEID().getString());

				// raise Event
				ConnectionEvent::raise(ConnectionEvent::CONNECTION_UP, in._localeid);

				// startup the connection threads
				StreamConnection::start();

				// set the timer for this connection
				StreamConnection::setTimer(out._keepalive, in._keepalive - 5);

				// check fragmentation flag
				_reactive_fragmentation = (in._flags & 0x02);

			} catch (dtn::exceptions::InvalidDataException ex) {

				// return with shutdown, if the stream is wrong
				_stream << StreamDataSegment(StreamDataSegment::MSG_SHUTDOWN_VERSION_MISSMATCH); flush();

				StreamConnection::setState(StreamConnection::CONNECTION_SHUTDOWN);

				// close the stream
				_stream.close();

				// forward the catched exception
				throw ex;
			}
		}

		void TCPConvergenceLayer::TCPConnection::TCPBundleStream::eventTimeout()
		{
			// close the stream
			_stream.close();

			// wait for the closed connection
			StreamConnection::waitState(StreamConnection::CONNECTION_CLOSED);

			// signal timeout
			_conn.eventTimeout();
		}

		void TCPConvergenceLayer::TCPConnection::TCPBundleStream::eventShutdown()
		{
			// close the stream
			_stream.close();

			// wait for the closed connection
			StreamConnection::waitState(StreamConnection::CONNECTION_CLOSED);

			// signal timeout
			_conn.eventShutdown();
		}

		/*
		 * class TCPConnection
		 */
		TCPConvergenceLayer::TCPConnection::TCPConnection(TCPConvergenceLayer &cl, int socket, ibrcommon::tcpstream::stream_direction d)
		 : _stream(*this, socket, d), _cl(cl), _receiver(*this), _connected(false)
		{
			_cl.add(this);

			bindEvent(GlobalEvent::className);
			bindEvent(NodeEvent::className);
		}

		TCPConvergenceLayer::TCPConnection::~TCPConnection()
		{
			_receiver.waitFor();
			_stream.waitFor();

			_cl.remove(this);

			unbindEvent(GlobalEvent::className);
			unbindEvent(NodeEvent::className);
		}

		void TCPConvergenceLayer::TCPConnection::embalm()
		{
		}

		void TCPConvergenceLayer::TCPConnection::raiseEvent(const Event *evt)
		{
			static ibrcommon::Mutex mutex;
			ibrcommon::MutexLock l(mutex);

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
					if (node->getNode().equals(getNode()))
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
			try {
				// define the outgoing header
				_out_header = header;

				// do the handshake
				_stream.handshake(_in_header, _out_header);

				// set state to connected
				_connected = true;

				// start the receiver for incoming bundles
				_receiver.start();

			} catch (dtn::exceptions::InvalidDataException ex) {
				// mark up for deletion
				bury();
			}

		}

		void TCPConvergenceLayer::TCPConnection::eventShutdown()
		{
			// stop the receiver
			_receiver.shutdown();

			// event
			ConnectionEvent::raise(ConnectionEvent::CONNECTION_DOWN, _in_header._localeid);

			// send myself to the graveyard
			bury();
		}

		void TCPConvergenceLayer::TCPConnection::eventTimeout()
		{
			// stop the receiver
			_receiver.shutdown();

			// event
			ConnectionEvent::raise(ConnectionEvent::CONNECTION_TIMEOUT, _in_header._localeid);

			// send myself to the graveyard
			bury();
		}

		void TCPConvergenceLayer::TCPConnection::shutdown()
		{
			// stop the receiver
			_receiver.shutdown();

			// disconnect
			_stream.shutdown();

			// send myself to the graveyard
			bury();
		}

		const dtn::core::Node& TCPConvergenceLayer::TCPConnection::getNode() const
		{
			return _stream.getNode();
		}

		bool TCPConvergenceLayer::TCPConnection::isConnected()
		{
			return _connected;
		}

		bool TCPConvergenceLayer::TCPConnection::isBusy() const
		{
			return _busy;
		}

		void TCPConvergenceLayer::TCPConnection::read(dtn::data::Bundle &bundle)
		{
			ibrcommon::MutexLock l(_readlock);

			try {
				_stream >> bundle;
				if (!_stream.good()) throw dtn::exceptions::IOException("read from stream failed");
			} catch (dtn::exceptions::InvalidDataException ex) {
				throw dtn::exceptions::IOException("read from stream failed");
			} catch (dtn::exceptions::InvalidBundleData ex) {
				throw dtn::exceptions::IOException("Bundle data invalid");
			}
		}

		void TCPConvergenceLayer::TCPConnection::write(const dtn::data::Bundle &bundle)
		{
			static ibrcommon::Mutex mutex;
			ibrcommon::IndicatingLock l(mutex, _busy);

			// transmit the bundle and flush
			_stream << bundle; _stream.flush();

			// wait until all segments are acknowledged.
			_stream.waitCompleted();
		}

		TCPConvergenceLayer::TCPConnection::Receiver::Receiver(TCPConnection &connection)
		 :  _running(true), _connection(connection)
		{
		}

		TCPConvergenceLayer::TCPConnection::Receiver::~Receiver()
		{
			join();
		}

		void TCPConvergenceLayer::TCPConnection::Receiver::run()
		{
			try {
				while (_running)
				{
					dtn::data::Bundle bundle;
					_connection.read(bundle);

					// raise default bundle received event
					dtn::net::BundleReceivedEvent::raise(EID(), bundle);

					yield();
				}
			} catch (dtn::exceptions::IOException ex) {
				_running = false;
			}
		}

		void TCPConvergenceLayer::TCPConnection::Receiver::shutdown()
		{
			_running = false;
		}

		/*
		 * class TCPConvergenceLayer
		 */
		const int TCPConvergenceLayer::DEFAULT_PORT = 4556;

		void TCPConvergenceLayer::add(TCPConnection *conn)
		{
			ibrcommon::MutexLock l(_connection_lock);

#ifdef DO_EXTENDED_DEBUG_OUTPUT
			cout << "TCPConvergenceLayer: connection added" << endl;
#endif

			// remove the connection out of the list
			_connections.push_back( conn );
		}

		void TCPConvergenceLayer::remove(TCPConnection *conn)
		{
			ibrcommon::MutexLock l(_connection_lock);

#ifdef DO_EXTENDED_DEBUG_OUTPUT
			cout << "TCPConvergenceLayer: connection removed" << endl;
#endif

			// remove the connection out of the list
			_connections.remove( conn );
		}

		TCPConvergenceLayer::TCPConvergenceLayer(ibrcommon::NetInterface net)
		 : _net(net), tcpserver(net), _running(true), _header(dtn::core::BundleCore::local)
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
			TCPConnection *conn = new TCPConnection(*this, sock, ibrcommon::tcpstream::STREAM_OUTGOING);

			// start the ClientHandler (service)
			conn->initialize(_header);

			return conn;
		}

		// search a matching Connection or create a new one
		BundleConnection* TCPConvergenceLayer::getConnection(const dtn::core::Node &n)
		{
			if (n.getProtocol() != TCP_CONNECTION) return NULL;

			// Lock the connection in this section
			{
				ibrcommon::MutexLock l(_connection_lock);

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
							if ( (*iter)->isConnected() )
							{
								return (*iter);
							}
						}
					}

					iter++;
				}
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
					TCPConnection *conn = new TCPConnection(*this, accept(), ibrcommon::tcpstream::STREAM_INCOMING);

					// start the ClientHandler (service)
					conn->initialize(_header);
				}

				// breakpoint to stop this thread
				yield();
			}
		}
	}
}

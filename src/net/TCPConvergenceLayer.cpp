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
//		/*
//		 * class TCPBundleStream
//		 */
//		TCPConvergenceLayer::TCPConnection::TCPBundleStream::TCPBundleStream(TCPConnection &conn, ibrcommon::tcpstream *stream)
//		 : _conn(conn), _stream(stream), StreamConnection(*_stream), _reactive_fragmentation(false)
//		{
//			_node.setAddress(_stream->getAddress());
//			_node.setPort(_stream->getPort());
//			_node.setProtocol(TCP_CONNECTION);
//
//#ifdef DO_EXTENDED_DEBUG_OUTPUT
//			cout << "new tcpconnection, " << _node.getAddress() << ":" << _node.getPort() << endl;
//#endif
//		}

//		TCPConvergenceLayer::TCPConnection::TCPBundleStream::~TCPBundleStream()
//		{
//#ifdef DO_EXTENDED_DEBUG_OUTPUT
//			cout << "removed tcpconnection, " << _node.getAddress() << ":" << _node.getPort() << endl;
//#endif
//		}
//
//		const dtn::core::Node& TCPConvergenceLayer::TCPConnection::TCPBundleStream::getNode() const
//		{
//			return _node;
//		}


//		void TCPConvergenceLayer::TCPConnection::TCPBundleStream::handshake(dtn::streams::StreamContactHeader &in, dtn::streams::StreamContactHeader &out)
//		{
//			// send the header
//			(*this) << out << std::flush;
//
//			try {
//				// get the header
//				(*this) >> in;
//				_node.setURI(in.getEID().getString());
//
//				// raise Event
//				ConnectionEvent::raise(ConnectionEvent::CONNECTION_UP, in._localeid, &_conn);
//
//				// startup the connection threads
//				StreamConnection::start();
//
//				// set the timer for this connection
//				StreamConnection::setTimer(out._keepalive, in._keepalive - 5);
//
//				// check fragmentation flag
//				_reactive_fragmentation = (in._flags & 0x02);
//
//			} catch (dtn::exceptions::InvalidDataException ex) {
//
//				// return with shutdown, if the stream is wrong
//				(*_stream) << StreamDataSegment(StreamDataSegment::MSG_SHUTDOWN_VERSION_MISSMATCH) << std::flush;
//
//				//StreamConnection::setState(StreamConnection::CONNECTION_SHUTDOWN);
//				StreamConnection::shutdown();
//
//				// close the stream
//				_stream->close();
//
//				// forward the catched exception
//				throw ex;
//			}
//		}

//		void TCPConvergenceLayer::TCPConnection::TCPBundleStream::eventTimeout()
//		{
//			// close the stream
//			_stream->close();
//
//			// wait for the closed connection
//			StreamConnection::waitClosed();
//
//			// signal timeout
//			_conn.eventTimeout();
//		}
//
//		void TCPConvergenceLayer::TCPConnection::TCPBundleStream::eventShutdown()
//		{
//			// close the stream
//			_stream->close();
//
//			// wait for the closed connection
//			StreamConnection::waitClosed();
//
//			// signal timeout
//			_conn.eventShutdown();
//		}

		/*
		 * class TCPConnection
		 */
		TCPConvergenceLayer::TCPConnection::TCPConnection(TCPConvergenceLayer &cl, ibrcommon::tcpstream *stream)
		 : _tcpstream(stream), _stream(*this, *_tcpstream), _cl(cl), _receiver(*this)
		{
			_cl.add(this);

			bindEvent(GlobalEvent::className);
			bindEvent(NodeEvent::className);
		}

		TCPConvergenceLayer::TCPConnection::~TCPConnection()
		{
			unbindEvent(GlobalEvent::className);
			unbindEvent(NodeEvent::className);

			// wait until all data is received
			_stream.wait();

			// stop the receiver
			_receiver.shutdown();

			// disconnect
			_stream.close();

			_cl.remove(this);

			// delete the tcpstream
			delete _tcpstream;
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
			return _peer;
		}

		void TCPConvergenceLayer::TCPConnection::initialize(const dtn::data::EID &name, const size_t timeout)
		{
			try {
				// do the handshake
				_stream.handshake(name, timeout);

				// start the receiver for incoming bundles
				_receiver.start();

			} catch (dtn::exceptions::InvalidDataException ex) {
				// mark up for deletion
				bury();
			}
		}

		void TCPConvergenceLayer::TCPConnection::eventConnectionUp(const StreamContactHeader &header)
		{
			_node.setURI(header._localeid.getString());
		}

		void TCPConvergenceLayer::TCPConnection::eventShutdown()
		{
			// stop the receiver
			_receiver.shutdown();

			// event
			ConnectionEvent::raise(ConnectionEvent::CONNECTION_DOWN, _peer._localeid, this);

			// send myself to the graveyard
			bury();
		}

		void TCPConvergenceLayer::TCPConnection::eventTimeout()
		{
			// stop the receiver
			_receiver.shutdown();

			// event
			ConnectionEvent::raise(ConnectionEvent::CONNECTION_TIMEOUT, _peer._localeid, this);

			// send myself to the graveyard
			bury();
		}

		void TCPConvergenceLayer::TCPConnection::shutdown()
		{
			// wait until the stream is closed
			_stream.wait();

			// disconnect
			_stream.close();

			// stop the receiver
			_receiver.shutdown();

			// send myself to the graveyard
			bury();
		}

		const dtn::core::Node& TCPConvergenceLayer::TCPConnection::getNode() const
		{
			return _node;
		}

		bool TCPConvergenceLayer::TCPConnection::isConnected()
		{
			return _stream.isConnected();
		}

		bool TCPConvergenceLayer::TCPConnection::isBusy() const
		{
			return _busy;
		}

		void TCPConvergenceLayer::TCPConnection::read(dtn::data::Bundle &bundle)
		{
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
			ibrcommon::IndicatingLock l(_busymutex, _busy);

			// transmit the bundle and flush
			_stream << bundle << std::flush;

			// wait until all segments are acknowledged.
			_stream.wait();
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
		 : _net(net), tcpserver(net), _running(true)
		{
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

		BundleConnection* TCPConvergenceLayer::getConnection(const dtn::core::Node &n)
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
			TCPConnection *conn = new TCPConnection(*this, new ibrcommon::tcpstream(sock));

			// raise setup event
			EID eid(n.getURI());
			ConnectionEvent::raise(ConnectionEvent::CONNECTION_SETUP, eid, conn);

			// start the ClientHandler (service)
			conn->initialize(dtn::core::BundleCore::local, 10);

			return conn;
		}

		void TCPConvergenceLayer::run()
		{
			while (_running)
			{
				// wait for incoming connections
				if (waitPending())
				{
					// create a new Connection
					TCPConnection *conn = new TCPConnection(*this, accept());

					// start the ClientHandler (service)
					conn->initialize(dtn::core::BundleCore::local, 10);
				}

				// breakpoint to stop this thread
				yield();
			}
		}
	}
}

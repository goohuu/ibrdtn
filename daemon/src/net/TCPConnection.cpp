/*
 * TCPConnection.cpp
 *
 *  Created on: 26.04.2010
 *      Author: morgenro
 */

#include "net/TCPConvergenceLayer.h"
#include "net/BundleReceivedEvent.h"
#include "core/BundleCore.h"
#include "core/NodeEvent.h"
#include "core/GlobalEvent.h"
#include "ibrcommon/net/NetInterface.h"
#include "net/ConnectionEvent.h"
#include "ibrcommon/TimeMeasurement.h"

#include "routing/RequeueBundleEvent.h"
#include "net/TransferCompletedEvent.h"

#include <iostream>
#include <iomanip>

namespace dtn
{
	namespace net
	{
		/*
		 * class TCPConnection
		 */
		TCPConvergenceLayer::TCPConnection::TCPConnection(ibrcommon::tcpstream *stream)
		 : _free(false), _tcpstream(stream), _stream(*this, *stream), _receiver(*this), _sender(*this)
		{
			bindEvent(GlobalEvent::className);
			bindEvent(NodeEvent::className);
		}

		TCPConvergenceLayer::TCPConnection::~TCPConnection()
		{
			shutdown();
			std::cout << "TCPConnection deleted" << std::endl;
		}

		void TCPConvergenceLayer::TCPConnection::iamfree()
		{
			unbindEvent(GlobalEvent::className);
			unbindEvent(NodeEvent::className);
			_free = true;
		}

		bool TCPConvergenceLayer::TCPConnection::free()
		{
			ibrcommon::MutexLock l(_freemutex);
			return _free;
		}

		void TCPConvergenceLayer::TCPConnection::queue(const dtn::data::Bundle &bundle)
		{
			ibrcommon::MutexLock l(_queue_lock);
			_bundles.push(bundle);
			_queue_lock.signal();
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
			ibrcommon::MutexLock l(_freemutex);

			try {
				// do the handshake
				_stream.handshake(name, timeout);

				// start the receiver for incoming bundles
				_receiver.start();

				// start the sender for outgoing bundles
				_sender.start();

			} catch (dtn::exceptions::InvalidDataException ex) {
				// mark up for deletion
				iamfree();
			}
		}

		void TCPConvergenceLayer::TCPConnection::eventConnectionUp(const StreamContactHeader &header)
		{
			_peer = header;
			_node.setURI(header._localeid.getString());

			// raise up event
			ConnectionEvent::raise(ConnectionEvent::CONNECTION_UP, header.getEID());
		}

		void TCPConvergenceLayer::TCPConnection::eventShutdown()
		{
			// stop the receiver
			_receiver.shutdown();

			// stop the sender
			_sender.shutdown();

			// event
			ConnectionEvent::raise(ConnectionEvent::CONNECTION_DOWN, _peer._localeid);

			// close the tcpstream
			try {
				_tcpstream->close();
			} catch (ibrcommon::ConnectionClosedException ex) {

			}

			// send myself to the graveyard
			iamfree();
		}

		void TCPConvergenceLayer::TCPConnection::eventTimeout()
		{
			// stop the receiver
			_receiver.shutdown();

			// stop the sender
			_sender.shutdown();

			// event
			ConnectionEvent::raise(ConnectionEvent::CONNECTION_TIMEOUT, _peer._localeid);

			// close the tcpstream
			try {
				_tcpstream->close();
			} catch (ibrcommon::ConnectionClosedException ex) {

			}

			// send myself to the graveyard
			ibrcommon::MutexLock l(_freemutex);
			iamfree();
		}

		void TCPConvergenceLayer::TCPConnection::shutdown()
		{
			// wait until all data is acknowledged
			_stream.wait();

			// disconnect
			_stream.close();
		}

		const dtn::core::Node& TCPConvergenceLayer::TCPConnection::getNode() const
		{
			return _node;
		}

		TCPConvergenceLayer::TCPConnection& operator>>(TCPConvergenceLayer::TCPConnection &conn, dtn::data::Bundle &bundle)
		{
			ibrcommon::MutexLock l(conn._freemutex);

			conn._stream >> bundle;
			if (!conn._stream.good()) throw dtn::exceptions::IOException("read from stream failed");
		}

		TCPConvergenceLayer::TCPConnection& operator<<(TCPConvergenceLayer::TCPConnection &conn, const dtn::data::Bundle &bundle)
		{
			ibrcommon::MutexLock l(conn._freemutex);

			// prepare a measurement
			ibrcommon::TimeMeasurement m;

			try {
				// start the measurement
				m.start();

				// transmit the bundle
				conn._stream << bundle << std::flush;

				// stop the time measurement
				m.stop();

				// get throughput
				double kbytes_per_second = (bundle.getSize() / m.getSeconds()) / 1024;

				// print out throughput
				ibrcommon::slog << ibrcommon::SYSLOG_DEBUG << "transfer completed after " << m << " with "
						<< std::setiosflags(std::ios::fixed) << std::setprecision(2) << kbytes_per_second << " kb/s" << endl;

				// wait until all segments are acknowledged.
				conn._stream.wait();

				// signal completion of the transfer
				TransferCompletedEvent::raise(EID(conn._node.getURI()), bundle);

			} catch (ConnectionInterruptedException ex) {
				// stop the time measurement
				m.stop();

				// get throughput
				double kbytes_per_second = (ex.getLastAck() / m.getSeconds()) / 1024;

				// print out throughput
				ibrcommon::slog << ibrcommon::SYSLOG_DEBUG << "transfer interrupted after " << m << " with "
						<< std::setiosflags(std::ios::fixed) << std::setprecision(2) << kbytes_per_second << " kb/s" << endl;

				// TODO: the connection has been interrupted => create a fragment

				// signal interruption of the transfer
				dtn::routing::RequeueBundleEvent::raise(EID(conn._node.getURI()), bundle);

			} catch (dtn::exceptions::IOException ex) {
				// stop the time measurement
				m.stop();

				// the connection has been terminated and fragmentation is not possible => requeue the bundle

#ifdef DO_DEBUG_OUTPUT
				ibrcommon::slog << ibrcommon::SYSLOG_DEBUG << "fragmentation is not possible => requeue the bundle" << endl;
				ibrcommon::slog << ibrcommon::SYSLOG_DEBUG << "Exception: " << ex.what() << endl;
#endif
				// signal interruption of the transfer
				dtn::routing::RequeueBundleEvent::raise(EID(conn._node.getURI()), bundle);

			} catch (dtn::net::ConnectionNotAvailableException ex) {
				// the connection not available

#ifdef DO_DEBUG_OUTPUT
				ibrcommon::slog << ibrcommon::SYSLOG_DEBUG << "connection error => requeue the bundle" << endl;
				ibrcommon::slog << ibrcommon::SYSLOG_DEBUG << "Exception: " << ex.what() << endl;
#endif
				// signal interruption of the transfer
				dtn::routing::RequeueBundleEvent::raise(EID(conn._node.getURI()), bundle);

			}
		}

		TCPConvergenceLayer::TCPConnection::Receiver::Receiver(TCPConnection &connection)
		 :  _running(true), _connection(connection)
		{
		}

		TCPConvergenceLayer::TCPConnection::Receiver::~Receiver()
		{
			shutdown();
			join();
		}

		void TCPConvergenceLayer::TCPConnection::Receiver::run()
		{
			try {
				while (_running)
				{
					dtn::data::Bundle bundle;
					_connection >> bundle;

					// raise default bundle received event
					dtn::net::BundleReceivedEvent::raise(EID(), bundle);

					yield();
				}
			} catch (dtn::exceptions::IOException ex) {
				_running = false;
			} catch (dtn::exceptions::InvalidDataException ex) {
				_running = false;
			} catch (dtn::exceptions::InvalidBundleData ex) {
				_running = false;
			}
		}

		void TCPConvergenceLayer::TCPConnection::Receiver::shutdown()
		{
			_running = false;
		}

		TCPConvergenceLayer::TCPConnection::Sender::Sender(TCPConnection &connection)
		 :  _running(true), _connection(connection)
		{
		}

		TCPConvergenceLayer::TCPConnection::Sender::~Sender()
		{
			shutdown();
			join();
		}

		void TCPConvergenceLayer::TCPConnection::Sender::run()
		{
			try {
				while (_running)
				{
					dtn::data::Bundle bundle;
					{
						// get queued bundle
						ibrcommon::MutexLock l(_connection._queue_lock);

						while (_connection._bundles.empty())
						{
							_connection._queue_lock.wait();
							if (!_running) return;
						}

						bundle = _connection._bundles.front();
						_connection._bundles.pop();
					}

					// send bundle
					_connection << bundle;

					// idle a little bit
					yield();
				}
			} catch (dtn::exceptions::IOException ex) {
				ibrcommon::MutexLock l(_connection._queue_lock);
				_running = false;
			} catch (dtn::exceptions::InvalidDataException ex) {
				ibrcommon::MutexLock l(_connection._queue_lock);
				_running = false;
			} catch (dtn::exceptions::InvalidBundleData ex) {
				ibrcommon::MutexLock l(_connection._queue_lock);
				_running = false;
			}
		}

		void TCPConvergenceLayer::TCPConnection::Sender::shutdown()
		{
			ibrcommon::MutexLock l(_connection._queue_lock);
			_running = false;
			_connection._queue_lock.signal();
		}
	}
}

/*
 * TCPConnection.cpp
 *
 *  Created on: 26.04.2010
 *      Author: morgenro
 */

#include "net/TCPConvergenceLayer.h"
#include "net/BundleReceivedEvent.h"
#include "core/BundleCore.h"
#include "core/GlobalEvent.h"
#include "net/ConnectionEvent.h"
#include "core/BundleEvent.h"
#include "ibrcommon/TimeMeasurement.h"

#include "net/TransferCompletedEvent.h"
#include "net/TransferAbortedEvent.h"

#include <ibrcommon/net/NetInterface.h>
#include <ibrcommon/Logger.h>

#include <iostream>
#include <iomanip>

namespace dtn
{
	namespace net
	{
		/*
		 * class TCPConnection
		 */
		TCPConvergenceLayer::TCPConnection::TCPConnection(GenericServer<TCPConnection> &tcpsrv, ibrcommon::tcpstream *stream, const dtn::data::EID &name, const size_t timeout)
		 : GenericConnection<TCPConvergenceLayer::TCPConnection>((GenericServer<TCPConvergenceLayer::TCPConnection>&)tcpsrv),
		   _peer(), _node(Node::NODE_CONNECTED), _tcpstream(stream), _stream(*this, *stream), _sender(*this),
		   _name(name), _timeout(timeout), _lastack(0)
		{
			stream->enableKeepalive();
			stream->enableNoDelay();
			_node.setProtocol(Node::CONN_TCPIP);
		}

		TCPConvergenceLayer::TCPConnection::~TCPConnection()
		{
		}

		void TCPConvergenceLayer::TCPConnection::queue(const dtn::data::Bundle &bundle)
		{
			_sender.push(bundle);
		}

		const StreamContactHeader TCPConvergenceLayer::TCPConnection::getHeader() const
		{
			return _peer;
		}

		void TCPConvergenceLayer::TCPConnection::handshake()
		{
			char flags = 0;

			// enable ACKs and NACKs
			flags |= dtn::streams::StreamContactHeader::REQUEST_ACKNOWLEDGMENTS;
			flags |= dtn::streams::StreamContactHeader::REQUEST_NEGATIVE_ACKNOWLEDGMENTS;

			// do the handshake
			_stream.handshake(_name, _timeout, flags);
		}

		void TCPConvergenceLayer::TCPConnection::initialize()
		{
			// start the receiver for incoming bundles + handshake
			try {
				start();
			} catch (const ibrcommon::ThreadException &ex) {
				IBRCOMMON_LOGGER(error) << "failed to start thread in TCPConnection\n" << ex.what() << IBRCOMMON_LOGGER_ENDL;
			}
		}

		void TCPConvergenceLayer::TCPConnection::eventConnectionUp(const StreamContactHeader &header)
		{
			_peer = header;
			_node.setURI(header._localeid.getString());

			// raise up event
			ConnectionEvent::raise(ConnectionEvent::CONNECTION_UP, _node);
		}

		void TCPConvergenceLayer::TCPConnection::eventConnectionDown()
		{
			IBRCOMMON_LOGGER_DEBUG(40) << "TCPConnection::eventConnectionDown()" << IBRCOMMON_LOGGER_ENDL;

			if (_peer._localeid != dtn::data::EID())
			{
				// event
				ConnectionEvent::raise(ConnectionEvent::CONNECTION_DOWN, _node);
			}

			// stop the sender
			_sender.shutdown();
		}

		void TCPConvergenceLayer::TCPConnection::eventBundleRefused()
		{
			try {
				const dtn::data::Bundle bundle = _sentqueue.getnpop();

				// requeue the bundle
				TransferAbortedEvent::raise(EID(_node.getURI()), bundle, dtn::net::TransferAbortedEvent::REASON_REFUSED);

				// set ACK to zero
				_lastack = 0;

			} catch (ibrcommon::QueueUnblockedException) {
				// pop on empty queue!
			}
		}

		void TCPConvergenceLayer::TCPConnection::eventBundleForwarded()
		{
			try {
				const dtn::data::Bundle bundle = _sentqueue.getnpop();

				// signal completion of the transfer
				TransferCompletedEvent::raise(EID(_node.getURI()), bundle);

				// raise bundle event
				dtn::core::BundleEvent::raise(bundle, BUNDLE_FORWARDED);

				// set ACK to zero
				_lastack = 0;
			} catch (ibrcommon::QueueUnblockedException) {
				// pop on empty queue!
			}
		}

		void TCPConvergenceLayer::TCPConnection::eventBundleAck(size_t ack)
		{
			_lastack = ack;
		}

		void TCPConvergenceLayer::TCPConnection::eventShutdown()
		{
		}

		void TCPConvergenceLayer::TCPConnection::eventTimeout()
		{
			// event
			ConnectionEvent::raise(ConnectionEvent::CONNECTION_TIMEOUT, _node);
		}

		void TCPConvergenceLayer::TCPConnection::eventError()
		{
		}

		void TCPConvergenceLayer::TCPConnection::shutdown()
		{
			// shutdown
			_stream.shutdown(StreamConnection::CONNECTION_SHUTDOWN_ERROR);
		}

		const dtn::core::Node& TCPConvergenceLayer::TCPConnection::getNode() const
		{
			return _node;
		}

		void TCPConvergenceLayer::TCPConnection::rejectTransmission()
		{
			_stream.reject();
		}

		TCPConvergenceLayer::TCPConnection& operator>>(TCPConvergenceLayer::TCPConnection &conn, dtn::data::Bundle &bundle)
		{
			dtn::data::DefaultDeserializer(conn._stream, dtn::core::BundleCore::getInstance()) >> bundle;
			return conn;
		}

		TCPConvergenceLayer::TCPConnection& operator<<(TCPConvergenceLayer::TCPConnection &conn, const dtn::data::Bundle &bundle)
		{
			// prepare a measurement
			ibrcommon::TimeMeasurement m;

			// create a serializer
			dtn::data::DefaultSerializer serializer(conn._stream);

			// put the bundle into the sentqueue
			conn._sentqueue.push(bundle);

			// start the measurement
			m.start();

			try {
				// transmit the bundle
				serializer << bundle;

				// flush the stream
				conn._stream << std::flush;

				// stop the time measurement
				m.stop();

				// get throughput
				double kbytes_per_second = (serializer.getLength(bundle) / m.getSeconds()) / 1024;

				// print out throughput
				IBRCOMMON_LOGGER_DEBUG(5) << "transfer finished after " << m << " with "
						<< std::setiosflags(std::ios::fixed) << std::setprecision(2) << kbytes_per_second << " kb/s" << IBRCOMMON_LOGGER_ENDL;

			} catch (ibrcommon::Exception ex) {
				// the connection not available
				IBRCOMMON_LOGGER_DEBUG(10) << "connection error: " << ex.what() << IBRCOMMON_LOGGER_ENDL;

				// forward exception
				throw;
			}

			return conn;
		}

		void TCPConvergenceLayer::TCPConnection::run()
		{
			try {
				// firstly, do the handshake
				handshake();

				// start the sender
				_sender.start();

				while (true)
				{
					dtn::data::Bundle bundle;

					try {
						(*this) >> bundle;

						// check the bundle
						if ( ( bundle._destination == EID() ) || ( bundle._source == EID() ) )
						{
							// invalid bundle!
						}
						else
						{
							// raise default bundle received event
							dtn::net::BundleReceivedEvent::raise(_peer._localeid, bundle);
						}

					} catch (dtn::data::Validator::RejectedException ex) {
						// bundle rejected
						rejectTransmission();
					}

					yield();
				}
			} catch (const ibrcommon::ThreadException &ex) {
				IBRCOMMON_LOGGER(error) << "failed to start thread in TCPConnection\n" << ex.what() << IBRCOMMON_LOGGER_ENDL;
				_stream.shutdown(StreamConnection::CONNECTION_SHUTDOWN_ERROR);
			} catch (const std::exception &ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "TCPConnection::run(): std::exception (" << ex.what() << ")" << IBRCOMMON_LOGGER_ENDL;
				_stream.shutdown(StreamConnection::CONNECTION_SHUTDOWN_ERROR);
			} catch (...) {
				IBRCOMMON_LOGGER_DEBUG(10) << "TCPConnection::run(): canceled" << IBRCOMMON_LOGGER_ENDL;
				throw;
			}
		}

		void TCPConvergenceLayer::TCPConnection::finally()
		{
			// close the tcpstream
			try {
				_tcpstream->close();
			} catch (ibrcommon::ConnectionClosedException ex) {

			}

			// shutdown the sender thread
			_sender.shutdown();

			{
				ibrcommon::MutexLock l(_server.mutex());
				_server.remove(this);
			}

			// connection down -> connection to be deleted
			delete this;
		}

		TCPConvergenceLayer::TCPConnection::Sender::Sender(TCPConnection &connection)
		 : _abort(false), _connection(connection)
		{
		}

		TCPConvergenceLayer::TCPConnection::Sender::~Sender()
		{
		}

		void TCPConvergenceLayer::TCPConnection::Sender::shutdown()
		{
			_abort = true;
			this->abort();
			//::sleep(1);
			this->stop();
			this->join();
		}

		void TCPConvergenceLayer::TCPConnection::Sender::run()
		{
			try {
				while (!_abort)
				{
					try {
						dtn::data::Bundle bundle = getnpop(true);

						// send bundle
						_connection << bundle;
					} catch (ibrcommon::QueueUnblockedException) { }

					// idle a little bit
					yield();
				}
			} catch (std::exception ex) {
				IBRCOMMON_LOGGER(error) << "TCPConnection::Sender terminated by exception: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
			} catch (...) {
				IBRCOMMON_LOGGER_DEBUG(10) << "TCPConnection: canceled" << IBRCOMMON_LOGGER_ENDL;
				throw;
			}

			_connection.stop();
		}

		void TCPConvergenceLayer::TCPConnection::clearQueue()
		{
			try {
				while (true)
				{
					const dtn::data::Bundle bundle = _sender.getnpop();

					if (_lastack > 0)
					{
						// some data are already acknowledged, make a fragment?
						//TODO: make a fragment
						TransferAbortedEvent::raise(EID(_node.getURI()), bundle, dtn::net::TransferAbortedEvent::REASON_CONNECTION_DOWN);
					}
					else
					{
						// raise transfer abort event for all bundles without an ACK
						TransferAbortedEvent::raise(EID(_node.getURI()), bundle, dtn::net::TransferAbortedEvent::REASON_CONNECTION_DOWN);
					}

					// set last ack to zero
					_lastack = 0;
				}
			} catch (ibrcommon::QueueUnblockedException) {
				// queue emtpy
			}
		}

		void TCPConvergenceLayer::TCPConnection::Sender::finally()
		{
			_connection.clearQueue();
		}
	}
}

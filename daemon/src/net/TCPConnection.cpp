/*
 * TCPConnection.cpp
 *
 *  Created on: 26.04.2010
 *      Author: morgenro
 */

#include "core/BundleCore.h"
#include "core/GlobalEvent.h"
#include "core/BundleEvent.h"
#include "core/BundleStorage.h"
#include "Configuration.h"

#include "net/TCPConvergenceLayer.h"
#include "net/BundleReceivedEvent.h"
#include "net/ConnectionEvent.h"
#include "net/TransferCompletedEvent.h"
#include "net/TransferAbortedEvent.h"

#include <ibrcommon/TimeMeasurement.h>
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
		 : GenericConnection<TCPConvergenceLayer::TCPConnection>((GenericServer<TCPConvergenceLayer::TCPConnection>&)tcpsrv), ibrcommon::DetachedThread(),
		   _peer(), _node(Node::NODE_CONNECTED), _tcpstream(stream), _stream(*this, *stream, dtn::daemon::Configuration::getInstance().getNetwork().getTCPChunkSize()), _sender(*this, _keepalive_timeout),
		   _name(name), _timeout(timeout), _lastack(0), _keepalive_timeout(0)
		{
			if ( dtn::daemon::Configuration::getInstance().getNetwork().getTCPOptionNoDelay() )
			{
				stream->enableNoDelay();
			}

			stream->enableKeepalive();
			_node.setProtocol(Node::CONN_TCPIP);
		}

		TCPConvergenceLayer::TCPConnection::~TCPConnection()
		{
		}

		void TCPConvergenceLayer::TCPConnection::queue(const dtn::data::BundleID &bundle)
		{
			_sender.push(bundle);
		}

		const StreamContactHeader TCPConvergenceLayer::TCPConnection::getHeader() const
		{
			return _peer;
		}

		const dtn::core::Node& TCPConvergenceLayer::TCPConnection::getNode() const
		{
			return _node;
		}

		void TCPConvergenceLayer::TCPConnection::rejectTransmission()
		{
			_stream.reject();
		}

		void TCPConvergenceLayer::TCPConnection::eventShutdown()
		{
		}

		void TCPConvergenceLayer::TCPConnection::eventTimeout()
		{
			// event
			ConnectionEvent::raise(ConnectionEvent::CONNECTION_TIMEOUT, _node);

			// stop the receiver thread
			this->stop();
		}

		void TCPConvergenceLayer::TCPConnection::eventError()
		{
		}

		void TCPConvergenceLayer::TCPConnection::eventConnectionUp(const StreamContactHeader &header)
		{
			_peer = header;
			_node.setURI(header._localeid.getString());
			_keepalive_timeout = header._keepalive * 1000;

			// raise up event
			ConnectionEvent::raise(ConnectionEvent::CONNECTION_UP, _node);
		}

		void TCPConvergenceLayer::TCPConnection::eventConnectionDown()
		{
			IBRCOMMON_LOGGER_DEBUG(40) << "TCPConnection::eventConnectionDown()" << IBRCOMMON_LOGGER_ENDL;

			try {
				// stop the sender
				_sender.abort();
				_sender.stop();
			} catch (const ibrcommon::ThreadException &ex) {
				IBRCOMMON_LOGGER_DEBUG(50) << "TCPConnection::eventConnectionDown(): ThreadException (" << ex.what() << ")" << IBRCOMMON_LOGGER_ENDL;
			}

			if (_peer._localeid != dtn::data::EID())
			{
				// event
				ConnectionEvent::raise(ConnectionEvent::CONNECTION_DOWN, _node);
			}
		}

		void TCPConvergenceLayer::TCPConnection::eventBundleRefused()
		{
			try {
				const dtn::data::BundleID bundle = _sentqueue.getnpop();

				// requeue the bundle
				TransferAbortedEvent::raise(EID(_node.getURI()), bundle, dtn::net::TransferAbortedEvent::REASON_REFUSED);

				// set ACK to zero
				_lastack = 0;

			} catch (ibrcommon::QueueUnblockedException) {
				// pop on empty queue!
				IBRCOMMON_LOGGER(error) << "transfer refused without a bundle in queue" << IBRCOMMON_LOGGER_ENDL;
			}
		}

		void TCPConvergenceLayer::TCPConnection::eventBundleForwarded()
		{
			try {
				const dtn::data::MetaBundle bundle = _sentqueue.getnpop();

				// signal completion of the transfer
				TransferCompletedEvent::raise(EID(_node.getURI()), bundle);

				// raise bundle event
				dtn::core::BundleEvent::raise(bundle, BUNDLE_FORWARDED);

				// set ACK to zero
				_lastack = 0;
			} catch (ibrcommon::QueueUnblockedException) {
				// pop on empty queue!
				IBRCOMMON_LOGGER(error) << "transfer completed without a bundle in queue" << IBRCOMMON_LOGGER_ENDL;
			}
		}

		void TCPConvergenceLayer::TCPConnection::eventBundleAck(size_t ack)
		{
			_lastack = ack;
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

		void TCPConvergenceLayer::TCPConnection::shutdown()
		{
			// shutdown
			_stream.shutdown(StreamConnection::CONNECTION_SHUTDOWN_ERROR);

			try {
				// abort the connection thread
				this->stop();
			} catch (const ibrcommon::ThreadException &ex) {
				IBRCOMMON_LOGGER_DEBUG(50) << "TCPConnection::shutdown(): ThreadException (" << ex.what() << ")" << IBRCOMMON_LOGGER_ENDL;
			}
		}

		void TCPConvergenceLayer::TCPConnection::finally()
		{
			IBRCOMMON_LOGGER_DEBUG(60) << "TCPConnection down" << IBRCOMMON_LOGGER_ENDL;

			// close the tcpstream
			try {
				_tcpstream->close();
			} catch (ibrcommon::ConnectionClosedException ex) {

			}

			try {
				// shutdown the sender thread
				_sender.shutdown();
			} catch (std::exception) {

			}

			ibrcommon::MutexLock l(_server.mutex());
			_server.remove(this);
		}

		void TCPConvergenceLayer::TCPConnection::run()
		{
			try {
				// do the handshake
				char flags = 0;

				// enable ACKs and NACKs
				flags |= dtn::streams::StreamContactHeader::REQUEST_ACKNOWLEDGMENTS;
				flags |= dtn::streams::StreamContactHeader::REQUEST_NEGATIVE_ACKNOWLEDGMENTS;

				// do the handshake
				_stream.handshake(_name, _timeout, flags);

				// start the sender
				_sender.start();

				while (true)
				{
					try {
						// create a new empty bundle
						dtn::data::Bundle bundle;

						// deserialize the bundle
						(*this) >> bundle;

						// check the bundle
						if ( ( bundle._destination == EID() ) || ( bundle._source == EID() ) )
						{
							// invalid bundle!
							throw dtn::data::Validator::RejectedException("destination or source EID is null");
						}

						// raise default bundle received event
						dtn::net::BundleReceivedEvent::raise(_peer._localeid, bundle);
					}
					catch (dtn::data::Validator::RejectedException ex)
					{
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

		TCPConvergenceLayer::TCPConnection::Sender::Sender(TCPConnection &connection, size_t &keepalive_timeout)
		 : _abort(false), _connection(connection), _keepalive_timeout(keepalive_timeout)
		{
		}

		TCPConvergenceLayer::TCPConnection::Sender::~Sender()
		{
		}

		void TCPConvergenceLayer::TCPConnection::Sender::shutdown()
		{
			try {
				this->stop();
			} catch (const ibrcommon::ThreadException &ex) {
				IBRCOMMON_LOGGER_DEBUG(50) << "TCPConnection::Sender::shutdown(): ThreadException (" << ex.what() << ")" << IBRCOMMON_LOGGER_ENDL;
			}

			this->join();
		}

		void TCPConvergenceLayer::TCPConnection::Sender::run()
		{
			try {
				dtn::core::BundleStorage &storage = dtn::core::BundleCore::getInstance().getStorage();

				while (!_abort)
				{
					try {
						dtn::data::BundleID id = getnpop(true, _keepalive_timeout);

						try {
							// read the bundle out of the storage
							const dtn::data::Bundle bundle = storage.get(id);

							// send bundle
							_connection << bundle;
						} catch (const dtn::core::BundleStorage::NoBundleFoundException&) {
							// send transfer aborted event
							TransferAbortedEvent::raise(EID(_connection._node.getURI()), id, dtn::net::TransferAbortedEvent::REASON_BUNDLE_DELETED);
						}
					} catch (const ibrcommon::QueueUnblockedException &ex) {
						switch (ex.reason)
						{
							case ibrcommon::QueueUnblockedException::QUEUE_ERROR:
							case ibrcommon::QueueUnblockedException::QUEUE_ABORT:
								throw;
							case ibrcommon::QueueUnblockedException::QUEUE_TIMEOUT:
							{
								// send a keepalive
								_connection.keepalive();
							}
						}
					}

					// idle a little bit
					yield();
				}
			} catch (std::exception ex) {
				IBRCOMMON_LOGGER(error) << "TCPConnection::Sender terminated by exception: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
			} catch (...) {
				IBRCOMMON_LOGGER_DEBUG(10) << "TCPConnection::Sender canceled" << IBRCOMMON_LOGGER_ENDL;
				throw;
			}

			_connection.stop();
		}

		void TCPConvergenceLayer::TCPConnection::clearQueue()
		{
			try {
				while (true)
				{
					const dtn::data::BundleID id = _sender.getnpop();

					if (_lastack > 0)
					{
						// some data are already acknowledged, make a fragment?
						//TODO: make a fragment
						TransferAbortedEvent::raise(EID(_node.getURI()), id, dtn::net::TransferAbortedEvent::REASON_CONNECTION_DOWN);
					}
					else
					{
						// raise transfer abort event for all bundles without an ACK
						TransferAbortedEvent::raise(EID(_node.getURI()), id, dtn::net::TransferAbortedEvent::REASON_CONNECTION_DOWN);
					}

					// set last ack to zero
					_lastack = 0;
				}
			} catch (ibrcommon::QueueUnblockedException) {
				// queue emtpy
			}
		}

		void TCPConvergenceLayer::TCPConnection::keepalive()
		{
			_stream.keepalive();
		}

		void TCPConvergenceLayer::TCPConnection::Sender::finally()
		{
			_connection.clearQueue();
		}
	}
}

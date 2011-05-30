/*
 * TCPConnection.cpp
 *
 *  Created on: 26.04.2010
 *      Author: morgenro
 */

#include "config.h"
#include "Configuration.h"
#include "core/BundleCore.h"
#include "core/BundleEvent.h"
#include "core/BundleStorage.h"

#include "net/TCPConvergenceLayer.h"
#include "net/BundleReceivedEvent.h"
#include "net/ConnectionEvent.h"
#include "net/TransferCompletedEvent.h"
#include "net/TransferAbortedEvent.h"
#include "routing/RequeueBundleEvent.h"

#include <ibrcommon/net/tcpclient.h>
#include <ibrcommon/TimeMeasurement.h>
#include <ibrcommon/net/vinterface.h>
#include <ibrcommon/Logger.h>

#include <iostream>
#include <iomanip>

#ifdef WITH_BUNDLE_SECURITY
#include "security/SecurityManager.h"
#endif

namespace dtn
{
	namespace net
	{
		/*
		 * class TCPConnection
		 */
		TCPConnection::TCPConnection(TCPConvergenceLayer &tcpsrv, ibrcommon::tcpstream *stream, const dtn::data::EID &name, const size_t timeout)
		 : _peer(), _node(Node::NODE_CONNECTED), _tcpstream(stream), _stream(*this, *stream, dtn::daemon::Configuration::getInstance().getNetwork().getTCPChunkSize()), _sender(*this, _keepalive_timeout),
		   _name(name), _timeout(timeout), _lastack(0), _keepalive_timeout(0), _callback(tcpsrv)
		{
			_stream.exceptions(std::ios::badbit | std::ios::eofbit);

			if ( dtn::daemon::Configuration::getInstance().getNetwork().getTCPOptionNoDelay() )
			{
				stream->enableNoDelay();
			}

			stream->enableLinger(10);
			stream->enableKeepalive();

			// add default TCP connection
			_node.add(dtn::core::Node::URI("0.0.0.0", Node::CONN_TCPIP));
		}

		TCPConnection::TCPConnection(TCPConvergenceLayer &tcpsrv, const dtn::core::Node &node, const dtn::data::EID &name, const size_t timeout)
		 : _peer(), _node(node), _tcpstream(new ibrcommon::tcpclient()), _stream(*this, *_tcpstream, dtn::daemon::Configuration::getInstance().getNetwork().getTCPChunkSize()), _sender(*this, _keepalive_timeout),
		   _name(name), _timeout(timeout), _lastack(0), _keepalive_timeout(0), _callback(tcpsrv)
		{
			_stream.exceptions(std::ios::badbit | std::ios::eofbit);
		}

		TCPConnection::~TCPConnection()
		{
		}

		void TCPConnection::queue(const dtn::data::BundleID &bundle)
		{
			_sender.push(bundle);
		}

		const StreamContactHeader& TCPConnection::getHeader() const
		{
			return _peer;
		}

		const dtn::core::Node& TCPConnection::getNode() const
		{
			return _node;
		}

		void TCPConnection::rejectTransmission()
		{
			_stream.reject();
		}

		void TCPConnection::eventShutdown(StreamConnection::ConnectionShutdownCases)
		{
		}

		void TCPConnection::eventTimeout()
		{
			// event
			ConnectionEvent::raise(ConnectionEvent::CONNECTION_TIMEOUT, _node);

			// stop the receiver thread
			this->stop();
		}

		void TCPConnection::eventError()
		{
		}

		void TCPConnection::eventConnectionUp(const StreamContactHeader &header)
		{
			_peer = header;
			_node.setEID(header._localeid);
			_keepalive_timeout = header._keepalive * 1000;

			// set the incoming timer if set (> 0)
			if (_peer._keepalive > 0)
			{
				// set the timer
				_tcpstream->setTimeout(header._keepalive * 2);
			}

			// raise up event
			ConnectionEvent::raise(ConnectionEvent::CONNECTION_UP, _node);
		}

		void TCPConnection::eventConnectionDown()
		{
			IBRCOMMON_LOGGER_DEBUG(40) << "TCPConnection::eventConnectionDown()" << IBRCOMMON_LOGGER_ENDL;

			try {
				// stop the sender
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

		void TCPConnection::eventBundleRefused()
		{
			try {
				const dtn::data::BundleID bundle = _sentqueue.getnpop();

				// requeue the bundle
				TransferAbortedEvent::raise(EID(_node.getEID()), bundle, dtn::net::TransferAbortedEvent::REASON_REFUSED);

				// set ACK to zero
				_lastack = 0;

			} catch (const ibrcommon::QueueUnblockedException&) {
				// pop on empty queue!
				IBRCOMMON_LOGGER(error) << "transfer refused without a bundle in queue" << IBRCOMMON_LOGGER_ENDL;
			}
		}

		void TCPConnection::eventBundleForwarded()
		{
			try {
				const dtn::data::MetaBundle bundle = _sentqueue.getnpop();

				// signal completion of the transfer
				TransferCompletedEvent::raise(_node.getEID(), bundle);

				// raise bundle event
				dtn::core::BundleEvent::raise(bundle, BUNDLE_FORWARDED);

				// set ACK to zero
				_lastack = 0;
			} catch (const ibrcommon::QueueUnblockedException&) {
				// pop on empty queue!
				IBRCOMMON_LOGGER(error) << "transfer completed without a bundle in queue" << IBRCOMMON_LOGGER_ENDL;
			}
		}

		void TCPConnection::eventBundleAck(size_t ack)
		{
			_lastack = ack;
		}

		void TCPConnection::initialize()
		{
			// start the receiver for incoming bundles + handshake
			try {
				start();
			} catch (const ibrcommon::ThreadException &ex) {
				IBRCOMMON_LOGGER(error) << "failed to start thread in TCPConnection\n" << ex.what() << IBRCOMMON_LOGGER_ENDL;
			}
		}

		void TCPConnection::shutdown()
		{
			// shutdown
			_stream.shutdown(StreamConnection::CONNECTION_SHUTDOWN_ERROR);

			try {
				// abort the connection thread
				ibrcommon::DetachedThread::stop();
			} catch (const ibrcommon::ThreadException &ex) {
				IBRCOMMON_LOGGER_DEBUG(50) << "TCPConnection::shutdown(): ThreadException (" << ex.what() << ")" << IBRCOMMON_LOGGER_ENDL;
			}
		}

		bool TCPConnection::__cancellation()
		{
			// close the stream
			try {
				(*_tcpstream).close();
			} catch (const ibrcommon::ConnectionClosedException&) { };

			return true;
		}

		void TCPConnection::finally()
		{
			IBRCOMMON_LOGGER_DEBUG(60) << "TCPConnection down" << IBRCOMMON_LOGGER_ENDL;

			// close the tcpstream
			try {
				_tcpstream->close();
			} catch (const ibrcommon::ConnectionClosedException&) { };

			try {
				// shutdown the sender thread
				_sender.stop();
			} catch (const std::exception&) { };

			try {
				_callback.connectionDown(this);
			} catch (const ibrcommon::MutexException&) { };

			// clear the queue
			clearQueue();
		}

		void TCPConnection::setup()
		{
			// variables for address and port
			std::string address = "0.0.0.0";
			unsigned int port = 0;

			// try to connect to the other side
			try {
				const std::list<dtn::core::Node::URI> uri_list = _node.get(dtn::core::Node::CONN_TCPIP);
				if (uri_list.empty()) throw ibrcommon::tcpclient::SocketException("no address available to connect");

				// decode address and port
				const dtn::core::Node::URI &uri = uri_list.front();
				uri.decode(address, port);

				ibrcommon::tcpclient &client = dynamic_cast<ibrcommon::tcpclient&>(*_tcpstream);
				client.open(address, port, _timeout);

				if ( dtn::daemon::Configuration::getInstance().getNetwork().getTCPOptionNoDelay() )
				{
					_tcpstream->enableNoDelay();
				}

				_tcpstream->enableLinger(10);
				_tcpstream->enableKeepalive();
			} catch (const ibrcommon::tcpclient::SocketException&) {
				// error on open, requeue all bundles in the queue
				IBRCOMMON_LOGGER(warning) << "connection to " << _node.toString() << " (" << address << ":" << port << ") failed" << IBRCOMMON_LOGGER_ENDL;
				_stream.shutdown(StreamConnection::CONNECTION_SHUTDOWN_ERROR);
				throw;
			} catch (const bad_cast&) { };
		}

		void TCPConnection::run()
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

				while (!_stream.eof())
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
					catch (const dtn::data::Validator::RejectedException &ex)
					{
						// bundle rejected
						rejectTransmission();

						// display the rejection
						IBRCOMMON_LOGGER(warning) << "bundle has been rejected: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
					}
					catch (const dtn::InvalidDataException &ex) {
						// bundle rejected
						rejectTransmission();

						// display the rejection
						IBRCOMMON_LOGGER(warning) << "invalid bundle-data received: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
					}

					yield();
				}
			} catch (const ibrcommon::ThreadException &ex) {
				IBRCOMMON_LOGGER(error) << "failed to start thread in TCPConnection\n" << ex.what() << IBRCOMMON_LOGGER_ENDL;
				_stream.shutdown(StreamConnection::CONNECTION_SHUTDOWN_ERROR);
			} catch (const std::exception &ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "TCPConnection::run(): std::exception (" << ex.what() << ")" << IBRCOMMON_LOGGER_ENDL;
				_stream.shutdown(StreamConnection::CONNECTION_SHUTDOWN_ERROR);
			}
		}


		TCPConnection& operator>>(TCPConnection &conn, dtn::data::Bundle &bundle)
		{
			std::iostream &stream = conn._stream;

			// check if the stream is still good
			if (!stream.good()) throw ibrcommon::IOException("stream went bad");

			dtn::data::DefaultDeserializer(stream, dtn::core::BundleCore::getInstance()) >> bundle;
			return conn;
		}

		TCPConnection& operator<<(TCPConnection &conn, const dtn::data::Bundle &bundle)
		{
			// prepare a measurement
			ibrcommon::TimeMeasurement m;

			std::iostream &stream = conn._stream;

			// create a serializer
			dtn::data::DefaultSerializer serializer(stream);

			// put the bundle into the sentqueue
			conn._sentqueue.push(bundle);

			// start the measurement
			m.start();

			try {
				// activate exceptions for this method
				if (!stream.good()) throw ibrcommon::IOException("stream went bad");

				// transmit the bundle
				serializer << bundle;

				// flush the stream
				stream << std::flush;

				// stop the time measurement
				m.stop();

				// get throughput
				double kbytes_per_second = (serializer.getLength(bundle) / m.getSeconds()) / 1024;

				// print out throughput
				IBRCOMMON_LOGGER_DEBUG(5) << "transfer finished after " << m << " with "
						<< std::setiosflags(std::ios::fixed) << std::setprecision(2) << kbytes_per_second << " kb/s" << IBRCOMMON_LOGGER_ENDL;

			} catch (const ibrcommon::Exception &ex) {
				// the connection not available
				IBRCOMMON_LOGGER_DEBUG(10) << "connection error: " << ex.what() << IBRCOMMON_LOGGER_ENDL;

				// forward exception
				throw;
			}

			return conn;
		}

		TCPConnection::Sender::Sender(TCPConnection &connection, size_t &keepalive_timeout)
		 : _connection(connection), _keepalive_timeout(keepalive_timeout)
		{
		}

		TCPConnection::Sender::~Sender()
		{
			ibrcommon::JoinableThread::join();
		}

		bool TCPConnection::Sender::__cancellation()
		{
			// cancel the main thread in here
			ibrcommon::Queue<dtn::data::BundleID>::abort();

			return true;
		}

		void TCPConnection::Sender::run()
		{
			try {
				dtn::core::BundleStorage &storage = dtn::core::BundleCore::getInstance().getStorage();

				while (_connection.good())
				{
					try {
						_current_transfer = ibrcommon::Queue<dtn::data::BundleID>::getnpop(true, _keepalive_timeout);

						try {
							// read the bundle out of the storage
							dtn::data::Bundle bundle = storage.get(_current_transfer);

#ifdef WITH_BUNDLE_SECURITY
							const dtn::daemon::Configuration::Security::Level seclevel =
									dtn::daemon::Configuration::getInstance().getSecurity().getLevel();

							if (seclevel & dtn::daemon::Configuration::Security::SECURITY_LEVEL_AUTHENTICATED)
							{
								try {
									dtn::security::SecurityManager::getInstance().auth(bundle);
								} catch (const dtn::security::SecurityManager::KeyMissingException&) {
									// sign requested, but no key is available
									IBRCOMMON_LOGGER(warning) << "No key available for sign process." << IBRCOMMON_LOGGER_ENDL;
								}
							}
#endif
							// send bundle
							_connection << bundle;
						} catch (const dtn::core::BundleStorage::NoBundleFoundException&) {
							// send transfer aborted event
							TransferAbortedEvent::raise(_connection._node.getEID(), _current_transfer, dtn::net::TransferAbortedEvent::REASON_BUNDLE_DELETED);
						}
						
						// unset the current transfer
						_current_transfer = dtn::data::BundleID();
						
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
			} catch (const ibrcommon::QueueUnblockedException &ex) {
				IBRCOMMON_LOGGER_DEBUG(50) << "TCPConnection::Sender::run(): aborted" << IBRCOMMON_LOGGER_ENDL;
				return;
			} catch (const std::exception &ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "TCPConnection::Sender terminated by exception: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
			}

			_connection.stop();
		}

		void TCPConnection::clearQueue()
		{
			// requeue all bundles still queued
			try {
				while (true)
				{
					const dtn::data::BundleID id = _sender.getnpop();

					// raise transfer abort event for all bundles without an ACK
					dtn::routing::RequeueBundleEvent::raise(_node.getEID(), id);
				}
			} catch (const ibrcommon::QueueUnblockedException&) {
				// queue emtpy
			}

			// requeue all bundles still in transit
			try {
				while (true)
				{
					const dtn::data::BundleID id = _sentqueue.getnpop();

					if (_lastack > 0)
					{
						// some data are already acknowledged, make a fragment?
						//TODO: make a fragment
						dtn::routing::RequeueBundleEvent::raise(_node.getEID(), id);
					}
					else
					{
						// raise transfer abort event for all bundles without an ACK
						dtn::routing::RequeueBundleEvent::raise(_node.getEID(), id);
					}

					// set last ack to zero
					_lastack = 0;
				}
			} catch (const ibrcommon::QueueUnblockedException&) {
				// queue emtpy
			}
		}

		void TCPConnection::keepalive()
		{
			_stream.keepalive();
		}

		bool TCPConnection::good() const
		{
			return _tcpstream->good();
		}

		void TCPConnection::Sender::finally()
		{
			// notify the aborted transfer of the last bundle
			if (_current_transfer != dtn::data::BundleID())
			{
				dtn::routing::RequeueBundleEvent::raise(_connection._node.getEID(), _current_transfer);
			}
		}

		bool TCPConnection::match(const dtn::core::Node &n) const
		{
			return (_node == n);
		}

		bool TCPConnection::match(const dtn::data::EID &destination) const
		{
			return (_node.getEID() == destination.getNode());
		}

		bool TCPConnection::match(const NodeEvent &evt) const
		{
			const dtn::core::Node &n = evt.getNode();
			return match(n);
		}
	}
}

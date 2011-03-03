/*
 * TCPConnection.cpp
 *
 *  Created on: 26.04.2010
 *      Author: morgenro
 */

#include "config.h"
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
			_node.setProtocol(Node::CONN_TCPIP);
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
				TransferAbortedEvent::raise(EID(_node.getURI()), bundle, dtn::net::TransferAbortedEvent::REASON_REFUSED);

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
				TransferCompletedEvent::raise(EID(_node.getURI()), bundle);

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
				this->stop();
			} catch (const ibrcommon::ThreadException &ex) {
				IBRCOMMON_LOGGER_DEBUG(50) << "TCPConnection::shutdown(): ThreadException (" << ex.what() << ")" << IBRCOMMON_LOGGER_ENDL;
			}
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
			// try to connect to the other side
			try {
				ibrcommon::tcpclient &client = dynamic_cast<ibrcommon::tcpclient&>(*_tcpstream);
				client.open(_node.getAddress(), _node.getPort());

				if ( dtn::daemon::Configuration::getInstance().getNetwork().getTCPOptionNoDelay() )
				{
					_tcpstream->enableNoDelay();
				}

				_tcpstream->enableLinger(10);
				_tcpstream->enableKeepalive();
				_node.setProtocol(Node::CONN_TCPIP);
			} catch (const ibrcommon::tcpclient::SocketException&) {
				// error on open, requeue all bundles in the queue
				IBRCOMMON_LOGGER(warning) << "connection to " << _node.getURI() << " (" << _node.getAddress() << ":" << _node.getPort() << ") failed" << IBRCOMMON_LOGGER_ENDL;
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
		 : _abort(false), _connection(connection), _keepalive_timeout(keepalive_timeout)
		{
		}

		TCPConnection::Sender::~Sender()
		{
			join();
		}

		bool TCPConnection::Sender::__cancellation()
		{
			// cancel the main thread in here
			_abort = true;
			this->abort();

			// return false, to signal that further cancel (the hardway) is needed
			return false;
		}

		void TCPConnection::Sender::run()
		{
			// The queue is not cancel-safe with uclibc, so we need to
			// disable cancel here
			int oldstate;
			ibrcommon::Thread::disableCancel(oldstate);

			try {
				dtn::core::BundleStorage &storage = dtn::core::BundleCore::getInstance().getStorage();

				while (!_abort)
				{
					try {
						_current_transfer = getnpop(true, _keepalive_timeout);

						try {
							// read the bundle out of the storage
							dtn::data::Bundle bundle = storage.get(_current_transfer);

#ifdef WITH_BUNDLE_SECURITY
//							// Apply BundleAuthenticationBlocks, if keys are present
//							dtn::security::SecurityManager::getInstance().outgoing_p2p(EID(_connection.getNode().getURI()), bundle);

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

							// enable cancellation during transmission
							ibrcommon::Thread::CancelProtector cprotect(true);

							// send bundle
							_connection << bundle;

						} catch (const dtn::core::BundleStorage::NoBundleFoundException&) {
							// send transfer aborted event
							TransferAbortedEvent::raise(EID(_connection._node.getURI()), _current_transfer, dtn::net::TransferAbortedEvent::REASON_BUNDLE_DELETED);
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
			try {
				while (true)
				{
					const dtn::data::BundleID id = _sender.getnpop();

					if (_lastack > 0)
					{
						// some data are already acknowledged, make a fragment?
						//TODO: make a fragment
						dtn::routing::RequeueBundleEvent::raise(_node.getURI(), id);
					}
					else
					{
						// raise transfer abort event for all bundles without an ACK
						dtn::routing::RequeueBundleEvent::raise(_node.getURI(), id);
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

		void TCPConnection::Sender::finally()
		{
			// notify the aborted transfer of the last bundle
			if (_current_transfer != dtn::data::BundleID())
			{
				dtn::routing::RequeueBundleEvent::raise(_connection._node.getURI(), _current_transfer);
			}
		}

		bool TCPConnection::match(const dtn::core::Node &n) const
		{
			return (_node == n);
		}

		bool TCPConnection::match(const dtn::data::EID &destination) const
		{
			return (_node.getURI() == destination.getNodeEID());
		}

		bool TCPConnection::match(const NodeEvent &evt) const
		{
			const dtn::core::Node &n = evt.getNode();
			return match(n);
		}
	}
}

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
		TCPConvergenceLayer::TCPConnection::TCPConnection(ibrcommon::tcpstream *stream)
		 : dtn::data::DefaultDeserializer(_stream), _free(false), _tcpstream(stream), _stream(*this, *stream), _sender(*this), _receiver(*this), _name(), _timeout(0), _lastack(0)
		{
		}

		TCPConvergenceLayer::TCPConnection::~TCPConnection()
		{
			shutdown();
		}

		void TCPConvergenceLayer::TCPConnection::iamfree()
		{
			_free = true;
		}

		bool TCPConvergenceLayer::TCPConnection::free()
		{
			ibrcommon::MutexLock l(_freemutex);
			return _free;
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

			// start the sender for outgoing bundles
			_sender.start();
		}

		void TCPConvergenceLayer::TCPConnection::initialize(const dtn::data::EID &name, const size_t timeout)
		{
			_name = name;
			_timeout = timeout;

			// start the receiver for incoming bundles + handshake
			_receiver.start();
		}

		void TCPConvergenceLayer::TCPConnection::eventConnectionUp(const StreamContactHeader &header)
		{
			_peer = header;
			_node.setURI(header._localeid.getString());

			// raise up event
			ConnectionEvent::raise(ConnectionEvent::CONNECTION_UP, header.getEID());
		}

		void TCPConvergenceLayer::TCPConnection::eventConnectionDown()
		{
			// event
			ConnectionEvent::raise(ConnectionEvent::CONNECTION_DOWN, _peer._localeid);

			try {
				while (true)
				{
					const dtn::data::Bundle bundle = _sentqueue.frontpop();

					if (_lastack > 0)
					{
						// some data are already acknowledged, make a fragment?
						//TODO: make a fragment
						TransferAbortedEvent::raise(EID(_node.getURI()), bundle);
					}
					else
					{
						// raise transfer abort event for all bundles without an ACK
						TransferAbortedEvent::raise(EID(_node.getURI()), bundle);
					}

					// set last ack to zero
					_lastack = 0;
				}
			} catch (ibrcommon::Exception ex) {
				// queue emtpy
			}

			ibrcommon::MutexLock l(_freemutex);
			iamfree();
		}

		void TCPConvergenceLayer::TCPConnection::eventBundleRefused()
		{
			try {
				const dtn::data::Bundle bundle = _sentqueue.frontpop();

				// requeue the bundle
				TransferAbortedEvent::raise(EID(_node.getURI()), bundle);

				// set ACK to zero
				_lastack = 0;

			} catch (ibrcommon::Exception ex) {
				// pop on empty queue!
			}
		}

		void TCPConvergenceLayer::TCPConnection::eventBundleForwarded()
		{
			try {
				const dtn::data::Bundle bundle = _sentqueue.frontpop();

				// signal completion of the transfer
				TransferCompletedEvent::raise(EID(_node.getURI()), bundle);

				// raise bundle event
				dtn::core::BundleEvent::raise(bundle, BUNDLE_FORWARDED);

				// set ACK to zero
				_lastack = 0;
			} catch (ibrcommon::Exception ex) {
				// pop on empty queue!
			}
		}

		void TCPConvergenceLayer::TCPConnection::eventBundleAck(size_t ack)
		{
			_lastack = ack;
		}

		void TCPConvergenceLayer::TCPConnection::eventShutdown()
		{
			// stop the sender
			_sender.shutdown();

			// stop the receiver
			_receiver.shutdown();

			// close the tcpstream
			try {
				_tcpstream->done();
				_tcpstream->close();
			} catch (ibrcommon::ConnectionClosedException ex) {

			}
		}

		void TCPConvergenceLayer::TCPConnection::eventTimeout()
		{
			// stop the sender
			_sender.shutdown();

			// event
			ConnectionEvent::raise(ConnectionEvent::CONNECTION_TIMEOUT, _peer._localeid);

			// close the tcpstream
			try {
				_tcpstream->done();
				_tcpstream->close();
			} catch (ibrcommon::ConnectionClosedException ex) {

			}
		}

		void TCPConvergenceLayer::TCPConnection::eventError()
		{
			// stop the sender
			_sender.shutdown();

			// close the tcpstream
			try {
				_tcpstream->close();
			} catch (ibrcommon::ConnectionClosedException ex) {

			}
		}

		void TCPConvergenceLayer::TCPConnection::shutdown()
		{
			_stream.shutdown();
		}

		const dtn::core::Node& TCPConvergenceLayer::TCPConnection::getNode() const
		{
			return _node;
		}

		void TCPConvergenceLayer::TCPConnection::validate(const dtn::data::PrimaryBlock&) const throw (dtn::data::DefaultDeserializer::RejectedException)
		{
			/*
			 *
			 * TODO: reject a bundle if...
			 * ... the bundle version is not supported
			 * ... it is expired
			 * ... already in the storage
			 * ... a fragment of an already received bundle in the storage
			 *
			 * throw dtn::data::DefaultDeserializer::RejectedException();
			 *
			 */
		}

		void TCPConvergenceLayer::TCPConnection::validate(const dtn::data::Block&, const size_t) const throw (dtn::data::DefaultDeserializer::RejectedException)
		{
			/*
			 *
			 * TODO: reject a block if
			 * ... it exceeds the payload limit
			 *
			 * throw dtn::data::DefaultDeserializer::RejectedException();
			 *
			 */
		}

		void TCPConvergenceLayer::TCPConnection::validate(const dtn::data::Bundle&) const throw (dtn::data::DefaultDeserializer::RejectedException)
		{
			/*
			 *
			 * TODO: reject a bundle if
			 * ... the security checks (DTNSEC) failed
			 * ... a checksum mismatch is detected (CRC)
			 *
			 * throw dtn::data::DefaultDeserializer::RejectedException();
			 *
			 */
		}

		void TCPConvergenceLayer::TCPConnection::rejectTransmission()
		{
			_stream.reject();
		}

		TCPConvergenceLayer::TCPConnection& operator>>(TCPConvergenceLayer::TCPConnection &conn, dtn::data::Bundle &bundle)
		{
			try {
				((dtn::data::DefaultDeserializer&)conn) >> bundle;
			} catch (dtn::data::DefaultDeserializer::RejectedException ex) {
				// bundle rejected
				conn.rejectTransmission();
			}

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
				throw ex;
			}

			return conn;
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
				// firstly, do the handshake
				_connection.handshake();

				while (_running)
				{
					dtn::data::Bundle bundle;

					try {
						((dtn::data::DefaultDeserializer&)_connection) >> bundle;

						// raise default bundle received event
						dtn::net::BundleReceivedEvent::raise(EID(), bundle);

						// raise bundle event
						dtn::core::BundleEvent::raise(bundle, BUNDLE_RECEIVED);

					} catch (dtn::data::DefaultDeserializer::RejectedException ex) {
						// bundle rejected
						_connection.rejectTransmission();
					}

					yield();
				}
			} catch (...) {
				_running = false;
			}
		}

		void TCPConvergenceLayer::TCPConnection::Receiver::shutdown()
		{
			_running = false;
		}

		TCPConvergenceLayer::TCPConnection::Sender::Sender(TCPConnection &connection)
		 : _running(true), _connection(connection)
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
					dtn::data::Bundle bundle = blockingpop();

					// send bundle
					_connection << bundle;

					// idle a little bit
					yield();
				}
			} catch (...) {
				_running = false;
			}
		}

		void TCPConvergenceLayer::TCPConnection::Sender::shutdown()
		{
			{
				ibrcommon::MutexLock l(*this);
				_running = false;
				signal();
			}
		}
	}
}

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
#include "ibrcommon/TimeMeasurement.h"

#include "routing/RequeueBundleEvent.h"
#include "net/TransferCompletedEvent.h"

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
		 : dtn::data::DefaultDeserializer(_stream), _free(false), _tcpstream(stream), _stream(*this, *stream), _sender(*this), _receiver(*this), _name(), _timeout(0)
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

			ibrcommon::MutexLock l(_freemutex);
			iamfree();
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
			//throw dtn::data::DefaultDeserializer::RejectedException();
		}

		void TCPConvergenceLayer::TCPConnection::validate(const dtn::data::Block&, const size_t) const throw (dtn::data::DefaultDeserializer::RejectedException)
		{
//			if (length > 256)
//			{
//				throw dtn::data::DefaultDeserializer::RejectedException();
//			}
		}

		void TCPConvergenceLayer::TCPConnection::validate(const dtn::data::Bundle&) const throw (dtn::data::DefaultDeserializer::RejectedException)
		{
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

			try {
				// start the measurement
				m.start();

				// create a serializer
				dtn::data::DefaultSerializer serializer(conn._stream);

				// transmit the bundle
				serializer << bundle;

				// flush the stream
				conn._stream << std::flush;

				// stop the time measurement
				m.stop();

				// get throughput
				double kbytes_per_second = (serializer.getLength(bundle) / m.getSeconds()) / 1024;

				// print out throughput
				IBRCOMMON_LOGGER_DEBUG(15) << "transfer completed after " << m << " with "
						<< std::setiosflags(std::ios::fixed) << std::setprecision(2) << kbytes_per_second << " kb/s" << IBRCOMMON_LOGGER_ENDL;

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
				IBRCOMMON_LOGGER_DEBUG(15) << "transfer interrupted after " << m << " with "
						<< std::setiosflags(std::ios::fixed) << std::setprecision(2) << kbytes_per_second << " kb/s" << IBRCOMMON_LOGGER_ENDL;

				// TODO: the connection has been interrupted => create a fragment

				// signal interruption of the transfer
				dtn::routing::RequeueBundleEvent::raise(EID(conn._node.getURI()), bundle);

				// forward exception
				throw ex;
			} catch (ConnectionNotAvailableException ex) {
				// the connection not available
				IBRCOMMON_LOGGER_DEBUG(15) << "connection error => requeue the bundle" << IBRCOMMON_LOGGER_ENDL;
				IBRCOMMON_LOGGER_DEBUG(15) << "Exception: " << ex.what() << IBRCOMMON_LOGGER_ENDL;

				// signal interruption of the transfer
				dtn::routing::RequeueBundleEvent::raise(EID(conn._node.getURI()), bundle);

				// forward exception
				throw ex;
			} catch (ibrcommon::IOException ex) {
				// stop the time measurement
				m.stop();

				// the connection has been terminated and fragmentation is not possible => requeue the bundle
				IBRCOMMON_LOGGER_DEBUG(15) << "fragmentation is not possible => requeue the bundle" << IBRCOMMON_LOGGER_ENDL;
				IBRCOMMON_LOGGER_DEBUG(15) << "Exception: " << ex.what() << IBRCOMMON_LOGGER_ENDL;

				// signal interruption of the transfer
				dtn::routing::RequeueBundleEvent::raise(EID(conn._node.getURI()), bundle);

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

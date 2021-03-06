/*
 * StreamConnection.h
 *
 *  Created on: 01.07.2009
 *      Author: morgenro
 */

#ifndef STREAMCONNECTION_H_
#define STREAMCONNECTION_H_


#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/data/Exceptions.h"
#include "ibrdtn/streams/StreamContactHeader.h"
#include "ibrdtn/streams/StreamDataSegment.h"
#include <ibrcommon/thread/Mutex.h>
#include <ibrcommon/thread/MutexLock.h>
#include <ibrcommon/thread/Timer.h>
#include <ibrcommon/Exceptions.h>
#include <ibrcommon/thread/Queue.h>
#include <iostream>
#include <streambuf>

namespace dtn
{
	namespace streams
	{
		class StreamConnection : public iostream
		{
		public:
			enum ConnectionShutdownCases
			{
				CONNECTION_SHUTDOWN_NOTSET = 0,
				CONNECTION_SHUTDOWN_IDLE = 1,
				CONNECTION_SHUTDOWN_ERROR = 2,
				CONNECTION_SHUTDOWN_SIMPLE_SHUTDOWN = 3,
				CONNECTION_SHUTDOWN_NODE_TIMEOUT = 4,
				CONNECTION_SHUTDOWN_PEER_SHUTDOWN = 5
			};

			class TransmissionInterruptedException : public ibrcommon::IOException
			{
				public:
					TransmissionInterruptedException(const dtn::data::Bundle &bundle, const size_t position) throw()
					 : ibrcommon::IOException("Transmission was interrupted."), _bundle(bundle), _position(position)
					{
					};

					virtual ~TransmissionInterruptedException() throw ()
					{
					};

					const dtn::data::Bundle _bundle;
					const size_t _position;
			};

			class StreamClosedException : public ibrcommon::IOException
			{
			public:
				StreamClosedException(string what = "The stream has been closed.") throw() : IOException(what)
				{
				};
			};

			class StreamErrorException : public ibrcommon::IOException
			{
			public:
				StreamErrorException(string what = "StreamError") throw() : IOException(what)
				{
				};
			};

			class StreamShutdownException : public ibrcommon::IOException
			{
			public:
				StreamShutdownException(string what = "Shutdown message received.") throw() : IOException(what)
				{
				};
			};

			class Callback
			{
			public:
				/**
				 * This method is called if a SHUTDOWN message is
				 * received.
				 */
				virtual void eventShutdown(StreamConnection::ConnectionShutdownCases csc) = 0;

				/**
				 * This method is called if the stream is closed
				 * by a TIMEOUT.
				 */
				virtual void eventTimeout() = 0;

				/**
				 * This method is called if a error occured in the stream.
				 */
				virtual void eventError() = 0;

				/**
				 * This method is called if a bundle is refused by the peer.
				 */
				virtual void eventBundleRefused() = 0;
				/**
				 * This method is called if a bundle is refused by the peer.
				 */
				virtual void eventBundleForwarded() = 0;
				/**
				 * This method is called if a ACK is received.
				 */
				virtual void eventBundleAck(size_t ack) = 0;

				/**
				 * This method is called if a handshake was successful.
				 * @param header
				 */
				virtual void eventConnectionUp(const StreamContactHeader &header) = 0;

				/**
				 * This method is called if a connection went down.
				 */
				virtual void eventConnectionDown() = 0;
			};

			/**
			 * Constructor of the StreamConnection class
			 * @param cb Callback object for events of this stream
			 * @param stream The underlying stream object
			 */
			StreamConnection(StreamConnection::Callback &cb, iostream &stream, const size_t buffer_size = 4096);

			/**
			 * Destructor of the StreamConnection class
			 * @return
			 */
			virtual ~StreamConnection();

			/**
			 * Do a handshake with the other peer. This operation is needed to communicate.
			 * @param eid The local address of this node.
			 * @param timeout The desired timeout for this connection.
			 */
			void handshake(const dtn::data::EID &eid, const size_t timeout = 10, const char flags = 0);

			/**
			 * This method shutdown the whole connection handling process. To differ between the
			 * expected cases of disconnection a connection shutdown case is needed.
			 *
			 * CONNECTION_SHUTDOWN_IDLE
			 * The connection was idle and should go down now.
			 *
			 * CONNECTION_SHUTDOWN_ERROR
			 * A critical tcp error occured and the connection is closed.
			 *
			 * CONNECTION_SHUTDOWN_SIMPLE_SHUTDOWN
			 * The connection is ok, but a shutdown is requested from anywhere.
			 *
			 * CONNECTION_SHUTDOWN_NODE_TIMEOUT
			 * The node of this connection is gone. So shutdown this connection.
			 *
			 * CONNECTION_SHUTDOWN_PEER_SHUTDOWN
			 * The peer has shutdown this connection by a shutdown message.
			 *
			 * @param csc The case of the requested shutdown.
			 */
			void shutdown(ConnectionShutdownCases csc = CONNECTION_SHUTDOWN_SIMPLE_SHUTDOWN);

			/**
			 * This method rejects the currently transmitted bundle
			 */
			void reject();

			/**
			 * send a keepalive
			 */
			void keepalive();

			/**
			 * enables the idle timeout thread
			 * @param seconds
			 */
			void enableIdleTimeout(size_t seconds);

		private:
			/**
			 * stream buffer class
			 */
			class StreamBuffer : public std::basic_streambuf<char, std::char_traits<char> >, public ibrcommon::TimerCallback
			{
			public:
				enum State
				{
					INITIAL = 0,
					IDLE = 1,
					DATA_AVAILABLE = 2,
					DATA_TRANSFER = 3,
					SHUTDOWN = 4
				};

				/**
				 * constructor
				 */
				StreamBuffer(StreamConnection &conn, iostream &stream, const size_t buffer_size = 1024);
				virtual ~StreamBuffer();

				/**
				 * Do a handshake with the peer
				 * @param header The header to send.
				 * @return The received header.
				 */
				const StreamContactHeader handshake(const StreamContactHeader &header);

				/**
				 * close this stream immediately
				 */
				void close();

				/**
				 * send a shutdown message to the peer
				 */
				void shutdown(const StreamDataSegment::ShutdownReason reason = StreamDataSegment::MSG_SHUTDOWN_NONE);

				/**
				 * This method rejects the currently transmitted bundle
				 */
				void reject();

				/**
				 * Wait until all segments are acknowledged
				 */
				void wait();

				void abort();

				/**
				 * send a keepalive
				 */
				void keepalive();

				/**
				 * Idle timeout timer callback
				 * @param timer
				 * @return
				 */
				size_t timeout(ibrcommon::Timer *timer);

				/**
				 * enables the idle timeout thread
				 * @param seconds
				 */
				void enableIdleTimeout(size_t seconds);

			protected:
				virtual int sync();
				virtual int overflow(int = std::char_traits<char>::eof());
				virtual int underflow();

			private:
				/**
				 * @return True, if the stream is working.
				 */
				bool __good() const;

				/**
				 * print out the state of the stream
				 */
				void __error() const;

				enum timerNames
				{
					TIMER_IN = 1,
					TIMER_OUT = 2
				};

				enum StateBits
				{
					STREAM_FAILED = 1 << 0,
					STREAM_BAD = 1 << 1,
					STREAM_EOF = 1 << 2,
					STREAM_HANDSHAKE = 1 << 3,
					STREAM_SHUTDOWN = 1 << 4,
					STREAM_CLOSED = 1 << 5,
					STREAM_REJECT = 1 << 6,
					STREAM_SKIP = 1 << 7,
					STREAM_ACK_SUPPORT = 1 << 8,
					STREAM_NACK_SUPPORT = 1 << 9,
					STREAM_SOB = 1 << 10,			// start of bundle
					STREAM_TIMER_SUPPORT = 1 << 11
				};

				void skipData(size_t &size);

				bool get(const StateBits bit) const;
				void set(const StateBits bit);
				void unset(const StateBits bit);

				const size_t _buffer_size;

				ibrcommon::Mutex _statelock;
				int _statebits;

				StreamConnection &_conn;

				// Input buffer
				char *in_buf_;

				// Output buffer
				char *out_buf_;
				ibrcommon::Mutex _sendlock;

				std::iostream &_stream;

				size_t _recv_size;

				// this queue contains all sent data segments
				// they are removed if an ack or nack is received
				ibrcommon::Queue<StreamDataSegment> _segments;
				std::queue<StreamDataSegment> _rejected_segments;

				size_t _underflow_data_remain;
				State _underflow_state;

				ibrcommon::Timer _idle_timer;
			};

			void connectionTimeout();

			void eventShutdown(StreamConnection::ConnectionShutdownCases csc);

			void eventBundleAck(size_t ack);
			void eventBundleRefused();
			void eventBundleForwarded();

			StreamConnection::Callback &_callback;

			dtn::streams::StreamContactHeader _peer;

			StreamConnection::StreamBuffer _buf;

			ibrcommon::Mutex _shutdown_reason_lock;
			ConnectionShutdownCases _shutdown_reason;
		};
	}
}

#endif /* STREAMCONNECTION_H_ */


/*
 * StreamConnection.h
 *
 *  Created on: 01.07.2009
 *      Author: morgenro
 */

#ifndef STREAMCONNECTION_H_
#define STREAMCONNECTION_H_

#include "ibrdtn/default.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/data/Exceptions.h"
#include "ibrdtn/streams/StreamContactHeader.h"
#include "ibrdtn/streams/StreamDataSegment.h"
#include "ibrcommon/thread/Mutex.h"
#include "ibrcommon/thread/MutexLock.h"
#include "ibrcommon/thread/Timer.h"
#include "ibrcommon/thread/TimerCallback.h"
#include "ibrcommon/Exceptions.h"
#include <iostream>
#include <streambuf>
#include <queue>

namespace dtn
{
	namespace streams
	{
		class StreamConnection : public iostream
		{
		public:
			enum ConnectionState
			{
				CONNECTION_INITIAL = 0,
				CONNECTION_CONNECTED = 1,
				CONNECTION_SHUTDOWN = 2,
				CONNECTION_CLOSED = 3
			};

			class TransmissionInterruptedException : public ibrcommon::Exception
			{
				public:
					TransmissionInterruptedException(dtn::data::Bundle &bundle, size_t position) : ibrcommon::Exception("Transmission was interrupted.")
					{
					};
			};

			class StreamClosedException : public ibrcommon::Exception
			{
			public:
				StreamClosedException(string what = "The stream has been closed.") throw() : Exception(what)
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
				virtual void eventShutdown() = 0;

				/**
				 * This method is called if the stream is closed
				 * by a TIMEOUT.
				 */
				virtual void eventTimeout() = 0;

				/**
				 * This method is called if a handshake was successful.
				 * @param header
				 */
				virtual void eventConnectionUp(const StreamContactHeader &header) = 0;
			};

			/**
			 * Constructor of the StreamConnection class
			 * @param cb Callback object for events of this stream
			 * @param stream The underlying stream object
			 */
			StreamConnection(StreamConnection::Callback &cb, iostream &stream);

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
			 * Close this stream connection
			 */
			void close(bool force = false);

//			/**
//			 * Send a shutdown request to the peer and set a timer.
//			 */
//			void shutdown();

			/**
			 * Returns a variable which tells the connection status.
			 * @return True, if connected.
			 */
			bool isConnected();

//			/**
//			 * This method tells us, if the current data transfer has not acknowledged parts.
//			 * @return True, if no ACK is outstanding.
//			 */
//			bool isCompleted();
//
//			/**
//			 * Wait until all ACKs are received.
//			 * @return False, if a shutdown has been initiated but not all parts has been received.
//			 */
//			bool waitCompleted();
//
//			/**
//			 * Wait until the connection has been closed.
//			 */
//			void waitClosed();

			/**
			 * wait until all data has been acknowledged
			 * or the connection has been closed
			 */
			void wait();

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

				// The size of the input and output buffers.
				static const size_t BUFF_SIZE = 1024;

				/**
				 * constructor
				 */
				StreamBuffer(StreamConnection &conn, iostream &stream);
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
				void shutdown();

				/**
				 * This method is called by the timers.
				 * @param timer
				 * @return
				 */
				bool timeout(ibrcommon::Timer *timer);

			protected:
				virtual int sync();
				virtual int overflow(int = std::char_traits<char>::eof());
				virtual int underflow();

			private:
				void actionShutdown(const StreamDataSegment::ShutdownReason reason);
				void actionClose();
				void actionAck(size_t size);

				void actionConnectionTimeout();
				void actionShutdownTimeout();
				void actionKeepaliveTimeout();

				void readSegment();

				ibrcommon::StatefulConditional<StreamBuffer::State, StreamBuffer::SHUTDOWN> _in_state;
				ibrcommon::StatefulConditional<StreamBuffer::State, StreamBuffer::SHUTDOWN> _out_state;

				StreamConnection &_conn;

				// Input buffer
				char *in_buf_;
				// Output buffer
				char *out_buf_;

				std::iostream &_stream;

				size_t in_data_remain_;
				bool _start_of_bundle;

				size_t _sent_size;
				size_t _recv_size;

				ibrcommon::Timer _in_timer;
				ibrcommon::Timer _out_timer;
				ibrcommon::Timer _shutdown_timer;
			};

			void connectionTimeout();
			void shutdownTimeout();

			void eventShutdown();
			void eventAck(size_t ack, size_t sent);

			size_t _ack_size;
			size_t _sent_size;

			StreamConnection::StreamBuffer _buf;
			StreamConnection::Callback &_callback;

			ibrcommon::StatefulConditional<ConnectionState, CONNECTION_CLOSED> _in_state;
			ibrcommon::StatefulConditional<ConnectionState, CONNECTION_CLOSED> _out_state;

			dtn::streams::StreamContactHeader _peer;
		};
	}
}

#endif /* STREAMCONNECTION_H_ */


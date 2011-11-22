/*
 * DatagramConnection.h
 *
 *  Created on: 21.11.2011
 *      Author: morgenro
 */

#ifndef DATAGRAMCONNECTION_H_
#define DATAGRAMCONNECTION_H_

#include "net/ConvergenceLayer.h"
#include <ibrcommon/thread/Thread.h>
#include <ibrcommon/thread/Queue.h>
#include <ibrcommon/thread/Conditional.h>
#include <streambuf>
#include <iostream>
#include <stdint.h>

namespace dtn
{
	namespace net
	{
		class DatagramException : public ibrcommon::Exception
		{
		public:
			DatagramException(const std::string &what) : ibrcommon::Exception(what)
			{};

			virtual ~DatagramException() throw() {};
		};

		class DatagramConnection;

		class DatagramConnectionCallback
		{
		public:
			virtual ~DatagramConnectionCallback() {};
			virtual void callback_send(DatagramConnection &connection, const std::string &destination, const char *buf, int len) throw (DatagramException) = 0;
			virtual void callback_ack(DatagramConnection &connection, const std::string &destination, const char *buf, int len) throw (DatagramException) = 0;

			virtual void connectionUp(const DatagramConnection *conn) = 0;
			virtual void connectionDown(const DatagramConnection *conn) = 0;
		};

		class DatagramConnection : public ibrcommon::DetachedThread
		{
		public:
			DatagramConnection(const std::string &identifier, size_t maxmsglen, DatagramConnectionCallback &callback);
			virtual ~DatagramConnection();

			void run();
			void setup();
			void finally();

			const std::string& getIdentifier() const;

			/**
			 * Queue job for delivery to another node
			 * @param job
			 */
			void queue(const ConvergenceLayer::Job &job);

			/**
			 * queue data for delivery to the stream
			 * @param buf
			 * @param len
			 */
			void queue(const char *buf, int len);

			/**
			 * This method is called by the DatagramCL, if an ACK is received.
			 * @param seq
			 */
			void ack(const char *buf, int len);

		private:
			class Stream : public std::basic_streambuf<char, std::char_traits<char> >, public std::iostream
			{
			public:
				enum HEADER_FLAGS
				{
					SEGMENT_FIRST = 0x20,
					SEGMENT_LAST = 0x10,
					SEGMENT_MIDDLE = 0x00,
					SEQ_NUM_MASK = 0x07
				};

				Stream(DatagramConnection &conn, size_t maxmsglen);
				virtual ~Stream();

				/**
				 * Queueing data received from the CL worker thread for the LOWPANConnection
				 * @param buf Buffer with received data
				 * @param len Length of the buffer
				 */
				void queue(const char *buf, int len);

				void abort();

			protected:
				virtual int sync();
				virtual int overflow(int = std::char_traits<char>::eof());
				virtual int underflow();

			private:
				// buffer size and maximum message size
				const size_t _buf_size;

				// state for incoming segments
				char _in_state;

				// buffer for incoming data to queue
				// the underflow method will block until
				// this buffer contains any data
				char *_queue_buf;

				// the number of bytes available in the queue buffer
				int _queue_buf_len;

				// conditional to lock the queue buffer and the
				// corresponding length variable
				ibrcommon::Conditional _queue_buf_cond;

				// outgoing data from the upper layer is stored
				// here first and processed by the overflow() method
				char *_out_buf;

				// incoming data to deliver data to the upper layer
				// is stored in this buffer by the underflow() method
				char *_in_buf;

				// next expected incoming sequence number
				uint8_t in_seq_num_;

				// next outgoing sequence number
				uint8_t out_seq_num_;

				// this variable is set to true to shutdown
				// this stream
				bool _abort;

				// callback to the corresponding connection object
				DatagramConnection &_callback;
			};

			class Sender : public ibrcommon::JoinableThread
			{
			public:
				Sender(Stream &stream);
				~Sender();

				void run();
				bool __cancellation();

				ibrcommon::Queue<ConvergenceLayer::Job> queue;

			private:
				DatagramConnection::Stream &_stream;
			};

			void stream_send_ack(const char *buf) throw (DatagramException);
			void stream_send(const char *buf, int len) throw (DatagramException);

			DatagramConnectionCallback &_callback;
			bool _running;
			const std::string _identifier;
			DatagramConnection::Stream _stream;
			DatagramConnection::Sender _sender;

			ibrcommon::Conditional _ack_cond;
			int _last_ack;
		};
	} /* namespace data */
} /* namespace dtn */
#endif /* DATAGRAMCONNECTION_H_ */

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
		class DatagramConnection;

		class DatagramConnectionCallback
		{
		public:
			virtual ~DatagramConnectionCallback() {};
			virtual void callback_send(DatagramConnection &connection, const std::string &destination, const char *buf, int len) = 0;

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

				void close();

			protected:
				virtual int sync();
				virtual int overflow(int = std::char_traits<char>::eof());
				virtual int underflow();

			private:
				const size_t _buf_size;

				bool _in_first_segment;
				char _out_stat;

				// Input buffer
				char *in_buf_;
				int in_buf_len;
				bool in_buf_free;

				// Output buffer
				char *out_buf_;
				char *out2_buf_;

				// sequence number
				uint8_t in_seq_num_;
				uint8_t out_seq_num_;
				long out_seq_num_global;

				bool _abort;

				DatagramConnection &_callback;

				ibrcommon::Conditional in_buf_cond;
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

			void stream_send(const char *buf, int len);

			DatagramConnectionCallback &_callback;
			bool _running;
			const std::string _identifier;
			DatagramConnection::Stream _stream;
			DatagramConnection::Sender _sender;
		};
	} /* namespace data */
} /* namespace dtn */
#endif /* DATAGRAMCONNECTION_H_ */

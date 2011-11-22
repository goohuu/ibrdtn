/*
 * DatagramConnection.cpp
 *
 *  Created on: 21.11.2011
 *      Author: morgenro
 */

#include "net/DatagramConnection.h"
#include "net/BundleReceivedEvent.h"
#include "core/BundleEvent.h"
#include "net/TransferCompletedEvent.h"
#include "net/TransferAbortedEvent.h"
#include "routing/RequeueBundleEvent.h"
#include "core/BundleCore.h"

#include <ibrdtn/data/ScopeControlHopLimitBlock.h>
#include <ibrdtn/utils/Utils.h>
#include <ibrdtn/data/Serializer.h>

#include <ibrcommon/Logger.h>
#include <string.h>

namespace dtn
{
	namespace net
	{
		DatagramConnection::DatagramConnection(const std::string &identifier, size_t maxmsglen, DatagramConnectionCallback &callback)
		 : _callback(callback), _identifier(identifier), _stream(*this, maxmsglen), _sender(*this, _stream), _last_ack(-1), _wait_ack(-1)
		{
		}

		DatagramConnection::~DatagramConnection()
		{
		}

		void DatagramConnection::shutdown()
		{
			_stream.abort();
		}

		void DatagramConnection::run()
		{
			_callback.connectionUp(this);

			try {
				while(_stream.good())
				{
					try {
						dtn::data::DefaultDeserializer deserializer(_stream);
						dtn::data::Bundle bundle;
						deserializer >> bundle;

						IBRCOMMON_LOGGER_DEBUG(10) << "DatagramConnection::run"<< IBRCOMMON_LOGGER_ENDL;

						// determine sender
						EID sender;

						// increment value in the scope control hop limit block
						try {
							dtn::data::ScopeControlHopLimitBlock &schl = bundle.getBlock<dtn::data::ScopeControlHopLimitBlock>();
							schl.increment();
						} catch (const dtn::data::Bundle::NoSuchBlockFoundException&) { };

						// raise default bundle received event
						dtn::net::BundleReceivedEvent::raise(sender, bundle);

					} catch (const dtn::InvalidDataException &ex) {
						IBRCOMMON_LOGGER_DEBUG(10) << "Received a invalid bundle: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
					} catch (const ibrcommon::IOException&) {

					}
				}
			} catch (std::exception &ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "Thread died: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
			}
		}

		void DatagramConnection::setup()
		{
			_sender.start();
		}

		void DatagramConnection::finally()
		{
			// remove this connection from the connection list
			_callback.connectionDown(this);

			{
				ibrcommon::MutexLock l(_ack_cond);
				_ack_cond.abort();
			}
			_sender.stop();
			_sender.join();

			// clear the queue
			_sender.clearQueue();
		}

		const std::string& DatagramConnection::getIdentifier() const
		{
			return _identifier;
		}

		/**
		 * Queue job for delivery to another node
		 * @param job
		 */
		void DatagramConnection::queue(const ConvergenceLayer::Job &job)
		{
			IBRCOMMON_LOGGER_DEBUG(10) << "DatagramConnection::queue"<< IBRCOMMON_LOGGER_ENDL;
			_sender.queue.push(job);
		}

		/**
		 * queue data for delivery to the stream
		 * @param buf
		 * @param len
		 */
		void DatagramConnection::queue(const char *buf, int len)
		{
			_stream.queue(buf, len);

			// send ack for this message
			stream_send_ack(buf);
		}

		void DatagramConnection::ack(const char *buf, int len)
		{
			if (len > 0)
			{
				ibrcommon::MutexLock l(_ack_cond);
				int seq = (*buf) & Stream::SEQ_NUM_MASK;
				if (_wait_ack == seq)
				{
					_last_ack = seq;
					_ack_cond.signal(true);
					IBRCOMMON_LOGGER_DEBUG(20) << "DatagramConnection: ack received " << _last_ack << IBRCOMMON_LOGGER_ENDL;
				}
			}
		}

		void DatagramConnection::stream_send_ack(const char *buf) throw (DatagramException)
		{
			_callback.callback_ack(*this, getIdentifier(), buf, 1);
		}

		void DatagramConnection::stream_send(const char *buf, int len) throw (DatagramException)
		{
			_wait_ack = (*buf) & Stream::SEQ_NUM_MASK;

			// max. 5 retries
			for (int i = 0; i < 5; i++)
			{
				// send the datagram
				_callback.callback_send(*this, getIdentifier(), buf, len);

				// set timeout to 200 ms
				struct timespec ts;
				ibrcommon::Conditional::gettimeout(200, &ts);

				try {
					// wait here for an ACK
					ibrcommon::MutexLock l(_ack_cond);
					while (_last_ack != _wait_ack)
					{
						_ack_cond.wait(&ts);
					}

					// success!
					return;
				}
				catch (const ibrcommon::Conditional::ConditionalAbortException &e) { };
			}

			// transmission failed - abort the stream
			IBRCOMMON_LOGGER_DEBUG(20) << "DatagramConnection::stream_send: transmission failed - abort the stream" << IBRCOMMON_LOGGER_ENDL;
			throw DatagramException("transmission failed - abort the stream");
		}

		DatagramConnection::Stream::Stream(DatagramConnection &conn, size_t maxmsglen)
		 : std::iostream(this), _buf_size(maxmsglen), _in_state(SEGMENT_FIRST),
		   _queue_buf(new char[_buf_size]), _queue_buf_len(0), _out_buf(new char[_buf_size+2]), _in_buf(new char[_buf_size]),
		   in_seq_num_(0), out_seq_num_(0), _abort(false), _callback(conn)
		{
			// Initialize get pointer. This should be zero so that underflow
			// is called upon first read.
			setg(0, 0, 0);

			// mark the buffer for outgoing data as free
			// the +1 sparse the first byte in the buffer and leave room
			// for the processing flags of the segment
			setp(_out_buf + 1, _out_buf + _buf_size - 1);

			// set the sequence number for the first outgoing segment
			_out_buf[0] = 0x07 & out_seq_num_;

			// initialize the first byte with SEGMENT_FIRST flag
			_out_buf[0] |= SEGMENT_FIRST;
		}

		DatagramConnection::Stream::~Stream()
		{
			delete[] _queue_buf;
			delete[] _out_buf;
			delete[] _in_buf;
		}

		void DatagramConnection::Stream::queue(const char *buf, int len)
		{
			ibrcommon::MutexLock l(_queue_buf_cond);

			IBRCOMMON_LOGGER_DEBUG(10) << "DatagramConnection::Stream::queue"<< IBRCOMMON_LOGGER_ENDL;

			// Retrieve sequence number of frame
			unsigned int seq_num = buf[0] & SEQ_NUM_MASK;

			IBRCOMMON_LOGGER_DEBUG(45) << "Received frame sequence number: " << seq_num << IBRCOMMON_LOGGER_ENDL;

			// Check if the sequence number is what we expect
			if (in_seq_num_ != seq_num)
			{
				IBRCOMMON_LOGGER(error) << "Received frame with out of bound sequence number (" << seq_num << " expected " << (int)in_seq_num_ << ")"<< IBRCOMMON_LOGGER_ENDL;
				_abort = true;
				_queue_buf_cond.signal();
				return;
			}

			// check if this is the first segment since we expect a first segment
			if ((_in_state & SEGMENT_FIRST) && (!(buf[0] & SEGMENT_FIRST)))
			{
				IBRCOMMON_LOGGER(error) << "Received frame with wrong segment mark"<< IBRCOMMON_LOGGER_ENDL;
				_abort = true;
				_queue_buf_cond.signal();
				return;
			}
			// check if this is a second first segment without any previous last segment
			else if ((_in_state & SEGMENT_MIDDLE) && (buf[0] & SEGMENT_FIRST))
			{
				IBRCOMMON_LOGGER(error) << "Received frame with wrong segment mark"<< IBRCOMMON_LOGGER_ENDL;
				_abort = true;
				_queue_buf_cond.signal();
				return;
			}

			// if this is the last segment then...
			if (buf[0] & SEGMENT_LAST)
			{
				// ... expect a first segment as next
				_in_state = SEGMENT_FIRST;
			}
			else
			{
				// if this is not the last segment we expect everything
				// but a first segment
				_in_state = SEGMENT_MIDDLE;
			}

			// increment the sequence number
			in_seq_num_ = (in_seq_num_ + 1) % 8;

			// wait until the buffer is free
			while (_queue_buf_len > 0)
			{
				_queue_buf_cond.wait();
			}

			// copy the new data into the buffer, but leave out the first byte (header)
			::memcpy(_queue_buf, buf + 1, len - 1);

			// store the buffer length
			_queue_buf_len = len - 1;

			// notify waiting threads
			_queue_buf_cond.signal();
		}

		void DatagramConnection::Stream::abort()
		{
			ibrcommon::MutexLock l(_queue_buf_cond);

			while (_queue_buf_len > 0)
			{
				_queue_buf_cond.wait();
			}

			_abort = true;
			_queue_buf_cond.signal();
		}

		int DatagramConnection::Stream::sync()
		{
			IBRCOMMON_LOGGER_DEBUG(10) << "DatagramConnection::Stream::sync" << IBRCOMMON_LOGGER_ENDL;

			// Here we know we get the last segment. Mark it so.
			_out_buf[0] |= SEGMENT_LAST;

			int ret = std::char_traits<char>::eq_int_type(this->overflow(
					std::char_traits<char>::eof()), std::char_traits<char>::eof()) ? -1
					: 0;

			// initialize the first byte with SEGMENT_FIRST flag
			_out_buf[0] |= SEGMENT_FIRST;

			return ret;
		}

		int DatagramConnection::Stream::overflow(int c)
		{
			if (_abort) throw DatagramException("stream aborted");

			char *ibegin = _out_buf;
			char *iend = pptr();

			IBRCOMMON_LOGGER_DEBUG(10) << "DatagramConnection::Stream::overflow" << IBRCOMMON_LOGGER_ENDL;

			// mark the buffer for outgoing data as free
			// the +1 sparse the first byte in the buffer and leave room
			// for the processing flags of the segment
			setp(_out_buf + 1, _out_buf + _buf_size - 1);

			if (!std::char_traits<char>::eq_int_type(c, std::char_traits<char>::eof()))
			{
				*iend++ = std::char_traits<char>::to_char_type(c);
			}

			// bytes to send
			size_t bytes = (iend - ibegin);

			// if there is nothing to send, just return
			if (bytes == 0)
			{
				IBRCOMMON_LOGGER_DEBUG(10) << "DatagramConnection::Stream::overflow() nothing to sent" << IBRCOMMON_LOGGER_ENDL;
				return std::char_traits<char>::not_eof(c);
			}

			// Send segment to CL, use callback interface
			_callback.stream_send(_out_buf, bytes);

			// increment the sequence number for outgoing segments
			out_seq_num_ = (out_seq_num_ + 1) % 8;

			// reset the first byte of the next segment
			// set the sequence number for the next outgoing segment
			_out_buf[0] = 0x07 & out_seq_num_;

			return std::char_traits<char>::not_eof(c);
		}

		int DatagramConnection::Stream::underflow()
		{
			ibrcommon::MutexLock l(_queue_buf_cond);

			IBRCOMMON_LOGGER_DEBUG(10) << "DatagramConnection::Stream::underflow"<< IBRCOMMON_LOGGER_ENDL;

			while (_queue_buf_len == 0)
			{
				if (_abort) throw ibrcommon::Exception("stream aborted");
				_queue_buf_cond.wait();
			}

			// copy the queue buffer to an internal buffer
			::memcpy(_in_buf,_queue_buf, _queue_buf_len);

			// Since the input buffer content is now valid (or is new)
			// the get pointer should be initialized (or reset).
			setg(_in_buf, _in_buf, _in_buf + _queue_buf_len);

			// mark the queue buffer as free
			_queue_buf_len = 0;
			_queue_buf_cond.signal();

			return std::char_traits<char>::not_eof((unsigned char) _in_buf[0]);
		}

		DatagramConnection::Sender::Sender(DatagramConnection &conn, Stream &stream)
		 : _stream(stream), _connection(conn)
		{
		}

		DatagramConnection::Sender::~Sender()
		{
		}

		void DatagramConnection::Sender::run()
		{
			try {
				while(_stream.good())
				{
					_current_job = queue.getnpop(true);
					dtn::data::DefaultSerializer serializer(_stream);

					IBRCOMMON_LOGGER_DEBUG(10) << "DatagramConnection::Sender::run"<< IBRCOMMON_LOGGER_ENDL;

					dtn::core::BundleStorage &storage = dtn::core::BundleCore::getInstance().getStorage();

					// read the bundle out of the storage
					const dtn::data::Bundle bundle = storage.get(_current_job._bundle);

					// Put bundle into stringstream
					serializer << bundle; _stream.flush();
					// raise bundle event
					dtn::net::TransferCompletedEvent::raise(_current_job._destination, bundle);
					dtn::core::BundleEvent::raise(bundle, dtn::core::BUNDLE_FORWARDED);
					_current_job.clear();
				}
				// FIXME: Exit strategy when sending on socket failed. Like destroying the connection object
				// Also check what needs to be done when the node is not reachable (transfer requeue...)

				IBRCOMMON_LOGGER_DEBUG(10) << "DatagramConnection::Sender::run stream destroyed"<< IBRCOMMON_LOGGER_ENDL;
			} catch (std::exception &ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "Thread died: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
			}

			_connection.stop();
		}

		void DatagramConnection::Sender::clearQueue()
		{
			// requeue all bundles still queued
			try {
				while (true)
				{
					const ConvergenceLayer::Job job = queue.getnpop();

					// raise transfer abort event for all bundles without an ACK
					dtn::routing::RequeueBundleEvent::raise(job._destination, job._bundle);
				}
			} catch (const ibrcommon::QueueUnblockedException&) {
				// queue emtpy
			}
		}

		void DatagramConnection::Sender::finally()
		{
			// notify the aborted transfer of the last bundle
			if (_current_job._bundle != dtn::data::BundleID())
			{
				// putback job on the queue
				queue.push(_current_job);
			}
		}

		bool DatagramConnection::Sender::__cancellation()
		{
			queue.abort();
			return true;
		}
	} /* namespace data */
} /* namespace dtn */

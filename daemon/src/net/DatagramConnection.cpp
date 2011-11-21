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
		 : _callback(callback), _identifier(identifier), _stream(*this, maxmsglen), _sender(_stream)
		{
		}

		DatagramConnection::~DatagramConnection()
		{
		}

		void DatagramConnection::run()
		{
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
			_sender.stop();
			_sender.join();

			// remove this connection from the connection list
			_callback.connectionDown(this);
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
		}

		DatagramConnection::Stream::Stream(DatagramConnection &conn, size_t maxmsglen)
		 : std::iostream(this), _buf_size(maxmsglen), _in_first_segment(true), _out_stat(SEGMENT_FIRST),
		   in_buf_(new char[_buf_size]), in_buf_len(0), in_buf_free(true), out_buf_(new char[_buf_size+2]), out2_buf_(new char[_buf_size]),
		   in_seq_num_(0), out_seq_num_(0), out_seq_num_global(0), _abort(false), _callback(conn)
		{
			// Initialize get pointer. This should be zero so that underflow is called upon first read.
			setg(0, 0, 0);
			setp(out_buf_ + 1, out_buf_ + _buf_size - 1);
		}

		DatagramConnection::Stream::~Stream()
		{
			delete[] in_buf_;
			delete[] out_buf_;
			delete[] out2_buf_;
		}

		void DatagramConnection::Stream::queue(const char *buf, int len)
		{
			ibrcommon::MutexLock l(in_buf_cond);

			IBRCOMMON_LOGGER_DEBUG(10) << "DatagramConnection::Stream::queue"<< IBRCOMMON_LOGGER_ENDL;

			// Retrieve sequence number of frame
			unsigned int seq_num = buf[0] & SEQ_NUM_MASK;

			IBRCOMMON_LOGGER_DEBUG(45) << "Received frame sequence number: " << seq_num << IBRCOMMON_LOGGER_ENDL;

			// Check if the sequence number is what we expect
			if (in_seq_num_ != seq_num)
			{
				IBRCOMMON_LOGGER(error) << "Received frame with out of bound sequence number (" << seq_num << " expected " << (int)in_seq_num_ << ")"<< IBRCOMMON_LOGGER_ENDL;
				_abort = true;
				in_buf_cond.signal();
				return;
			}

			// increment the sequence number
			in_seq_num_ = (in_seq_num_ + 1) % 8;

			// check if this is the right segment
			if (_in_first_segment)
			{
				if (!(buf[0] & SEGMENT_FIRST)) return;
				if (!(buf[0] & SEGMENT_LAST)) _in_first_segment = false;
			}

			while (!in_buf_free)
			{
				in_buf_cond.wait();
			}

			memcpy(in_buf_, buf + 1, len - 1);
			in_buf_len = len - 1;
			in_buf_free = false;
			in_buf_cond.signal();
		}

		void DatagramConnection::Stream::abort()
		{
			ibrcommon::MutexLock l(in_buf_cond);

			while (!in_buf_free)
			{
				in_buf_cond.wait();
			}

			_abort = true;
			in_buf_cond.signal();
		}

		void DatagramConnection::Stream::close()
		{

		}

		int DatagramConnection::Stream::sync()
		{
			// Here we know we get the last segment. Mark it so.
			_out_stat |= SEGMENT_LAST;

			int ret = std::char_traits<char>::eq_int_type(this->overflow(
					std::char_traits<char>::eof()), std::char_traits<char>::eof()) ? -1
					: 0;

			IBRCOMMON_LOGGER_DEBUG(10) << "DatagramConnection::Stream::sync"<< IBRCOMMON_LOGGER_ENDL;

			return ret;
		}

		int DatagramConnection::Stream::overflow(int c)
		{
			char *ibegin = out_buf_;
			char *iend = pptr();

			IBRCOMMON_LOGGER_DEBUG(10) << "DatagramConnection::Stream::overflow"<< IBRCOMMON_LOGGER_ENDL;

			// mark the buffer as free
			setp(out_buf_ + 1, out_buf_ + _buf_size - 1);

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

			//FIXME: Should we write in the segment position here?
			out_buf_[0] = 0x07 & out_seq_num_;
			out_buf_[0] |= _out_stat;

			out_seq_num_global++;
			out_seq_num_ = (out_seq_num_ + 1) % 8;

			IBRCOMMON_LOGGER_DEBUG(10) << "DatagramConnection::Stream send segment " << (int)out_seq_num_ << " / " << out_seq_num_global << IBRCOMMON_LOGGER_ENDL;

			// Send segment to CL, use callback interface
			_callback.stream_send(out_buf_, bytes);

			if (_out_stat & SEGMENT_LAST)
			{
				// reset outgoing status byte
				_out_stat = SEGMENT_FIRST;
			}
			else
			{
				_out_stat = SEGMENT_MIDDLE;
			}

			return std::char_traits<char>::not_eof(c);
		}

		int DatagramConnection::Stream::underflow()
		{
			ibrcommon::MutexLock l(in_buf_cond);

			IBRCOMMON_LOGGER_DEBUG(10) << "DatagramConnection::Stream::underflow"<< IBRCOMMON_LOGGER_ENDL;

			while (in_buf_free)
			{
				if (_abort) throw ibrcommon::Exception("stream aborted");
				in_buf_cond.wait();
			}
			memcpy(out2_buf_ ,in_buf_, in_buf_len);
			in_buf_free = true;
			IBRCOMMON_LOGGER_DEBUG(10) << "DatagramConnection::Stream::underflow in_buf_free: " << in_buf_free << IBRCOMMON_LOGGER_ENDL;
			in_buf_cond.signal();

			// Since the input buffer content is now valid (or is new)
			// the get pointer should be initialized (or reset).
			setg(out2_buf_, out2_buf_, out2_buf_ + in_buf_len);

			return std::char_traits<char>::not_eof((unsigned char) out2_buf_[0]);
		}

		DatagramConnection::Sender::Sender(Stream &stream)
		 : _stream(stream)
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
					ConvergenceLayer::Job job = queue.getnpop(true);
					dtn::data::DefaultSerializer serializer(_stream);

					IBRCOMMON_LOGGER_DEBUG(10) << "DatagramConnection::Sender::run"<< IBRCOMMON_LOGGER_ENDL;

					dtn::core::BundleStorage &storage = dtn::core::BundleCore::getInstance().getStorage();

					// read the bundle out of the storage
					const dtn::data::Bundle bundle = storage.get(job._bundle);

					// Put bundle into stringstream
					serializer << bundle; _stream.flush();
					// raise bundle event
					dtn::net::TransferCompletedEvent::raise(job._destination, bundle);
					dtn::core::BundleEvent::raise(bundle, dtn::core::BUNDLE_FORWARDED);
				}
				// FIXME: Exit strategy when sending on socket failed. Like destroying the connection object
				// Also check what needs to be done when the node is not reachable (transfer requeue...)

				IBRCOMMON_LOGGER_DEBUG(10) << "DatagramConnection::Sender::run stream destroyed"<< IBRCOMMON_LOGGER_ENDL;
			} catch (std::exception &ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "Thread died: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
			}
		}

		bool DatagramConnection::Sender::__cancellation()
		{
			queue.abort();
			return true;
		}
	} /* namespace data */
} /* namespace dtn */

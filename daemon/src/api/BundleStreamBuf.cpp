/*
 * BundleStreamBuf.cpp
 *
 *  Created on: 17.11.2011
 *      Author: morgenro
 */

#include "BundleStreamBuf.h"
#include <ibrdtn/data/StreamBlock.h>
#include <ibrcommon/Logger.h>
#include <ibrcommon/TimeMeasurement.h>

namespace dtn
{
	namespace api
	{
		BundleStreamBuf::BundleStreamBuf(BundleStreamBufCallback &callback, size_t buffer, bool wait_seq_zero)
		 : _callback(callback), _in_buf(new char[BUFF_SIZE]), _out_buf(new char[BUFF_SIZE]),
		   _buffer(buffer), _chunk_payload(ibrcommon::BLOB::create()), _chunk_offset(0), _in_seq(0), _out_seq(0), _streaming(wait_seq_zero), _timeout_receive(0)
		{
			// Initialize get pointer.  This should be zero so that underflow is called upon first read.
			setg(0, 0, 0);
			setp(_in_buf, _in_buf + BUFF_SIZE - 1);
		};

		BundleStreamBuf::~BundleStreamBuf()
		{
			delete[] _in_buf;
			delete[] _out_buf;
		};

		int BundleStreamBuf::sync()
		{
			int ret = std::char_traits<char>::eq_int_type(this->overflow(
					std::char_traits<char>::eof()), std::char_traits<char>::eof()) ? -1
					: 0;

			// send the current chunk and clear it
			flushPayload();

			return ret;
		}

		int BundleStreamBuf::overflow(int c)
		{
			char *ibegin = _in_buf;
			char *iend = pptr();

			// mark the buffer as free
			setp(_in_buf, _in_buf + BUFF_SIZE - 1);

			if (!std::char_traits<char>::eq_int_type(c, std::char_traits<char>::eof()))
			{
				*iend++ = std::char_traits<char>::to_char_type(c);
			}

			// if there is nothing to send, just return
			if ((iend - ibegin) == 0)
			{
				return std::char_traits<char>::not_eof(c);
			}

			// copy data into the bundles payload
			BundleStreamBuf::append(_chunk_payload, _in_buf, iend - ibegin);

			// if size exceeds chunk limit, send it
			if (_chunk_payload.iostream().size() > _buffer)
			{
				flushPayload();
			}

			return std::char_traits<char>::not_eof(c);
		}

		void BundleStreamBuf::flushPayload()
		{
			// create an empty bundle
			dtn::data::Bundle b;

			dtn::data::StreamBlock &block = b.push_front<dtn::data::StreamBlock>();
			block.setSequenceNumber(_out_seq);

			// add tmp payload to the bundle
			b.push_back(_chunk_payload);

			// send the current chunk
			_callback.put(b);

			// and clear the payload
			_chunk_payload = ibrcommon::BLOB::create();

			// increment the sequence number
			_out_seq++;
		}

		void BundleStreamBuf::append(ibrcommon::BLOB::Reference &ref, const char* data, size_t length)
		{
			ibrcommon::BLOB::iostream stream = ref.iostream();
			(*stream).seekp(0, ios::end);
			(*stream).write(data, length);
		}

		int BundleStreamBuf::underflow()
		{
			// receive chunks until the next sequence number is received
			while (_chunks.empty())
			{
				// request the next bundle
				dtn::data::Bundle b = _callback.get(_timeout_receive);

				IBRCOMMON_LOGGER_DEBUG(40) << "BundleStreamBuf::underflow(): bundle received" << IBRCOMMON_LOGGER_ENDL;

				if (BundleStreamBuf::getSequenceNumber(b) >= _in_seq)
				{
					IBRCOMMON_LOGGER_DEBUG(40) << "BundleStreamBuf::underflow(): bundle accepted, seq. no. " << BundleStreamBuf::getSequenceNumber(b) << IBRCOMMON_LOGGER_ENDL;
					_chunks.insert(Chunk(b));
				}
			}

			ibrcommon::TimeMeasurement tm;
			tm.start();

			// while not the right sequence number received -> wait
			while ((_in_seq != (*_chunks.begin())._seq))
			{
				// request the next bundle
				dtn::data::Bundle b = _callback.get(_timeout_receive);
				IBRCOMMON_LOGGER_DEBUG(40) << "BundleStreamBuf::underflow(): bundle received" << IBRCOMMON_LOGGER_ENDL;

				if (BundleStreamBuf::getSequenceNumber(b) >= _in_seq)
				{
					IBRCOMMON_LOGGER_DEBUG(40) << "BundleStreamBuf::underflow(): bundle accepted, seq. no. " << BundleStreamBuf::getSequenceNumber(b) << IBRCOMMON_LOGGER_ENDL;
					_chunks.insert(Chunk(b));
				}

				tm.stop();
				if (((_timeout_receive > 0) && (tm.getSeconds() > _timeout_receive)) || !_streaming)
				{
					// skip the missing bundles and proceed with the last received one
					_in_seq = (*_chunks.begin())._seq;

					// set streaming to active
					_streaming = true;
				}
			}

			IBRCOMMON_LOGGER_DEBUG(40) << "BundleStreamBuf::underflow(): read the payload" << IBRCOMMON_LOGGER_ENDL;

			// get the first chunk in the buffer
			const Chunk &c = (*_chunks.begin());

			const dtn::data::PayloadBlock &payload = c._bundle.getBlock<dtn::data::PayloadBlock>();
			ibrcommon::BLOB::Reference r = payload.getBLOB();

			// get stream lock
			ibrcommon::BLOB::iostream stream = r.iostream();

			// jump to the offset position
			(*stream).seekg(_chunk_offset, ios::beg);

			// copy the data of the last received bundle into the buffer
			(*stream).read(_out_buf, BUFF_SIZE);

			// get the read bytes
			size_t bytes = (*stream).gcount();

			if ((*stream).eof())
			{
				// bundle consumed
		//		std::cerr << std::endl << "# " << c._seq << std::endl << std::flush;

				// delete the last chunk
				_chunks.erase(c);

				// reset the chunk offset
				_chunk_offset = 0;

				// increment sequence number
				_in_seq++;

				// if no more bytes are read, get the next bundle -> call underflow() recursive
				if (bytes == 0)
				{
					return underflow();
				}
			}
			else
			{
				// increment the chunk offset
				_chunk_offset += bytes;
			}

			// Since the input buffer content is now valid (or is new)
			// the get pointer should be initialized (or reset).
			setg(_out_buf, _out_buf, _out_buf + bytes);

			return std::char_traits<char>::not_eof((unsigned char) _out_buf[0]);
		}

		size_t BundleStreamBuf::getSequenceNumber(const dtn::data::Bundle &b)
		{
			try {
				const dtn::data::StreamBlock &block = b.getBlock<dtn::data::StreamBlock>();
				return block.getSequenceNumber();
			} catch (const dtn::data::Bundle::NoSuchBlockFoundException&) { }

			return 0;
		}

		BundleStreamBuf::Chunk::Chunk(const dtn::data::Bundle &b)
		 : _bundle(b), _seq(BundleStreamBuf::getSequenceNumber(b))
		{
		}

		BundleStreamBuf::Chunk::~Chunk()
		{
		}

		bool BundleStreamBuf::Chunk::operator==(const Chunk& other) const
		{
			return (_seq == other._seq);
		}

		bool BundleStreamBuf::Chunk::operator<(const Chunk& other) const
		{
			return (_seq < other._seq);
		}
	} /* namespace data */
} /* namespace dtn */

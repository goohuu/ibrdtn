/*
 * bpstreambuf.cpp
 *
 *  Created on: 14.07.2009
 *      Author: morgenro
 */

#include "ibrdtn/streams/StreamConnection.h"

namespace dtn
{
	namespace streams
	{
		StreamConnection::StreamBuffer::StreamBuffer(StreamConnection &conn, iostream &stream)
			: _conn(conn), in_buf_(new char[BUFF_SIZE]), out_buf_(new char[BUFF_SIZE]),
			  _stream(stream), _start_of_bundle(true), _in_state(INITIAL), _out_state(INITIAL)
		{
			// Initialize get pointer.  This should be zero so that underflow is called upon first read.
			setg(0, 0, 0);
			setp(out_buf_, out_buf_ + BUFF_SIZE - 1);
		}

		StreamConnection::StreamBuffer::~StreamBuffer()
		{
			// stop all timer
			_timer.remove(*this, TIMER_SHUTDOWN);
			_timer.remove(*this, TIMER_IN);
			_timer.remove(*this, TIMER_OUT);

			delete [] in_buf_;
			delete [] out_buf_;
		}

		void StreamConnection::StreamBuffer::actionShutdown(const StreamDataSegment::ShutdownReason reason)
		{
			if (_out_state.ifState(SHUTDOWN)) return;

			if (!_in_state.ifState(SHUTDOWN))
			{
				// return with shutdown, if the stream is wrong
				_stream << StreamDataSegment(reason) << std::flush;
			}

			// set out state to shutdown
			_out_state.setState(SHUTDOWN);

			// stop in/out timer
			_timer.remove(*this, TIMER_IN);
			_timer.remove(*this, TIMER_OUT);

			// set timer
			_timer.set(*this, TIMER_SHUTDOWN, 5);
		}

		void StreamConnection::StreamBuffer::actionClose()
		{
			//_stream.clear(iostream::badbit);

			// set in state to shutdown
			_in_state.setState(SHUTDOWN);

			// set out state to shutdown
			_out_state.setState(SHUTDOWN);
		}

		void StreamConnection::StreamBuffer::actionAck(size_t size)
		{
			// New data segment received. Send an ACK.
			_stream << StreamDataSegment(StreamDataSegment::MSG_ACK_SEGMENT, size) << std::flush;
		}

		void StreamConnection::StreamBuffer::actionConnectionTimeout()
		{
#ifdef DO_EXTENDED_DEBUG_OUTPUT
			cout << "Connection timed out" << endl;
#endif
			// send shutdown message
			actionShutdown(StreamDataSegment::MSG_SHUTDOWN_IDLE_TIMEOUT);

			// signal timeout
			_conn.connectionTimeout();
		}

		void StreamConnection::StreamBuffer::actionShutdownTimeout()
		{
#ifdef DO_EXTENDED_DEBUG_OUTPUT
			cout << "shutdown timer" << endl;
#endif
			{
				ibrcommon::MutexLock l(_in_state);
				// set in state to shutdown
				_in_state.setState(SHUTDOWN);
			}

			{
				ibrcommon::MutexLock l(_out_state);
				// set out state to shutdown
				_out_state.setState(SHUTDOWN);
			}

			// signal the timeout
			_conn.shutdownTimeout();
		}

		void StreamConnection::StreamBuffer::actionKeepaliveTimeout()
		{
			 _stream << StreamDataSegment() << std::flush;
		}

		const StreamContactHeader StreamConnection::StreamBuffer::handshake(const StreamContactHeader &header)
		{
			try {

				// transfer the local header
				_stream << header << std::flush;

				// receive the remote header
				StreamContactHeader peer;
				_stream >> peer;

				// read the timer values
				_in_timeout = header._keepalive;
				_out_timeout = peer._keepalive - 2;

				// activate timer
				_timer.set(*this, TIMER_IN, _in_timeout);
				_timer.set(*this, TIMER_OUT, _out_timeout);

				// start the timer
				_timer.start();

				// return the received header
				return peer;

			} catch (dtn::exceptions::InvalidDataException ex) {

				// shutdown the stream
				actionShutdown(StreamDataSegment::MSG_SHUTDOWN_VERSION_MISSMATCH);

				// close the stream
				close();

				// call the shutdown event
				_conn.eventShutdown();

				// forward the catched exception
				throw ex;
			}
		}

		void StreamConnection::StreamBuffer::shutdown()
		{
			actionShutdown(StreamDataSegment::MSG_SHUTDOWN_IDLE_TIMEOUT);
		}

		size_t StreamConnection::StreamBuffer::timeout(size_t identifier)
		{
			switch (identifier)
			{
			case TIMER_SHUTDOWN:
				actionShutdownTimeout();
				break;

			case TIMER_IN:
				actionConnectionTimeout();
				break;

			case TIMER_OUT:
				actionKeepaliveTimeout();
				return _out_timeout;
				break;
			}
		}

		void StreamConnection::StreamBuffer::close()
		{
			actionClose();
		}

		void StreamConnection::StreamBuffer::readSegment()
		{
			if (_in_state.ifState(SHUTDOWN)) return;

			// container for segment data
			dtn::streams::StreamDataSegment seg;

			// read the segment
			_stream >> seg;

			// check for read errors or end of stream
			if (!_stream.good())
			{
				_in_state.setState(SHUTDOWN);
				return;
			}

			// reset the incoming timer
			_timer.set(*this, TIMER_IN, _in_timeout);

			switch (seg._type)
			{
				case StreamDataSegment::MSG_DATA_SEGMENT:
				{
					if (seg._flags & 0x01)
					{
						_recv_size = seg._value;
					}
					else
					{
						_recv_size += seg._value;
					}

					// set the new data length
					in_data_remain_ = seg._value;

					// announce the new data block
					_in_state.setState(DATA_AVAILABLE);
					break;
				}

				case StreamDataSegment::MSG_ACK_SEGMENT:
				{
					_conn.eventAck(seg._value, _sent_size);
					break;
				}

				case StreamDataSegment::MSG_KEEPALIVE:
					break;

				case StreamDataSegment::MSG_REFUSE_BUNDLE:
					break;

				case StreamDataSegment::MSG_SHUTDOWN:
				{
					actionShutdown(StreamDataSegment::MSG_SHUTDOWN_IDLE_TIMEOUT);

					// close the connection
					actionClose();

					// call the shutdown event
					_conn.eventShutdown();
				}
			}
		}

		// This function is called when the output buffer is filled.
		// In this function, the buffer should be written to wherever it should
		// be written to (in this case, the streambuf object that this is controlling).
		int StreamConnection::StreamBuffer::overflow(int c)
		{
			ibrcommon::MutexLock l(_out_state);

			if (_out_state.ifState(SHUTDOWN) || !_stream.good())
			{
				_out_state.setState(SHUTDOWN);
				return traits_type::eof();
			}

			char *ibegin = out_buf_;
			char *iend = pptr();

			// mark the buffer as free
			setp(out_buf_, out_buf_ + BUFF_SIZE - 1);

			// append the last character
			if(!traits_type::eq_int_type(c, traits_type::eof())) {
				*iend++ = traits_type::to_char_type(c);
			}

			// if there is nothing to send, just return
			if ((iend - ibegin) == 0)
			{
				return traits_type::not_eof(c);
			}

			// wrap a segment around the data
			StreamDataSegment seg(StreamDataSegment::MSG_DATA_SEGMENT, (iend - ibegin));

			// set the start flag
			if (_start_of_bundle)
			{
				if (!(seg._flags & 0x01)) seg._flags += 0x01;
				_start_of_bundle = false;
				_sent_size = 0;
			}

			if (char_traits<char>::eq_int_type(c, char_traits<char>::eof()))
			{
				// set the end flag
				if (!(seg._flags & 0x02)) seg._flags += 0x02;
				_start_of_bundle = true;
			}

			// write the segment to the stream
			_stream << seg;
			_stream.write(out_buf_, seg._value);

			if (!_stream.good())
			{
				_out_state.setState(SHUTDOWN);
				return traits_type::eof();
			}

			// add size to outgoing size
			_sent_size += seg._value;

			return traits_type::not_eof(c);
		}

		// This is called to flush the buffer.
		// This is called when we're done with the file stream (or when .flush() is called).
		int StreamConnection::StreamBuffer::sync()
		{
			int ret = traits_type::eq_int_type(this->overflow(traits_type::eof()),
											traits_type::eof()) ? -1 : 0;

			// ... and flush.
			_stream.flush();

			return ret;
		}

		// Fill the input buffer.  This reads out of the streambuf.
		int StreamConnection::StreamBuffer::underflow()
		{
			ibrcommon::MutexLock l(_in_state);

			if (_in_state.ifState(SHUTDOWN))
			{
				return traits_type::eof();
			}

			// read segments until DATA is AVAILABLE
			while (!_in_state.ifState(DATA_AVAILABLE))
			{
				readSegment();
				if (_in_state.ifState(SHUTDOWN))
				{
					return traits_type::eof();
				}
			}

			// set state to DATA TRANSFER
			_in_state.setState(DATA_TRANSFER);

			// currently transferring data
			size_t readsize = BUFF_SIZE;
			if (in_data_remain_ < BUFF_SIZE) readsize = in_data_remain_;

			// here receive the data
			_stream.read(in_buf_, readsize);

			// check for read errors or end of stream
			if (_stream.eof())
			{
				_in_state.setState(SHUTDOWN);
				return traits_type::eof();
			}

			// Since the input buffer content is now valid (or is new)
			// the get pointer should be initialized (or reset).
			setg(in_buf_, in_buf_, in_buf_ + readsize);

			// adjust the remain counter
			in_data_remain_ -= readsize;

			// if the transmission is completed
			if (in_data_remain_ == 0)
			{
				ibrcommon::MutexLock l(_out_state);

				// New data segment received. Send an ACK.
				actionAck(_recv_size);

				// return to idle state
				_in_state.setState(IDLE);
			}

			return traits_type::not_eof(in_buf_[0]);
		}
	}
}

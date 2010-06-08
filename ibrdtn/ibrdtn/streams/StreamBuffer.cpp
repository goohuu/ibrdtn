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
			  _stream(stream), _start_of_bundle(true), _in_state(INITIAL), _out_state(INITIAL), _timer(), _ack_support(false)
		{
			// Initialize get pointer.  This should be zero so that underflow is called upon first read.
			setg(0, 0, 0);
			setp(out_buf_, out_buf_ + BUFF_SIZE - 1);
		}

		StreamConnection::StreamBuffer::~StreamBuffer()
		{
			// stop all timer
			_timer.removeAll();

			// clear the own buffer
			delete [] in_buf_;
			delete [] out_buf_;
		}

		/**
		 * This method do a handshake with the peer including send and receive
		 * the contact header and set the IN/OUT timer. If the handshake was not
		 * successful a SHUTDOWN message is sent, the eventShutdown() is called and
		 * a exception is thrown.
		 * @param header
		 * @return
		 */
		const StreamContactHeader StreamConnection::StreamBuffer::handshake(const StreamContactHeader &header)
		{
			// enable exceptions on the stream
			_stream.exceptions(std::ios::badbit | std::ios::eofbit);

			try {
				// transfer the local header
				_stream << header << std::flush;

				// receive the remote header
				StreamContactHeader peer;
				_stream >> peer;

				// enable/disable ACK support
				_ack_support = peer._flags & StreamContactHeader::REQUEST_ACKNOWLEDGMENTS;

				// read the timer values
				_in_timeout = header._keepalive * 2;
				_out_timeout = peer._keepalive - 2;

				// activate timer
				_timer.set(*this, TIMER_OUT, _out_timeout);
				_timer.set(*this, TIMER_IN, _in_timeout);

				// start the timer
				_timer.start();

				// return the received header
				return peer;

			} catch (dtn::InvalidDataException ex) {

				// shutdown the stream
				shutdown(StreamDataSegment::MSG_SHUTDOWN_VERSION_MISSMATCH);

				// call the shutdown event
				_conn.shutdown(CONNECTION_SHUTDOWN_ERROR);

				// forward the catched exception
				throw ex;
			} catch (ios_base::failure ex) {
				// EOF or BAD?
				throw StreamErrorException("handshake not completed");
			}
		}

		/**
		 * This method is called by the super class and shutdown the hole
		 * connection. While we have no access to the connection itself this
		 * method send a SHUTDOWN message and set this connection as closed.
		 */
		void StreamConnection::StreamBuffer::shutdown(const StreamDataSegment::ShutdownReason reason)
		{
			try {
				ibrcommon::MutexLock l(_out_state);
				// send a SHUTDOWN message
				_stream << StreamDataSegment(reason) << std::flush;
			} catch (ios_base::failure ex) {
				// EOF or BAD?
				throw StreamErrorException("can not send shutdown message");
			}
		}

		/**
		 * This method is called by the timer object.
		 * In this class we have two timer.
		 *
		 * TIMER_IN: is raised if no data was received for a specified amount of time.
		 * TIMER_OUT: is raised every x seconds to send a keepalive message
		 *
		 * @param identifier Identifier for the timer.
		 * @return The new value for this timer.
		 */
		size_t StreamConnection::StreamBuffer::timeout(size_t identifier)
		{
			switch (identifier)
			{
			case TIMER_IN:
				_conn.shutdown(CONNECTION_SHUTDOWN_IDLE);
				break;

			case TIMER_OUT:
				{
#ifdef DO_EXTENDED_DEBUG_OUTPUT
					std::cout << "KEEPALIVE timeout" << std::endl;
#endif
					try {
						ibrcommon::MutexLock l(_out_state);
						_stream << StreamDataSegment() << std::flush;
						return _out_timeout;
					} catch (ios_base::failure ex) {
						// EOF or BAD?
						return 0;
					}
				}
				break;
			}
		}

		void StreamConnection::StreamBuffer::close()
		{
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
		}

		// This function is called when the output buffer is filled.
		// In this function, the buffer should be written to wherever it should
		// be written to (in this case, the streambuf object that this is controlling).
		int StreamConnection::StreamBuffer::overflow(int c)
		{
			try {
				ibrcommon::MutexLock l(_out_state);

				if (_out_state.ifState(SHUTDOWN)) throw StreamClosedException();

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

				// add size to outgoing size
				_sent_size += seg._value;

				// fake ACK if no support of the other side is available
				if (!_ack_support)
				{
					_conn.eventAck(_sent_size, _sent_size);
				}

				return traits_type::not_eof(c);
			} catch (StreamClosedException ex) {
				_conn.shutdown(CONNECTION_SHUTDOWN_ERROR);
			} catch (StreamErrorException ex) {
				_conn.shutdown(CONNECTION_SHUTDOWN_ERROR);
			} catch (ios_base::failure ex) {
				// EOF or BAD?
				_conn.shutdown(CONNECTION_SHUTDOWN_ERROR);
			}

			return traits_type::eof();
		}

		// This is called to flush the buffer.
		// This is called when we're done with the file stream (or when .flush() is called).
		int StreamConnection::StreamBuffer::sync()
		{
			int ret = traits_type::eq_int_type(this->overflow(traits_type::eof()),
											traits_type::eof()) ? -1 : 0;

			try {
				// ... and flush.
				_stream.flush();
			} catch (ios_base::failure ex) {
				// EOF or BAD?
				_conn.shutdown(CONNECTION_SHUTDOWN_ERROR);
			}

			return ret;
		}

		// Fill the input buffer.  This reads out of the streambuf.
		int StreamConnection::StreamBuffer::underflow()
		{
			try {
				ibrcommon::MutexLock l(_in_state);

				// check if the connection is already closed
				if (_in_state.ifState(SHUTDOWN))
				{
					throw StreamClosedException();
				}

				// read segments until DATA is AVAILABLE
				while (!_in_state.ifState(DATA_AVAILABLE))
				{
					// container for segment data
					dtn::streams::StreamDataSegment seg;

					// read the segment
					_stream >> seg;

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
							throw StreamShutdownException();
						}
					}
				}

				// set state to DATA TRANSFER
				_in_state.setState(DATA_TRANSFER);

				// currently transferring data
				size_t readsize = BUFF_SIZE;
				if (in_data_remain_ < BUFF_SIZE) readsize = in_data_remain_;

				// here receive the data
				_stream.read(in_buf_, readsize);

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
					if (_ack_support)
					{
						_stream << StreamDataSegment(StreamDataSegment::MSG_ACK_SEGMENT, _recv_size) << std::flush;
					}

					// return to idle state
					_in_state.setState(IDLE);
				}

				return traits_type::not_eof(in_buf_[0]);
			} catch (StreamClosedException ex) {
				_conn.shutdown(CONNECTION_SHUTDOWN_ERROR);
			} catch (StreamErrorException ex) {
				_conn.shutdown(CONNECTION_SHUTDOWN_ERROR);
			} catch (StreamShutdownException ex) {
				_conn.shutdown(CONNECTION_SHUTDOWN_PEER_SHUTDOWN);
			} catch (ios_base::failure ex) {
				// EOF or BAD?
				_conn.shutdown(CONNECTION_SHUTDOWN_ERROR);
			}

			return traits_type::eof();
		}
	}
}

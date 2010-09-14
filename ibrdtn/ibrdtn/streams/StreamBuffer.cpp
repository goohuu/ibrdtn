/*
 * bpstreambuf.cpp
 *
 *  Created on: 14.07.2009
 *      Author: morgenro
 */

#include "ibrdtn/streams/StreamConnection.h"
#include <ibrcommon/Logger.h>

namespace dtn
{
	namespace streams
	{
		StreamConnection::StreamBuffer::StreamBuffer(StreamConnection &conn, iostream &stream)
			: _statebits(STREAM_SOB), _conn(conn), in_buf_(new char[BUFF_SIZE]), out_buf_(new char[BUFF_SIZE]), _stream(stream),
			  _recv_size(0), _timer(*this, 0), _underflow_data_remain(0), _underflow_state(IDLE)
		{
			// Initialize get pointer.  This should be zero so that underflow is called upon first read.
			setg(0, 0, 0);
			setp(out_buf_, out_buf_ + BUFF_SIZE - 1);
		}

		StreamConnection::StreamBuffer::~StreamBuffer()
		{
			// stop all timer
			_timer.remove();

			// clear the own buffer
			delete [] in_buf_;
			delete [] out_buf_;
		}

		bool StreamConnection::StreamBuffer::get(const StateBits bit) const
		{
			return (_statebits & bit);
		}

		void StreamConnection::StreamBuffer::set(const StateBits bit)
		{
			ibrcommon::MutexLock l(_statelock);
			_statebits |= bit;
		}

		void StreamConnection::StreamBuffer::unset(const StateBits bit)
		{
			ibrcommon::MutexLock l(_statelock);
			_statebits &= ~(bit);
		}

		bool StreamConnection::StreamBuffer::good() const
		{
			int badbits = STREAM_FAILED + STREAM_BAD + STREAM_EOF + STREAM_SHUTDOWN + STREAM_CLOSED;
			return !(badbits & _statebits) && _stream.good();
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
			// activate exceptions for this method
			_stream.exceptions(std::ios::badbit | std::ios::eofbit | std::ios::failbit);

			try {
				ibrcommon::MutexLock l(_sendlock);

				// transfer the local header
				_stream << header << std::flush;

				// receive the remote header
				StreamContactHeader peer;
				_stream >> peer;

				// enable/disable ACK/NACK support
				if (peer._flags & StreamContactHeader::REQUEST_ACKNOWLEDGMENTS) set(STREAM_ACK_SUPPORT);
				if (peer._flags & StreamContactHeader::REQUEST_NEGATIVE_ACKNOWLEDGMENTS) set(STREAM_NACK_SUPPORT);

				// set the incoming timer if set (> 0)
				if (peer._keepalive > 0)
				{
					set(STREAM_TIMER_SUPPORT);

					// read the timer values
					_in_timeout = header._keepalive * 2;
					_out_timeout = peer._keepalive - 2;

					// activate timer
					ibrcommon::MutexLock timerl(_timer_lock);
					_in_timeout_value = _in_timeout;
					_out_timeout_value = _out_timeout;
					_timer.set(1);

					// start the timer
					_timer.start();
				}

				// set handshake completed bit
				set(STREAM_HANDSHAKE);

				// return the received header
				return peer;

			} catch (std::exception) {
				// set failed bit
				set(STREAM_FAILED);

				// shutdown the stream
				shutdown(StreamDataSegment::MSG_SHUTDOWN_VERSION_MISSMATCH);

				// call the shutdown event
				_conn.shutdown(CONNECTION_SHUTDOWN_ERROR);

				// forward the catched exception
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
				ibrcommon::MutexLock l(_sendlock);
				// send a SHUTDOWN message
				_stream << StreamDataSegment(reason) << std::flush;
			} catch (std::exception) {
				// set failed bit
				set(STREAM_FAILED);

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
		size_t StreamConnection::StreamBuffer::timeout(size_t)
		{
			size_t out_timeout_value = 0;
			size_t in_timeout_value = 0;
			{
				ibrcommon::MutexLock timerl(_timer_lock);
				_out_timeout_value--;
				_in_timeout_value--;
				out_timeout_value = _out_timeout_value;
				in_timeout_value = _in_timeout_value;
			}

			if (out_timeout_value <= 0)
			{
				try {
					ibrcommon::MutexLock l(_sendlock);
					_stream << StreamDataSegment() << std::flush;
					IBRCOMMON_LOGGER_DEBUG(15) << "KEEPALIVE sent" << IBRCOMMON_LOGGER_ENDL;
				} catch (std::exception) {
					// set failed bit
					set(STREAM_FAILED);
					return 0;
				}

				ibrcommon::MutexLock timerl(_timer_lock);
				_out_timeout_value = _out_timeout;
			}

			if (in_timeout_value <= 0)
			{
				IBRCOMMON_LOGGER_DEBUG(15) << "KEEPALIVE timeout reached -> shutdown connection" << IBRCOMMON_LOGGER_ENDL;
				_conn.shutdown(CONNECTION_SHUTDOWN_IDLE);
				return 0;
			}

			return 1;
		}

		void StreamConnection::StreamBuffer::close()
		{
			// set shutdown bit
			set(STREAM_SHUTDOWN);
		}

		void StreamConnection::StreamBuffer::shutdowntimers()
		{
			// stop all timer
			_timer.remove();
		}

		void StreamConnection::StreamBuffer::reject()
		{
			// we have to reject the current transmission
			// so we have to discard all all data until the next segment with a start bit
			set(STREAM_REJECT);

			// set the current in buffer to zero
			// this should result in a underflow call on the next read
			setg(0, 0, 0);
		}

		bool StreamConnection::StreamBuffer::isCompleted()
		{
			size_t size = _segments.size();
			IBRCOMMON_LOGGER_DEBUG(15) << size << " segments to confirm" << IBRCOMMON_LOGGER_ENDL;
			return (size == 0);
		}

		// This function is called when the output buffer is filled.
		// In this function, the buffer should be written to wherever it should
		// be written to (in this case, the streambuf object that this is controlling).
		int StreamConnection::StreamBuffer::overflow(int c)
		{
			// check if shutdown is executed
			if (!good()) throw StreamErrorException();

			// lock this method and avoid concurrent calls
			ibrcommon::MutexLock l(_overflow_mutex);

			try {
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
				if (get(STREAM_SOB))
				{
					seg._flags |= StreamDataSegment::MSG_MARK_BEGINN;
					unset(STREAM_SKIP);
					unset(STREAM_SOB);
				}

				if (char_traits<char>::eq_int_type(c, char_traits<char>::eof()))
				{
					// set the end flag
					seg._flags |= StreamDataSegment::MSG_MARK_END;
					set(STREAM_SOB);
				}

				if (!get(STREAM_SKIP))
				{
					ibrcommon::MutexLock l(_sendlock);

					// put the segment into the queue
					if (get(STREAM_ACK_SUPPORT))
					{
						_segments.push(seg);
					}
					else if (seg._flags & StreamDataSegment::MSG_MARK_END)
					{
						// without ACK support we have to assume that a bundle is forwarded
						// when the last segment is sent.
						_conn.eventBundleForwarded();
					}

					// write the segment to the stream
					_stream << seg;
					_stream.write(out_buf_, seg._value);
				}

				return traits_type::not_eof(c);
			} catch (StreamClosedException ex) {
				// set failed bit
				set(STREAM_FAILED);

				IBRCOMMON_LOGGER_DEBUG(10) << "StreamClosedException in overflow()" << IBRCOMMON_LOGGER_ENDL;
				_conn.shutdown(CONNECTION_SHUTDOWN_ERROR);
			} catch (StreamErrorException ex) {
				// set failed bit
				set(STREAM_FAILED);

				IBRCOMMON_LOGGER_DEBUG(10) << "StreamErrorException in overflow()" << IBRCOMMON_LOGGER_ENDL;
				_conn.shutdown(CONNECTION_SHUTDOWN_ERROR);
			} catch (ios_base::failure ex) {
				// set failed bit
				set(STREAM_FAILED);

				IBRCOMMON_LOGGER_DEBUG(10) << "ios_base::failure in overflow()" << IBRCOMMON_LOGGER_ENDL;
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
				ibrcommon::MutexLock l(_sendlock);

				// ... and flush.
				_stream.flush();
			} catch (ios_base::failure ex) {
				// set failed bit
				set(STREAM_BAD);

				_conn.shutdown(CONNECTION_SHUTDOWN_ERROR);
			}

			return ret;
		}

		void StreamConnection::StreamBuffer::skipData(size_t &size)
		{
			// a temporary buffer
			char tmpbuf[BUFF_SIZE];

			try {
				//  and read until the next segment
				while (size > 0 && _stream.good())
				{
					size_t readsize = BUFF_SIZE;
					if (size < BUFF_SIZE) readsize = size;

					// to reject a bundle read all remaining data of this segment
					_stream.read(tmpbuf, readsize);

					// adjust the remain counter
					size -= readsize;
				}
			} catch (ios_base::failure ex) {
				_underflow_state = IDLE;
				throw StreamErrorException("read error");
			}
		}

		// Fill the input buffer.  This reads out of the streambuf.
		int StreamConnection::StreamBuffer::underflow()
		{
			// check if shutdown is executed
			if (!good()) throw StreamErrorException();

			// lock this method and avoid concurrent calls
			ibrcommon::MutexLock l(_underflow_mutex);

			try {
				if (_underflow_state == DATA_TRANSFER)
				{
					// on bundle reject
					if (get(STREAM_REJECT))
					{
						// send NACK on bundle reject
						{
							ibrcommon::MutexLock l(_sendlock);

							// send a SHUTDOWN message
							_stream << StreamDataSegment(StreamDataSegment::MSG_REFUSE_BUNDLE) << std::flush;
						}

						// skip data in this segment
						skipData(_underflow_data_remain);

						// return to idle state
						_underflow_state = IDLE;
					}
					// send ACK if the data segment is received completely
					else if (_underflow_data_remain == 0)
					{
						// New data segment received. Send an ACK.
						if (get(STREAM_ACK_SUPPORT))
						{
							ibrcommon::MutexLock l(_sendlock);
							_stream << StreamDataSegment(StreamDataSegment::MSG_ACK_SEGMENT, _recv_size) << std::flush;
						}

						// return to idle state
						_underflow_state = IDLE;
					}
				}

				// read segments until DATA is AVAILABLE
				while (_underflow_state == IDLE)
				{
					// container for segment data
					dtn::streams::StreamDataSegment seg;

					try {
						// read the segment
						_stream >> seg;
					} catch (ios_base::failure ex) {
						throw StreamErrorException("read error");
					}

					// reset the incoming timer
					{
						ibrcommon::MutexLock timerl(_timer_lock);
						_in_timeout_value = _in_timeout;
					}

					switch (seg._type)
					{
						case StreamDataSegment::MSG_DATA_SEGMENT:
						{
							if (seg._flags & StreamDataSegment::MSG_MARK_BEGINN)
							{
								_recv_size = seg._value;
								unset(STREAM_REJECT);
							}
							else
							{
								_recv_size += seg._value;
							}

							// set the new data length
							_underflow_data_remain = seg._value;

							if (get(STREAM_REJECT))
							{
								// send NACK on bundle reject
								{
									// lock for sending
									ibrcommon::MutexLock l(_sendlock);

									// send a NACK message
									_stream << StreamDataSegment(StreamDataSegment::MSG_REFUSE_BUNDLE, 0) << std::flush;
								}

								// skip data in this segment
								skipData(_underflow_data_remain);
							}
							else
							{
								// announce the new data block
								_underflow_state = DATA_TRANSFER;
							}
							break;
						}

						case StreamDataSegment::MSG_ACK_SEGMENT:
						{
							// remove the segment in the queue
							if (get(STREAM_ACK_SUPPORT))
							{
								ibrcommon::LockedQueue<StreamDataSegment> q = _segments.LockedAccess();

								if ((*q).empty())
								{
									IBRCOMMON_LOGGER(error) << "got an unexpected ACK with size of " << seg._value << IBRCOMMON_LOGGER_ENDL;
								}
								else
								{
									StreamDataSegment &qs = (*q).front();

									if (qs._flags & StreamDataSegment::MSG_MARK_END)
									{
										_conn.eventBundleForwarded();
									}

									(*q).pop();

									_conn.eventBundleAck(seg._value);
								}
							}
							break;
						}

						case StreamDataSegment::MSG_KEEPALIVE:
							break;

						case StreamDataSegment::MSG_REFUSE_BUNDLE:
						{
							// remove the segment in the queue
							if (get(STREAM_ACK_SUPPORT) && get(STREAM_NACK_SUPPORT))
							{
								ibrcommon::LockedQueue<StreamDataSegment> q = _segments.LockedAccess();

								// skip segments
								if (!_rejected_segments.empty())
								{
									_rejected_segments.pop();

									// we received a NACK
									IBRCOMMON_LOGGER_DEBUG(30) << "NACK received, still " << _rejected_segments.size() << " segments to NACK" << IBRCOMMON_LOGGER_ENDL;
								}
								else if ((*q).empty())
								{
									IBRCOMMON_LOGGER(error) << "got an unexpected NACK" << IBRCOMMON_LOGGER_ENDL;
								}
								else
								{
									// we received a NACK
									IBRCOMMON_LOGGER_DEBUG(20) << "NACK received!" << IBRCOMMON_LOGGER_ENDL;

									// remove the next segment
									(*q).pop();

									// get all segment ACKs in the queue for this transmission
									while (!(*q).empty())
									{
										if ((*q).front()._flags & StreamDataSegment::MSG_MARK_BEGINN)
										{
											break;
										}

										// move the segments to another queue
										_rejected_segments.push((*q).front());
										(*q).pop();
									}

									// call event reject
									_conn.eventBundleRefused();

									// we received a NACK
									IBRCOMMON_LOGGER_DEBUG(30) << _rejected_segments.size() << " segments to NACK" << IBRCOMMON_LOGGER_ENDL;

									// the queue is empty, then skip the current transfer
									if ((*q).empty())
									{
										set(STREAM_SKIP);

										// we received a NACK
										IBRCOMMON_LOGGER_DEBUG(25) << "skip the current transfer" << IBRCOMMON_LOGGER_ENDL;
									}
								}
							}
							else
							{
								IBRCOMMON_LOGGER(error) << "got an unexpected NACK" << IBRCOMMON_LOGGER_ENDL;
							}

							break;
						}

						case StreamDataSegment::MSG_SHUTDOWN:
						{
							throw StreamShutdownException();
						}
					}
				}

				// currently transferring data
				size_t readsize = BUFF_SIZE;
				if (_underflow_data_remain < BUFF_SIZE) readsize = _underflow_data_remain;

				try {
					// here receive the data
					_stream.read(in_buf_, readsize);
				} catch (ios_base::failure ex) {
					_underflow_state = IDLE;
					throw StreamErrorException("read error");
				}

				// adjust the remain counter
				_underflow_data_remain -= readsize;

				// Since the input buffer content is now valid (or is new)
				// the get pointer should be initialized (or reset).
				setg(in_buf_, in_buf_, in_buf_ + readsize);

				return traits_type::not_eof(in_buf_[0]);

			} catch (StreamClosedException ex) {
				// set failed bit
				set(STREAM_FAILED);

				IBRCOMMON_LOGGER_DEBUG(10) << "StreamClosedException in underflow()" << IBRCOMMON_LOGGER_ENDL;
				_conn.shutdown(CONNECTION_SHUTDOWN_ERROR);
			} catch (StreamErrorException ex) {
				// set failed bit
				set(STREAM_FAILED);

				IBRCOMMON_LOGGER_DEBUG(10) << "StreamErrorException in underflow(): " << ex.what() << IBRCOMMON_LOGGER_ENDL;
				_conn.shutdown(CONNECTION_SHUTDOWN_ERROR);
			} catch (StreamShutdownException ex) {
				// set failed bit
				set(STREAM_FAILED);

				IBRCOMMON_LOGGER_DEBUG(10) << "StreamShutdownException in underflow()" << IBRCOMMON_LOGGER_ENDL;
				_conn.shutdown(CONNECTION_SHUTDOWN_PEER_SHUTDOWN);
			}

			return traits_type::eof();
		}
	}
}

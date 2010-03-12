/*
 * StreamConnection.cpp
 *
 *  Created on: 02.07.2009
 *      Author: morgenro
 */

#include "ibrdtn/streams/StreamConnection.h"

using namespace dtn::data;

namespace dtn
{
	namespace streams
	{
		StreamConnection::StreamConnection(std::iostream &stream)
		 : _buf(stream), std::iostream(&_buf), _recv_size(0), _ack_size(0), _in_timer(NULL), _out_timer(NULL),
		   _in_state(CONNECTION_IDLE), _out_state(CONNECTION_IDLE)
		{
		}

		StreamConnection::~StreamConnection()
		{
			shutdownTimer();
			join();
#ifdef DO_EXTENDED_DEBUG_OUTPUT
			cout << "remove StreamConnection" << endl;
#endif
		}

		void StreamConnection::shutdownTimer()
		{
			if (_in_timer != NULL)
			{
				_in_timer->stop();
				delete _in_timer;
				_in_timer = NULL;
			}

			if (_out_timer != NULL)
			{
				_out_timer->stop();
				delete _out_timer;
				_out_timer = NULL;
			}
		}

		void StreamConnection::setTimer(size_t in_timeout, size_t out_timeout)
		{
			_in_timer = new ibrcommon::Timer(*this, in_timeout);
			_out_timer = new ibrcommon::Timer(*this, out_timeout);

			_in_timer->start();
			_out_timer->start();
		}

		void StreamConnection::close()
		{
			// and close the connection
			_buf.shutdown();

			// connection closed
			{
				ibrcommon::MutexLock l(_in_state);
				_in_state.setState(CONNECTION_CLOSED);
			}

			{
				ibrcommon::MutexLock l(_out_state);
				_out_state.setState(CONNECTION_CLOSED);
			}
		}

		void StreamConnection::shutdown()
		{
			{
				ibrcommon::MutexLock l(_out_state);
				_out_state.setState(CONNECTION_SHUTDOWN_REQUEST);
				_buf << StreamDataSegment(StreamDataSegment::MSG_SHUTDOWN_NONE); flush();
			}

			{
				ibrcommon::MutexLock l(_in_state);
				_in_state.waitState(CONNECTION_SHUTDOWN_REQUEST);
			}

			close();
			eventShutdown();
		}

		bool StreamConnection::isCompleted()
		{
			ibrcommon::MutexLock l(_in_state);

			if (_buf.getOutSize() == _ack_size)
			{
				return true;
			}

			return false;
		}

		bool StreamConnection::waitCompleted()
		{
			ibrcommon::MutexLock l(_in_state);
#ifdef DO_EXTENDED_DEBUG_OUTPUT
			cout << "wait for completion of transmission, current size: " << _ack_size << " of " << _buf.getOutSize() << endl;
#endif
			while (_buf.getOutSize() != _ack_size)
			{
				_in_state.wait();
#ifdef DO_EXTENDED_DEBUG_OUTPUT
				cout << "current size: " << _ack_size << " of " << _buf.getOutSize() << endl;
#endif
			}

			if (_buf.getOutSize() == _ack_size) return true;
			return false;
		}

		void StreamConnection::waitClosed()
		{
			ibrcommon::MutexLock l(_in_state);
			_in_state.waitState(CONNECTION_CLOSED);
		}

		void StreamConnection::run()
		{
			while (!eof())
			{
				try {
					// container for segment data
					dtn::streams::StreamDataSegment seg;

					{
						ibrcommon::MutexLock l(_in_state);
						// read the segment
						_buf >> seg;
					}

					// reset the timer
					if (_in_timer != NULL) _in_timer->reset();

					switch (seg._type)
					{
						case StreamDataSegment::MSG_DATA_SEGMENT:
						{
							ibrcommon::MutexLock l(_in_state);

							if (seg._flags & 0x01)
							{
								_in_state.setState(CONNECTION_RECEIVING);
								_recv_size = seg._value;
							}
							else
							{
								_recv_size += seg._value;
							}

							// New data segment received. Send an ACK.
							_buf << StreamDataSegment(StreamDataSegment::MSG_ACK_SEGMENT, _recv_size); flush();

							// flush if this is the last segment
							if (seg._flags & 0x02)
							{
								flush();
								_in_state.setState(CONNECTION_IDLE);
							}
							break;
						}

						case StreamDataSegment::MSG_ACK_SEGMENT:
						{
							ibrcommon::MutexLock l(_in_state);

							_ack_size = seg._value;
#ifdef DO_EXTENDED_DEBUG_OUTPUT
							cout << "ACK received: " << _ack_size << endl;
#endif
							_in_state.signal(true);
							break;
						}

						case StreamDataSegment::MSG_KEEPALIVE:
							break;

						case StreamDataSegment::MSG_REFUSE_BUNDLE:
							break;

						case StreamDataSegment::MSG_SHUTDOWN:
						{
							ibrcommon::MutexLock l(_in_state);

							// we received a shutdown request
							_in_state.setState(CONNECTION_SHUTDOWN_REQUEST);

							{
								ibrcommon::MutexLock l(_out_state);
								_out_state.setState(CONNECTION_SHUTDOWN_REQUEST);
								_buf << StreamDataSegment(StreamDataSegment::MSG_SHUTDOWN_NONE); flush();
							}
						}
						close();
						eventShutdown();
						break;
					}

				}
				catch (dtn::exceptions::IOException ex)
				{
					// Connection terminated
#ifdef DO_EXTENDED_DEBUG_OUTPUT
					cout << "IOException: " << ex.what() << endl;
#endif

					// close the connection
					close();

					break;
				}

				// breakpoint to stop this thread
				yield();
			}
		}

		/**
		 * This method is called if a timer is fired.
		 */
		bool StreamConnection::timeout(ibrcommon::Timer *timer)
		{
			if (timer == _out_timer)
			{
				(*this) << StreamDataSegment(); flush();
				return true;
			}
			else if (timer == _in_timer)
			{
#ifdef DO_EXTENDED_DEBUG_OUTPUT
				cout << "Connection timed out" << endl;
#endif
				ibrcommon::MutexLock l(_out_state);

				if (_out_state.ifState(CONNECTION_SHUTDOWN_REQUEST))
				{
					// and close the connection
					_buf.shutdown();

					// connection closed
					_out_state.setState(CONNECTION_CLOSED);

					// call superclass
					eventTimeout();

					// return false to stop the timer
					return false;
				}
				else
				{
					// we received a shutdown request
					_out_state.setState(CONNECTION_SHUTDOWN_REQUEST);

					// reply with a shutdown
					_buf << StreamDataSegment(StreamDataSegment::MSG_SHUTDOWN_IDLE_TIMEOUT); flush();

					// return true, to reset the timer
					return true;
				}
			}
		}
	}
}

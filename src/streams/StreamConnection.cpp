/*
 * StreamConnection.cpp
 *
 *  Created on: 02.07.2009
 *      Author: morgenro
 */

#include "ibrdtn/streams/StreamConnection.h"
#include "ibrdtn/data/BLOBManager.h"

using namespace dtn::data;

namespace dtn
{
	namespace streams
	{
		StreamConnection::StreamConnection(std::iostream &stream, size_t timeout)
		 : _buf(stream), std::iostream(&_buf), _recv_size(0), _ack_size(0), _state(CONNECTION_IDLE), _in_timer(NULL), _out_timer(NULL)
		{
		}

		StreamConnection::~StreamConnection()
		{
                    if (_in_timer != NULL)
                    {
                        _in_timer->stop();
                        delete _in_timer;
                    }

                    if (_out_timer != NULL)
                    {
			_out_timer->stop();
                        delete _out_timer;
                    }

                    join();
#ifdef DO_EXTENDED_DEBUG_OUTPUT
                    cout << "remove StreamConnection" << endl;
#endif
		}

                void StreamConnection::setTimer(size_t in_timeout, size_t out_timeout)
                {
                    _in_timer = new dtn::utils::Timer(*this, in_timeout);
                    _out_timer = new dtn::utils::Timer(*this, out_timeout);

                    _in_timer->start();
                    _out_timer->start();
                }

                void StreamConnection::setState(ConnectionState conn)
                {
                    dtn::utils::MutexLock l(_state_cond);

                    // the closed state is a final state
                    if (_state == CONNECTION_CLOSED) return;

                    // block states smaller than SHUTDOWN, if a shutdown is in progress.
                    if ((_state >= CONNECTION_SHUTDOWN) && (conn < CONNECTION_SHUTDOWN)) return;

#ifdef DO_EXTENDED_DEBUG_OUTPUT
                    cout << "StreamConnection state changed from " << _state << " to " << conn << endl;
#endif
                    _state = conn;
                    _state_cond.signal(true);

                    if (_state >= CONNECTION_SHUTDOWN)
                    {
                        dtn::utils::MutexLock l(_completed_cond);
                        _completed_cond.signal(true);
                    }
                }

                StreamConnection::ConnectionState StreamConnection::getState()
                {
                    dtn::utils::MutexLock l(_state_cond);
                    return _state;
                }

                bool StreamConnection::waitState(ConnectionState conn)
                {
                    dtn::utils::MutexLock l(_state_cond);
                    while ((conn != _state) && (_state != CONNECTION_CLOSED))
                        _state_cond.wait();

                    if (conn == _state) return true;

                    return false;
                }

		bool StreamConnection::good()
		{
                    // only return good if shutdown is not set.
                    return (iostream::good() && (getState() < CONNECTION_SHUTDOWN));
		}

		void StreamConnection::shutdown()
		{
                    if (getState() < CONNECTION_SHUTDOWN)
                    {
                        (*this) << StreamDataSegment(StreamDataSegment::MSG_SHUTDOWN_NONE); flush();
                    }
		}

                bool StreamConnection::isCompleted()
                {
                    dtn::utils::MutexLock l(_completed_cond);
                    
                    if (_buf.getOutSize() == _ack_size)
                    {
                        return true;
                    }

                    return false;
                }

                bool StreamConnection::waitCompleted()
                {
                    dtn::utils::MutexLock l(_completed_cond);
#ifdef DO_EXTENDED_DEBUG_OUTPUT
                    cout << "wait for completion of transmission, current size: " << _ack_size << " of " << _buf.getOutSize() << endl;
#endif
                    while ((_buf.getOutSize() != _ack_size) && (getState() < CONNECTION_SHUTDOWN))
                    {
                        _completed_cond.wait();
#ifdef DO_EXTENDED_DEBUG_OUTPUT
                        cout << "current size: " << _ack_size << " of " << _buf.getOutSize() << endl;
#endif
                    }

                    if (_buf.getOutSize() == _ack_size) return true;
                    return false;
                }

		void StreamConnection::run()
		{
			while (good())
			{
				try {
					// container for segment data
					dtn::streams::StreamDataSegment seg;

					// read the segment
					(*this) >> seg;

					// reset the timer
					if (_in_timer != NULL) _in_timer->reset();

					switch (seg._type)
					{
						case StreamDataSegment::MSG_DATA_SEGMENT:
						{
                                                        if (seg._flags & 0x04)
                                                        {
                                                            setState(CONNECTION_RECEIVING);
                                                            _recv_size = seg._value;
                                                        }
                                                        else
                                                        {
                                                            _recv_size += seg._value;
                                                        }

							// New data segment received. Send a ACK.
							(*this) << StreamDataSegment(StreamDataSegment::MSG_ACK_SEGMENT, _recv_size);

                                                        // flush if this ack is the last segment
                                                        if (seg._flags & 0x08)
                                                        {
                                                            flush();
                                                            setState(CONNECTION_IDLE);
                                                        }
							break;
						}

						case StreamDataSegment::MSG_ACK_SEGMENT:
                                                {
                                                        dtn::utils::MutexLock l(_completed_cond);
							_ack_size = seg._value;
#ifdef DO_EXTENDED_DEBUG_OUTPUT
                                                        cout << "ACK received: " << _ack_size << endl;
#endif
                                                        _completed_cond.signal(true);
							break;
                                                }

						case StreamDataSegment::MSG_KEEPALIVE:
							break;

						case StreamDataSegment::MSG_REFUSE_BUNDLE:
							break;

						case StreamDataSegment::MSG_SHUTDOWN:
                                                {
                                                    if (getState() < CONNECTION_SHUTDOWN)
                                                    {
                                                        // we received a shutdown request
                                                        setState(CONNECTION_SHUTDOWN);

                                                        // reply with a shutdown
                                                        (*this) << StreamDataSegment(StreamDataSegment::MSG_SHUTDOWN_NONE); flush();
                                                    }
                                                    
                                                    // and close the connection
                                                    _buf.shutdown();

                                                    // connection closed
                                                    setState(CONNECTION_CLOSED);

                                                    // call the shutdown method of the derived class
                                                    shutdown();

                                                    break;
                                                }
					}

				}
                                catch (dtn::exceptions::IOException ex)
                                {
					// Connection terminated
#ifdef DO_EXTENDED_DEBUG_OUTPUT
					cout << "IOException: " << ex.what() << endl;
#endif
					// close the connection
                                        _buf.shutdown();

                                        // connection closed
                                        setState(CONNECTION_CLOSED);

                                        // call the shutdown method of the derived class
                                        shutdown();
					break;
				}

				// breakpoint to stop this thread
				yield();
			}
		}

                /**
                 * This method is called if a timer is fired.
                 */
		bool StreamConnection::timeout(dtn::utils::Timer *timer)
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
                                if (getState() < CONNECTION_SHUTDOWN)
                                {
                                    // we received a shutdown request
                                    setState(CONNECTION_SHUTDOWN);

                                    // reply with a shutdown
                                    (*this) << StreamDataSegment(StreamDataSegment::MSG_SHUTDOWN_IDLE_TIMEOUT); flush();

                                    // return true, to reset the timer
                                    return true;
                                }
                                else
                                {
                                    // and close the connection
                                    _buf.shutdown();

                                    // connection closed
                                    setState(CONNECTION_CLOSED);

                                    // call the shutdown method of the derived class
                                    shutdown();

                                    // return false to stop the timer
                                    return false;
                                }
			}
		}
	}
}

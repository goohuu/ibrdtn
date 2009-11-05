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
		 : _buf(stream), std::iostream(&_buf), _shutdown(false), _in_timer(*this, 0), _out_timer(*this, timeout)
		{
			// startup the timer for KEEPALIVE messages.
			_in_timer.start();
			_out_timer.start();
		}

		StreamConnection::~StreamConnection()
		{
			_in_timer.stop();
			_out_timer.stop();
			join();
#ifdef DO_EXTENDED_DEBUG_OUTPUT
			cout << "remove StreamConnection" << endl;
#endif
		}

		bool StreamConnection::good()
		{
			return (iostream::good() && !_shutdown);
		}

		bool StreamConnection::timeout(dtn::utils::Timer *timer)
		{
			if (timer == &_out_timer)
			{
				(*this) << StreamDataSegment();
				(*this).flush();
				return true;
			}
			else if (timer == &_in_timer)
			{
#ifdef DO_EXTENDED_DEBUG_OUTPUT
				cout << "Connection timed out" << endl;
#endif
				shutdown();
				return false;
			}
		}

		void StreamConnection::shutdown()
		{
			if (!_shutdown)
			{
				_shutdown = true;
#ifdef DO_EXTENDED_DEBUG_OUTPUT
				cout << "shutdown: StreamConnection" << endl;
#endif

				(*this) << StreamDataSegment(StreamDataSegment::MSG_SHUTDOWN_NONE);
				(*this).flush();

				_buf.shutdown();
			}
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
					_in_timer.reset();

					switch (seg._type)
					{
						case StreamDataSegment::MSG_DATA_SEGMENT:
						{
							// New data segment received. Send a ACK.
							(*this) << StreamDataSegment(StreamDataSegment::MSG_ACK_SEGMENT, seg._value);
							break;
						}

						case StreamDataSegment::MSG_ACK_SEGMENT:
							//out_ack_size_ = seg._value;
							break;

						case StreamDataSegment::MSG_KEEPALIVE:
							break;

						case StreamDataSegment::MSG_REFUSE_BUNDLE:
							break;

						case StreamDataSegment::MSG_SHUTDOWN:
							// reply with a shutdown
							break;
					}

				} catch (dtn::exceptions::IOException ex) {
					// Connection terminated
#ifdef DO_EXTENDED_DEBUG_OUTPUT
					cout << "IOException: " << ex.what() << endl;
#endif
					shutdown();
					break;
				}

				// breakpoint to stop this thread
				yield();
			}
		}
	}
}

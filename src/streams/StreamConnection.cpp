/*
 * StreamConnection.cpp
 *
 *  Created on: 02.07.2009
 *      Author: morgenro
 */

#include "ibrdtn/streams/StreamConnection.h"
#include "ibrdtn/data/Exceptions.h"
#include "ibrdtn/streams/StreamContactHeader.h"
#include "ibrdtn/streams/StreamDataSegment.h"
#include "ibrdtn/data/Exceptions.h"

using namespace dtn::data;

namespace dtn
{
	namespace streams
	{
		StreamConnection::StreamConnection(StreamConnection::Callback &cb, iostream &stream)
		 : _buf(*this, stream), _callback(cb), std::iostream(&_buf), _ack_size(0),
		   _in_state(CONNECTION_INITIAL), _out_state(CONNECTION_INITIAL)
		{
		}

		StreamConnection::~StreamConnection()
		{
			// do a forced close
			close(true);
		}

		void StreamConnection::handshake(const dtn::data::EID &eid, const size_t timeout, const char flags)
		{
			try {
				// create a new header
				dtn::streams::StreamContactHeader header(eid);

				// set timeout
				header._keepalive = timeout;

				// set flags
				header._flags = flags;

				// do the handshake
				_peer = _buf.handshake(header);

				// set the stream state
				{
					ibrcommon::MutexLock out_lock(_out_state);
					_out_state.setState(CONNECTION_CONNECTED);

					ibrcommon::MutexLock in_lock(_in_state);
					_in_state.setState(CONNECTION_CONNECTED);
				}

				// signal the complete handshake
				_callback.eventConnectionUp(_peer);
			} catch (dtn::exceptions::InvalidDataException ex) {
				close();
			}
		}

		void StreamConnection::close(bool force)
		{
			if (force)
			{
				// and close the connection
				_buf.close();

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
			else
			{
				_buf.shutdown();

				// wait until the shutdown is received
				ibrcommon::MutexLock inl(_in_state);
				while (!_in_state.ifState(CONNECTION_CLOSED))
				{
					(*this).get();
				}

				_buf.close();

				// connection closed
				ibrcommon::MutexLock outl(_out_state);
				_out_state.setState(CONNECTION_CLOSED);
			}
		}

		void StreamConnection::eventShutdown()
		{
			_in_state.setState(CONNECTION_CLOSED);
			_callback.eventShutdown();
		}

		void StreamConnection::eventAck(size_t ack, size_t sent)
		{
			ibrcommon::MutexLock l(_in_state);
			_ack_size = ack;
			_sent_size = sent;
			_in_state.signal(true);
		}

		bool StreamConnection::isConnected()
		{
			if  (
					(_in_state.getState() > CONNECTION_INITIAL) &&
					(_in_state.getState() < CONNECTION_CLOSED)
				)
					return true;

			return false;
		}

		void StreamConnection::wait()
		{
			ibrcommon::MutexLock l(_in_state);
#ifdef DO_EXTENDED_DEBUG_OUTPUT
			if (_sent_size != _ack_size)
					cout << "wait for completion of transmission, current size: " << _ack_size << " of " << _sent_size << endl;
#endif
			while (_sent_size != _ack_size)
			{
				_in_state.wait();
#ifdef DO_EXTENDED_DEBUG_OUTPUT
				cout << "current size: " << _ack_size << " of " << _sent_size << endl;
#endif
			}
		}

		void StreamConnection::connectionTimeout()
		{

		}

		void StreamConnection::shutdownTimeout()
		{
			// connection closed
			{
				ibrcommon::MutexLock l(_in_state);
				_in_state.setState(CONNECTION_CLOSED);
			}

			{
				ibrcommon::MutexLock l(_out_state);
				_out_state.setState(CONNECTION_CLOSED);
			}

			// call superclass
			_callback.eventTimeout();
		}
	}
}

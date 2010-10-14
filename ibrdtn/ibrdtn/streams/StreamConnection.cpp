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
#include <ibrcommon/TimeMeasurement.h>
#include <ibrcommon/Logger.h>

using namespace dtn::data;

namespace dtn
{
	namespace streams
	{
		StreamConnection::StreamConnection(StreamConnection::Callback &cb, iostream &stream, const size_t buffer_size)
		 : std::iostream(&_buf), _callback(cb), _buf(*this, stream, buffer_size), _shutdown_reason(CONNECTION_SHUTDOWN_NOTSET)
		{
		}

		StreamConnection::~StreamConnection()
		{
			_buf.shutdowntimers();
		}

		void StreamConnection::handshake(const dtn::data::EID &eid, const size_t timeout, const char flags)
		{
			// create a new header
			dtn::streams::StreamContactHeader header(eid);

			// set timeout
			header._keepalive = timeout;

			// set flags
			header._flags = flags;

			// do the handshake
			_peer = _buf.handshake(header);

			// signal the complete handshake
			_callback.eventConnectionUp(_peer);
		}

		void StreamConnection::reject()
		{
			_buf.reject();
		}

		void StreamConnection::keepalive()
		{
			_buf.keepalive();
		}

		void StreamConnection::close()
		{
		}

		void StreamConnection::shutdown(ConnectionShutdownCases csc)
		{
			if (csc == CONNECTION_SHUTDOWN_SIMPLE_SHUTDOWN)
			{
				// wait for the last ACKs
				_buf.wait();
			}

			// skip if another shutdown is in progress
			{
				ibrcommon::MutexLock l(_shutdown_reason_lock);
				if (_shutdown_reason != CONNECTION_SHUTDOWN_NOTSET)
				{
					_buf.abort();
					return;
				}
				_shutdown_reason = csc;
			}
			
			try {
				switch (csc)
				{
					case CONNECTION_SHUTDOWN_IDLE:
						_buf.shutdown(StreamDataSegment::MSG_SHUTDOWN_IDLE_TIMEOUT);
						_buf.shutdowntimers();
						_buf.abort();
						_callback.eventTimeout();
						break;
					case CONNECTION_SHUTDOWN_ERROR:
						_buf.shutdowntimers();
						_buf.abort();
						_callback.eventError();
						break;
					case CONNECTION_SHUTDOWN_SIMPLE_SHUTDOWN:
						_buf.shutdown(StreamDataSegment::MSG_SHUTDOWN_NONE);
						_buf.shutdowntimers();
						_callback.eventShutdown();
						break;
					case CONNECTION_SHUTDOWN_NODE_TIMEOUT:
						_buf.abort();
						_callback.eventTimeout();
						break;
					case CONNECTION_SHUTDOWN_PEER_SHUTDOWN:
					case CONNECTION_SHUTDOWN_NOTSET:
						_buf.shutdowntimers();
						_buf.abort();
						_callback.eventShutdown();
						break;
				}
			} catch (StreamConnection::StreamErrorException ex) {
				_buf.shutdowntimers();
				_callback.eventError();
			}

			_buf.close();
			close();
			_callback.eventConnectionDown();
		}

		void StreamConnection::eventShutdown()
		{
			_callback.eventShutdown();
		}

		void StreamConnection::eventBundleAck(size_t ack)
		{
			_callback.eventBundleAck(ack);
		}

		void StreamConnection::eventBundleRefused()
		{
			IBRCOMMON_LOGGER_DEBUG(20) << "bundle has been refused" << IBRCOMMON_LOGGER_ENDL;
			_callback.eventBundleRefused();
		}

		void StreamConnection::eventBundleForwarded()
		{
			IBRCOMMON_LOGGER_DEBUG(20) << "bundle has been forwarded" << IBRCOMMON_LOGGER_ENDL;
			_callback.eventBundleForwarded();
		}

		void StreamConnection::connectionTimeout()
		{
			// call superclass
			_callback.eventTimeout();
		}
	}
}

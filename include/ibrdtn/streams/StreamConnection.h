/*
 * StreamConnection.h
 *
 *  Created on: 01.07.2009
 *      Author: morgenro
 */

#ifndef STREAMCONNECTION_H_
#define STREAMCONNECTION_H_

#include "ibrdtn/default.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/data/Exceptions.h"
#include "ibrdtn/streams/StreamContactHeader.h"
#include "ibrdtn/streams/StreamDataSegment.h"
#include "ibrdtn/streams/bpstreambuf.h"
#include "ibrcommon/thread/Timer.h"
#include "ibrcommon/thread/TimerCallback.h"
#include "ibrcommon/Exceptions.h"

namespace dtn
{
	namespace streams
	{
		class TransmissionInterruptedException : public ibrcommon::Exception
		{
			public:
				TransmissionInterruptedException(dtn::data::Bundle &bundle, size_t position) : ibrcommon::Exception("Transmission was interrupted.")
				{
				};
		};

		class StreamConnection : public std::iostream, public ibrcommon::JoinableThread, public ibrcommon::TimerCallback
		{
		public:
			enum ConnectionState
			{
				CONNECTION_IDLE = 0,
				CONNECTION_RECEIVING = 1,
				CONNECTION_SHUTDOWN = 2,
				CONNECTION_SHUTDOWN_REQUEST = 3,
				CONNECTION_CLOSED = 4
			};

			StreamConnection(iostream &stream);
			virtual ~StreamConnection();

			virtual void close();
			virtual void shutdown();

			bool timeout(ibrcommon::Timer *timer);

			bool isCompleted();
			bool waitCompleted();

			void waitClosed();

		protected:
			void setTimer(size_t in_timeout, size_t out_timeout);
			void shutdownTimer();

			void run();

			virtual void eventTimeout() {};
			virtual void eventShutdown() {};

			size_t _recv_size;
			size_t _ack_size;

		private:
			bpstreambuf _buf;

			ibrcommon::Timer *_in_timer;
			ibrcommon::Timer *_out_timer;

			ibrcommon::StatefulConditional<ConnectionState, CONNECTION_CLOSED> _in_state;
			ibrcommon::StatefulConditional<ConnectionState, CONNECTION_CLOSED> _out_state;
		};
	}
}

#endif /* STREAMCONNECTION_H_ */


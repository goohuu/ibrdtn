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

namespace dtn
{
	namespace streams
	{
		class TransmissionInterruptedException : public exceptions::Exception
		{
			public:
				TransmissionInterruptedException(dtn::data::Bundle &bundle, size_t position) : Exception("Transmission was interrupted.")
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
                            CONNECTION_CLOSED = 3
                        };

			StreamConnection(iostream &stream);
			virtual ~StreamConnection();

			virtual void shutdown();
			virtual bool good();

			bool timeout(ibrcommon::Timer *timer);

		protected:
                        void setTimer(size_t in_timeout, size_t out_timeout);
                        void setState(ConnectionState conn);
                        ConnectionState getState();
                        bool waitState(ConnectionState conn, size_t timeout);
                        bool waitState(ConnectionState conn);
			void run();

                        size_t _recv_size;
                        size_t _ack_size;

                        bool isCompleted();
                        bool waitCompleted();

		private:
			bpstreambuf _buf;

			ibrcommon::Timer *_in_timer;
			ibrcommon::Timer *_out_timer;

                        ibrcommon::Conditional _completed_cond;

                        ibrcommon::Conditional _state_cond;
                        ConnectionState _state;

		};
	}
}

#endif /* STREAMCONNECTION_H_ */


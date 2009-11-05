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
#include "ibrdtn/utils/Timer.h"
#include "ibrdtn/utils/TimerCallback.h"

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

		class StreamConnection : public std::iostream, public dtn::utils::JoinableThread, public dtn::utils::TimerCallback
		{
		public:
			StreamConnection(iostream &stream, size_t timeout);
			virtual ~StreamConnection();

			virtual void shutdown();
			virtual bool good();

			bool timeout(dtn::utils::Timer *timer);

		protected:
			void run();

		private:
			bpstreambuf _buf;
			bool _shutdown;

			dtn::utils::Timer _in_timer;
			dtn::utils::Timer _out_timer;
		};
	}
}

#endif /* STREAMCONNECTION_H_ */


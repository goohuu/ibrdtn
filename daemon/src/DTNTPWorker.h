/*
 * DTNTPWorker.h
 *
 *  Created on: 05.05.2011
 *      Author: morgenro
 */

#ifndef DTNTPWORKER_H_
#define DTNTPWORKER_H_

#include "core/AbstractWorker.h"
#include "core/EventReceiver.h"
#include "Configuration.h"

namespace dtn
{
	namespace daemon
	{
		class DTNTPWorker : public dtn::core::AbstractWorker, public dtn::core::EventReceiver
		{
		public:
			DTNTPWorker();
			virtual ~DTNTPWorker();

			void callbackBundleReceived(const Bundle &b);
			void raiseEvent(const dtn::core::Event *evt);

			class TimeSyncMessage
			{
			public:
				enum MSG_TYPE
				{
					TIMESYNC_REQUEST = 1,
					TIMESYNC_RESPONSE = 2
				};

				TimeSyncMessage();
				~TimeSyncMessage();

				MSG_TYPE type;

				timeval origin_timestamp;
				float origin_quality;

				timeval peer_timestamp;
				float peer_quality;

				friend std::ostream &operator<<(std::ostream &stream, const DTNTPWorker::TimeSyncMessage &obj);
				friend std::istream &operator>>(std::istream &stream, DTNTPWorker::TimeSyncMessage &obj);
			};

		private:
//			void shared_sync(const TimeSyncMessage &msg);
			void sync(const TimeSyncMessage &msg, struct timeval &tv);

			const dtn::daemon::Configuration::TimeSync &_conf;
			size_t _qot_current_tic;
			double _sigma;
			double _epsilon;

			TimeSyncMessage _last_sync;

			ibrcommon::Mutex _sync_lock;
			time_t _sync_age;
		};
	}
}

#endif /* DTNTPWORKER_H_ */

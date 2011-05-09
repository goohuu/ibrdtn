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
#include "ibrdtn/data/EID.h"

namespace dtn
{
	namespace daemon
	{
		class DTNTPWorker : public dtn::core::AbstractWorker, public dtn::core::EventReceiver
		{
		public:
			DTNTPWorker();
			virtual ~DTNTPWorker();

			size_t getTimestamp() const;
			void callbackBundleReceived(const Bundle &b);
			void raiseEvent(const dtn::core::Event *evt);

		private:
			enum MSG_TYPE
			{
				MEASUREMENT_REQUEST = 1,
				MEASUREMENT_RESPONSE = 2
			};

			class TimeBeacon : public ibrcommon::Mutex
			{
			public:
				TimeBeacon();
				virtual ~TimeBeacon();

				dtn::data::EID nodeid;
				time_t sec;
				long int usec;
				float quality;
			};

			void set_time(size_t sec, size_t usec);
			void get_time(TimeBeacon &beacon);

			void shared_sync(const TimeBeacon &beacon);
			void sync(const TimeBeacon &beacon);

			const dtn::daemon::Configuration::TimeSync &_conf;
			size_t _qot_current_tic;
			double _sigma;
			double _epsilon;

			TimeBeacon _last_sync;

			ibrcommon::Mutex _sync_lock;
		};
	}
}

#endif /* DTNTPWORKER_H_ */

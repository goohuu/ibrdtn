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
		};
	}
}

#endif /* DTNTPWORKER_H_ */

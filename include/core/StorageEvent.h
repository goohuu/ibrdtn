/*
 * ScheduleEvent.h
 *
 *  Created on: 06.03.2009
 *      Author: morgenro
 */

#ifndef STORAGEEVENT_H_
#define STORAGEEVENT_H_

#include "core/Event.h"
#include "core/BundleSchedule.h"
#include "data/Bundle.h"
#include <string>

using namespace std;

namespace dtn
{
	namespace core
	{
		class StorageEvent : public Event
		{
		public:
			StorageEvent(const BundleSchedule &schedule);
			StorageEvent(const Bundle *bundle);

			~StorageEvent();

			BundleSchedule getSchedule() const;
			const Bundle* getBundle() const;

			const string getName() const;

		#ifdef DO_DEBUG_OUTPUT
			string toString();
		#endif

			static const string className;

		private:
			const Bundle *m_bundle;
			const BundleSchedule &m_schedule;
		};
	}
}

#endif /* STORAGEEVENT_H_ */

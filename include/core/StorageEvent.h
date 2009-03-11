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
		enum EventStorageAction
		{
			STORE_BUNDLE = 0,
			STORE_SCHEDULE = 1
		};

		class StorageEvent : public Event
		{
		public:
			StorageEvent(const BundleSchedule &schedule);
			StorageEvent(const Bundle &bundle);

			~StorageEvent();

			EventStorageAction getAction() const;
			const BundleSchedule& getSchedule() const;
			const Bundle& getBundle() const;

			const string getName() const;
			const EventType getType() const;

		#ifdef DO_DEBUG_OUTPUT
			string toString();
		#endif

			static const string className;

		private:
			Bundle *m_bundle;
			const BundleSchedule m_schedule;
			EventStorageAction m_action;
		};
	}
}

#endif /* STORAGEEVENT_H_ */

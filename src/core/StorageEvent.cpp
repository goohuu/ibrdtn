/*
 * StorageEvent.cpp
 *
 *  Created on: 06.03.2009
 *      Author: morgenro
 */

#include "core/StorageEvent.h"

namespace dtn
{
	namespace core
	{
		StorageEvent::StorageEvent(const BundleSchedule &schedule)
		: m_schedule(schedule), m_bundle(NULL), m_action(STORE_SCHEDULE)
		{}

		StorageEvent::StorageEvent(Bundle *bundle)
		: m_bundle(bundle), m_schedule(BundleSchedule(NULL, 0, "dtn:none")), m_action(STORE_BUNDLE)
		{}

		StorageEvent::~StorageEvent()
		{}

		EventStorageAction StorageEvent::getAction() const
		{
			return m_action;
		}

		BundleSchedule StorageEvent::getSchedule() const
		{
			return m_schedule;
		}

		Bundle* StorageEvent::getBundle() const
		{
			return m_bundle;
		}

		const string StorageEvent::getName() const
		{
			return className;
		}

	#ifdef DO_DEBUG_OUTPUT
		string StorageEvent::toString()
		{
			return className;
		}
	#endif

		const string StorageEvent::className = "StorageEvent";
	}
}

/*
 * StorageEvent.cpp
 *
 *  Created on: 06.03.2009
 *      Author: morgenro
 */

#include "core/StorageEvent.h"
#include "data/Exceptions.h"

namespace dtn
{
	namespace core
	{
		StorageEvent::StorageEvent(const BundleSchedule &schedule)
		: m_bundle(NULL), m_schedule(schedule), m_action(STORE_SCHEDULE)
		{}

		StorageEvent::StorageEvent(const Bundle &bundle)
		: m_bundle(NULL), m_schedule(BundleSchedule(bundle, 0, "dtn:none")), m_action(STORE_BUNDLE)
		{
			m_bundle = new Bundle(bundle);
		}

		StorageEvent::~StorageEvent()
		{
			if (m_bundle != NULL) delete m_bundle;
		}

		const EventType StorageEvent::getType() const
		{
			return EVENT_ASYNC;
		}

		EventStorageAction StorageEvent::getAction() const
		{
			return m_action;
		}

		const BundleSchedule& StorageEvent::getSchedule() const
		{
			return m_schedule;
		}

		const Bundle& StorageEvent::getBundle() const
		{
			if (m_action != STORE_BUNDLE)
				throw exceptions::MissingObjectException();
			return *m_bundle;
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

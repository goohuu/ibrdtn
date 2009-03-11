/*
 * BundleEvent.cpp
 *
 *  Created on: 06.03.2009
 *      Author: morgenro
 */

#include "core/BundleEvent.h"

using namespace dtn::core;
using namespace std;

namespace dtn
{
	namespace core
	{
		BundleEvent::BundleEvent(const Bundle &b, const EventBundleAction action) : m_bundle(b), m_action(action)
		{}

		BundleEvent::~BundleEvent()
		{}

		const Bundle& BundleEvent::getBundle() const
		{
			return m_bundle;
		}

		const EventType BundleEvent::getType() const
		{
			return EVENT_ASYNC;
		}

		EventBundleAction BundleEvent::getAction() const
		{
			return m_action;
		}

		const string BundleEvent::getName() const
		{
			return BundleEvent::className;
		}

#ifdef DO_DEBUG_OUTPUT
		string BundleEvent::toString()
		{
			return className;
		}
#endif

		const string BundleEvent::className = "BundleEvent";
	}
}

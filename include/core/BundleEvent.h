/*
 * BundleEvent.h
 *
 *  Created on: 06.03.2009
 *      Author: morgenro
 */

#ifndef BUNDLEEVENT_H_
#define BUNDLEEVENT_H_

#include <string>
#include "data/Bundle.h"
#include "core/Event.h"
#include "data/StatusReportBlock.h"

using namespace dtn::data;
using namespace dtn::core;
using namespace std;

namespace dtn
{
	namespace core
	{
		enum EventBundleAction
		{
			BUNDLE_DELETED = 0,
			BUNDLE_CUSTODY_ACCEPTED = 1,
			BUNDLE_FORWARDED = 2,
			BUNDLE_DELIVERED = 3,
			BUNDLE_RECEIVED = 4
		};

		class BundleEvent : public Event
		{
		public:
			BundleEvent(const Bundle &bundle, EventBundleAction action, StatusReportReasonCode reason = NO_ADDITIONAL_INFORMATION);

			~BundleEvent();

			StatusReportReasonCode getReason() const;
			EventBundleAction getAction() const;
			const Bundle& getBundle() const;
			const string getName() const;
			const EventType getType() const;

#ifdef DO_DEBUG_OUTPUT
			string toString();
#endif

			static const string className;

		private:
			const Bundle m_bundle;
			EventBundleAction m_action;
			StatusReportReasonCode m_reason;
		};
	}
}

#endif /* BUNDLEEVENT_H_ */

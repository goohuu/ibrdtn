/*
 * BundleEvent.h
 *
 *  Created on: 06.03.2009
 *      Author: morgenro
 */

#ifndef BUNDLEEVENT_H_
#define BUNDLEEVENT_H_

#include "ibrdtn/default.h"
#include <string>
#include "ibrdtn/data/Bundle.h"
#include "core/Event.h"
#include "ibrdtn/data/StatusReportBlock.h"

using namespace dtn::data;
using namespace dtn::core;

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
			BundleEvent(const Bundle &bundle, EventBundleAction action, StatusReportBlock::REASON_CODE reason = StatusReportBlock::NO_ADDITIONAL_INFORMATION);

			virtual ~BundleEvent();

			StatusReportBlock::REASON_CODE getReason() const;
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
			StatusReportBlock::REASON_CODE m_reason;
		};
	}
}

#endif /* BUNDLEEVENT_H_ */

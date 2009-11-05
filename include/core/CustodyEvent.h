/*
 * CustodyEvent.h
 *
 *  Created on: 06.03.2009
 *      Author: morgenro
 */

#ifndef CUSTODYEVENT_H_
#define CUSTODYEVENT_H_

#include "ibrdtn/default.h"
#include <string>
#include "ibrdtn/data/Bundle.h"
#include "core/Event.h"
#include "ibrdtn/data/CustodySignalBlock.h"
#include "core/CustodyTimer.h"

using namespace dtn::data;
using namespace dtn::core;

namespace dtn
{
	namespace core
	{
		enum EventCustodyAction
		{
			CUSTODY_REJECT = 0,
			CUSTODY_ACCEPT = 1
		};

		class CustodyEvent : public Event
		{
		public:
			CustodyEvent(const Bundle &bundle, const EventCustodyAction action);
			virtual ~CustodyEvent();

			EventCustodyAction getAction() const;
			const Bundle& getBundle() const;
			const string getName() const;
			const EventType getType() const;

#ifdef DO_DEBUG_OUTPUT
			string toString();
#endif

			static const string className;

		private:
			const Bundle &m_bundle;
			const EventCustodyAction m_action;
		};
	}
}

#endif /* CUSTODYEVENT_H_ */

/*
 * CustodyEvent.h
 *
 *  Created on: 06.03.2009
 *      Author: morgenro
 */

#ifndef CUSTODYEVENT_H_
#define CUSTODYEVENT_H_

#include <string>
#include "data/Bundle.h"
#include "core/Event.h"
#include "data/CustodySignalBlock.h"
#include "core/CustodyTimer.h"

using namespace dtn::data;
using namespace dtn::core;
using namespace std;

namespace dtn
{
	namespace core
	{
		enum EventCustodyAction
		{
			CUSTODY_ACCEPTANCE = 0,
			CUSTODY_REJECT = 1,
			CUSTODY_TIMEOUT = 2,
			CUSTODY_REMOVE_TIMER = 3
		};

		class CustodyEvent : public Event
		{
		public:
			CustodyEvent(Bundle *bundle, const EventCustodyAction action);

			/**
			 * remove a custody timer
			 */
			CustodyEvent(string eid, const CustodySignalBlock *signal);

			/**
			 * announce a custody timeout
			 */
			CustodyEvent(CustodyTimer *timer);

			~CustodyEvent();

			EventCustodyAction getAction() const;
			const Bundle* getBundle() const;
			const CustodySignalBlock* getCustodySignal() const;
			string getEID() const;
			const CustodyTimer* getTimer() const;
			const string getName() const;

#ifdef DO_DEBUG_OUTPUT
			string toString();
#endif

			static const string className;

		private:
			string m_eid;
			const CustodySignalBlock *m_custody;
			Bundle *m_bundle;
			const EventCustodyAction m_action;
			CustodyTimer *m_timer;
		};
	}
}

#endif /* CUSTODYEVENT_H_ */

#ifndef CUSTODYMANAGER_H_
#define CUSTODYMANAGER_H_

#include "data/Bundle.h"
#include "data/CustodySignalBlock.h"
#include "core/CustodyTimer.h"
#include "core/EventReceiver.h"

#include "utils/Service.h"
#include "utils/Mutex.h"
#include "utils/Conditional.h"
#include <list>

using namespace dtn::utils;

namespace dtn
{
	namespace core
	{
		class CustodyManager : public Service, public EventReceiver
		{
			public:
				CustodyManager();

				virtual ~CustodyManager();

				void tick();

				virtual void setTimer(const Bundle &bundle, unsigned int time, unsigned int attempt);
				virtual const Bundle removeTimer(const CustodySignalBlock &block);

				virtual void acceptCustody(const Bundle &bundle);
				virtual void rejectCustody(const Bundle &bundle);

				void raiseEvent(const Event *evt);

			protected:
				void terminate();

			private:
				void retransmitBundle(const Bundle &bundle);
				void checkCustodyTimer();

				Mutex m_custodylock;
				unsigned int m_nextcustodytimer;
				list<CustodyTimer> m_custodytimer;
				Conditional m_breakwait;
		};
	}
}

#endif /*CUSTODYMANAGER_H_*/

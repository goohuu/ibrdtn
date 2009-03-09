#ifndef CUSTODYMANAGER_H_
#define CUSTODYMANAGER_H_

#include "data/Bundle.h"
#include "data/CustodySignalBlock.h"
#include "core/Node.h"
#include "core/CustodyTimer.h"
#include "core/EventReceiver.h"

#include "utils/Service.h"
#include "utils/Mutex.h"
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

				virtual bool timerAvailable();

				virtual void setTimer(Bundle *bundle, unsigned int time, unsigned int attempt);
				virtual Bundle* removeTimer(const CustodySignalBlock &block);

				void raiseEvent(const Event *evt);

			private:
				void checkCustodyTimer();

				Mutex m_custodylock;

				// Der nächste Zeitpunkt zu dem ein CustodyTimer abläuft
				// Frühestens zu diesem Zeitpunkt muss ein checkCustodyTimer() ausgeführt werden
				unsigned int m_nextcustodytimer;

				list<CustodyTimer> m_custodytimer;
		};
	}
}

#endif /*CUSTODYMANAGER_H_*/

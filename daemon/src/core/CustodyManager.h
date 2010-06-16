#ifndef CUSTODYMANAGER_H_
#define CUSTODYMANAGER_H_

#include "core/CustodyTimer.h"
#include "core/EventReceiver.h"
#include <ibrdtn/data/Bundle.h>
#include <ibrdtn/data/CustodySignalBlock.h>
#include <ibrcommon/thread/Thread.h>
#include <ibrcommon/thread/Conditional.h>

#include <list>


namespace dtn
{
	namespace core
	{
		class CustodyManager : public ibrcommon::JoinableThread, public EventReceiver
		{
			public:
				class NoTimerFoundException : public dtn::MissingObjectException
				{
				public:
					NoTimerFoundException(string what = "No matching timer was found.") throw() : MissingObjectException(what)
					{
					};
				};

				CustodyManager();

				virtual ~CustodyManager();

				virtual void setTimer(const Bundle &bundle, unsigned int time, unsigned int attempt);
				virtual const Bundle removeTimer(const CustodySignalBlock &block);

				virtual void acceptCustody(const Bundle &bundle);
				virtual void rejectCustody(const Bundle &bundle);

				void raiseEvent(const Event *evt);

			protected:
				void run();

			private:
				void retransmitBundle(const Bundle &bundle);
				void checkCustodyTimer();

				ibrcommon::Mutex m_custodylock;
				unsigned int m_nextcustodytimer;
				list<CustodyTimer> m_custodytimer;
				ibrcommon::Conditional _wait;

				bool _running;
		};
	}
}

#endif /*CUSTODYMANAGER_H_*/

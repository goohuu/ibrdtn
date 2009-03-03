#ifndef CUSTODYMANAGER_H_
#define CUSTODYMANAGER_H_

#include "data/Bundle.h"
#include "data/CustodySignalBlock.h"
#include "core/Node.h"
#include "core/CustodyTimer.h"
#include "core/CustodyManagerCallback.h"

namespace dtn
{
	namespace core
	{
		class CustodyManager
		{
			public:
				CustodyManager()
				{};
				
				virtual ~CustodyManager() {};
				
				virtual bool timerAvailable() = 0;
				
				virtual void setTimer(Node node, Bundle *bundle, unsigned int time, unsigned int attempt) = 0;
				virtual Bundle* removeTimer(string source, CustodySignalBlock *block) = 0;
				
				virtual void setCallbackClass(CustodyManagerCallback *callback) = 0;
		};
	}
}

#endif /*CUSTODYMANAGER_H_*/

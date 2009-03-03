#ifndef CUSTODYMANAGERCALLBACK_H_
#define CUSTODYMANAGERCALLBACK_H_

#include "core/CustodyTimer.h"

namespace dtn
{
	namespace core
	{
		class CustodyManagerCallback
		{
			public:
				CustodyManagerCallback() {};
				virtual ~CustodyManagerCallback() {};
				
				virtual void triggerCustodyTimeout(CustodyTimer timer) = 0;
		};
	}
}

#endif /*CUSTODYMANAGERCALLBACK_H_*/

#ifndef DEBUGGER_H_
#define DEBUGGER_H_

#include "core/AbstractWorker.h"

using namespace dtn::data;
using namespace dtn::core;

/**
 * Dieser Service sendet regelmäßig einen Report an einen bestimmten
 * Knoten.
 */

namespace dtn
{
	namespace daemon
	{
		class Debugger : AbstractWorker
		{
			public:
				Debugger() : AbstractWorker("/debugger") {};
				~Debugger() {};

				TransmitReport callbackBundleReceived(const Bundle &b);
		};
	}
}

#endif /*DEBUGGER_H_*/

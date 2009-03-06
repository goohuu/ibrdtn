#include "daemon/Debugger.h"
#include "utils/Utils.h"
#include <iostream>

using namespace dtn::data;
using namespace dtn::core;
using namespace std;

namespace dtn
{
	namespace daemon
	{
		TransmitReport Debugger::callbackBundleReceived(const Bundle &b)
		{
			cout << "Bundle received " << b.toString() << endl;

			return BUNDLE_ACCEPTED;
		}
	}
}

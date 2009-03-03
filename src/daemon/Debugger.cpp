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
		TransmitReport Debugger::callbackBundleReceived(Bundle *b)
		{
			cout << "Bundle received!" << endl;

			delete b;
			return BUNDLE_ACCEPTED;
		}
	}
}

#include "daemon/Debugger.h"
#include "ibrdtn/utils/Utils.h"
#include <iostream>

using namespace dtn::data;
using namespace dtn::core;
using namespace std;

namespace dtn
{
	namespace daemon
	{
		void Debugger::callbackBundleReceived(const Bundle &b)
		{
			cout << "Bundle received " << b.toString() << endl;
		}
	}
}

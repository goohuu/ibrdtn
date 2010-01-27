/*
 * DevNull.cpp
 *
 *  Created on: 27.01.2010
 *      Author: morgenro
 */

#include "daemon/DevNull.h"

using namespace dtn::data;
using namespace dtn::core;
using namespace std;

namespace dtn
{
	namespace daemon
	{
		dtn::net::TransmitReport DevNull::callbackBundleReceived(const Bundle &b)
		{
			return dtn::net::BUNDLE_ACCEPTED;
		}
	}
}

/*
 * DevNull.h
 *
 *  Created on: 27.01.2010
 *      Author: morgenro
 */

#ifndef DEVNULL_H_
#define DEVNULL_H_

#include "ibrdtn/default.h"
#include "core/AbstractWorker.h"

using namespace dtn::data;
using namespace dtn::core;

namespace dtn
{
	namespace daemon
	{
		class DevNull : AbstractWorker
		{
		public:
			DevNull()
			{
				AbstractWorker::initialize("/null", true);
			};
			virtual ~DevNull() {};

			dtn::net::TransmitReport callbackBundleReceived(const Bundle &b);
		};
	}
}

#endif /* DEVNULL_H_ */

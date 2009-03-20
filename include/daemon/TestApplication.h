/*
 * TestApplication.h
 *
 *  Created on: 06.03.2009
 *      Author: morgenro
 */

#ifndef TESTAPPLICATION_H_
#define TESTAPPLICATION_H_

#include "core/AbstractWorker.h"
#include "utils/Service.h"

using namespace dtn::utils;
using namespace dtn::core;

namespace dtn
{
	namespace daemon
	{
		class TestApplication : public AbstractWorker, public Service
		{
			public:
				TestApplication(string destination)
					: AbstractWorker("/test"), Service("TestApplication"), m_dtntime(0), m_destination(destination) {}
				~TestApplication() {};
				void tick();

				TransmitReport callbackBundleReceived(const Bundle &b);

			private:
				void reportIt();
				unsigned int m_dtntime;
				string m_destination;
		};
	}
}

#endif /* TESTAPPLICATION_H_ */

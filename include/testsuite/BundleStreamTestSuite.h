/*
 * BundleStreamTestSuite.h
 *
 *  Created on: 25.06.2009
 *      Author: morgenro
 */

#include "ibrdtn/default.h"

#ifndef BUNDLESTREAMTESTSUITE_H_
#define BUNDLESTREAMTESTSUITE_H_

#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/data/PayloadBlock.h"
#include "ibrcommon/data/BLOBManager.h"

using namespace dtn::data;

namespace dtn
{
	namespace testsuite
	{
		class BundleStreamTestSuite
		{
		public:
			BundleStreamTestSuite();
			virtual ~BundleStreamTestSuite();

			bool runAllTests();
			bool commonTest();

		private:
			ibrcommon::BLOBManager &_blobmanager;

			PayloadBlock* createTestBlock();
		};
	}
}

#endif /* BUNDLESTREAMTESTSUITE_H_ */

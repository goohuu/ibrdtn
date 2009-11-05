/*
 * BLOBTestSuite.h
 *
 *  Created on: 19.06.2009
 *      Author: morgenro
 */

#include "ibrdtn/default.h"

#ifndef BLOBTESTSUITE_H_
#define BLOBTESTSUITE_H_

#include <iostream>

namespace dtn
{
	namespace testsuite
	{
		class BLOBTestSuite
		{
		public:
			BLOBTestSuite();
			virtual ~BLOBTestSuite();
			bool runAllTests();

		private:
			bool appendTest();
			bool writeTest();
			bool splitTest();
			bool mergeTest();
		};
	}
}

#endif /* BLOBTESTSUITE_H_ */

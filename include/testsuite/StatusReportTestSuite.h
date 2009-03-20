/*
 * StatusReportTestSuite.h
 *
 *  Created on: 20.03.2009
 *      Author: morgenro
 */

#include "config.h"

#ifndef STATUSREPORTTESTSUITE_H_
#define STATUSREPORTTESTSUITE_H_


namespace dtn
{
namespace testsuite
{
	class StatusReportTestSuite
	{
		public:
			StatusReportTestSuite();

			~StatusReportTestSuite();

			bool runAllTests();

		private:
			bool createTest();
			bool parseTest();
			bool fragmentTest();
			bool matchTest();
			bool sdnvTest();
	};
}
}

#endif /* STATUSREPORTTESTSUITE_H_ */

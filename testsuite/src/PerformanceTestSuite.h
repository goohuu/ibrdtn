#ifndef PERFORMANCETESTSUITE_H_
#define PERFORMANCETESTSUITE_H_

#include "ibrdtn/default.h"

namespace dtn
{
namespace testsuite
{
	class PerformanceTestSuite
	{
		public:
			PerformanceTestSuite();

			virtual ~PerformanceTestSuite();

			bool runAllTests();

		private:
			bool createBundleTest();

#ifdef HAVE_LIBSQLITE3
			bool SQLiteSpeedTest();
#endif

	};
}
}

#endif /*PERFORMANCETESTSUITE_H_*/

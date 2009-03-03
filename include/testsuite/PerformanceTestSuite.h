#ifndef PERFORMANCETESTSUITE_H_
#define PERFORMANCETESTSUITE_H_

namespace dtn
{
namespace testsuite
{
	class PerformanceTestSuite
	{
		public:
			PerformanceTestSuite();

			~PerformanceTestSuite();

			bool runAllTests();

		private:
			bool createBundleTest();
			bool SQLiteSpeedTest();
	};
}
}

#endif /*PERFORMANCETESTSUITE_H_*/

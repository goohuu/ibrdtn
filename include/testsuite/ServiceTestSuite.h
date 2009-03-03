#ifndef SERVICETESTSUITE_H_
#define SERVICETESTSUITE_H_

namespace dtn
{
namespace testsuite
{
	class ServiceTestSuite
	{
		public:
			ServiceTestSuite();
	
			~ServiceTestSuite();
			
			bool runAllTests();
			
		private:
			bool testTwoThreads();
	};
}
}

#endif /*SERVICETESTSUITE_H_*/

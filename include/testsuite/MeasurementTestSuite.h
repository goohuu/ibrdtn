#ifndef MEASUREMENTTESTSUITE_H_
#define MEASUREMENTTESTSUITE_H_

namespace dtn
{
	namespace testsuite
	{
		class MeasurementTestSuite
		{
			public:
				MeasurementTestSuite();
		
				~MeasurementTestSuite();
				
				bool runAllTests();
				
			private:
				bool createData();
		};
	}
}

#endif /*MEASUREMENTTESTSUITE_H_*/

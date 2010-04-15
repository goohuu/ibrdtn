#ifndef MEASUREMENTTESTSUITE_H_
#define MEASUREMENTTESTSUITE_H_

#include "ibrdtn/default.h"

#ifdef USE_EMMA_CODE

namespace dtn
{
	namespace testsuite
	{
		class MeasurementTestSuite
		{
			public:
				MeasurementTestSuite();

				virtual ~MeasurementTestSuite();

				bool runAllTests();

			private:
				bool createData();
		};
	}
}

#endif

#endif /*MEASUREMENTTESTSUITE_H_*/

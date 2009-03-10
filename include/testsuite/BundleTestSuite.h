#ifndef BUNDLETESTSUITE_H_
#define BUNDLETESTSUITE_H_

#include "data/Bundle.h"

using namespace dtn::data;

namespace dtn
{
namespace testsuite
{
	class BundleTestSuite
	{
		public:
			BundleTestSuite();

			~BundleTestSuite();

			bool runAllTests();

		private:
			bool createTest();
			bool copyTest();
			bool instanceTest();
			bool compareTest();
			bool splitTest();
			bool cutTest();
			bool sliceTest();
			bool mergeTest();
			bool mergeBlockTest();
	};
}
}

#endif /*BUNDLETESTSUITE_H_*/

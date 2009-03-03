#include "data/PayloadBlockFactory.h"

#ifndef SELFTESTSUITE_H_
#define SELFTESTSUITE_H_

namespace dtn
{
namespace testsuite
{
	class SelfTestSuite
	{
		public:
			SelfTestSuite();

			~SelfTestSuite();

			bool runAllTests();
	};
}
}

#endif /*SELFTESTSUITE_H_*/

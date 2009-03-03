#ifndef SELFTESTSUITE_H_
#define SELFTESTSUITE_H_

#include "data/PayloadBlockFactory.h"
#include "config.h"

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

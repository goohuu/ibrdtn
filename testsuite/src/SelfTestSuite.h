#ifndef SELFTESTSUITE_H_
#define SELFTESTSUITE_H_

#include "ibrdtn/default.h"

namespace dtn
{
namespace testsuite
{
	class SelfTestSuite
	{
		public:
			SelfTestSuite();

			virtual ~SelfTestSuite();

			bool runAllTests();
	};
}
}

#endif /*SELFTESTSUITE_H_*/

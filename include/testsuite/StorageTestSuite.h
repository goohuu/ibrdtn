#ifndef STORAGETESTSUITE_H_
#define STORAGETESTSUITE_H_

#include "core/AbstractBundleStorage.h"
#include "data/Bundle.h"

using namespace dtn::data;
using namespace dtn::core;

namespace dtn
{
namespace testsuite
{
	class StorageTestSuite
	{
		public:
			StorageTestSuite();

			~StorageTestSuite();

			bool runAllTests();

		private:
			bool testSimpleBundleStorage();
			bool fragmentationTest(AbstractBundleStorage &storage);
	};
}
}

#endif /*STORAGETESTSUITE_H_*/

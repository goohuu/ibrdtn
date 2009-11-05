#ifndef STORAGETESTSUITE_H_
#define STORAGETESTSUITE_H_

#include "core/BundleStorage.h"
#include "ibrdtn/data/Bundle.h"

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

			virtual ~StorageTestSuite();

			bool runAllTests();

		private:
			bool testBundleStorage(BundleStorage &storage);
			bool StoreAndRetrieveTest(BundleStorage &storage);
			bool fragmentationTest(BundleStorage &storage);
	};
}
}

#endif /*STORAGETESTSUITE_H_*/

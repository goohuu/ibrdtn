/*
 * SQLiteTestSuite.h
 *
 *  Created on: 13.10.2008
 *      Author: morgenro
 */

#ifndef SQLITETESTSUITE_H_
#define SQLITETESTSUITE_H_

#include "core/SQLiteBundleStorage.h"
#include "data/Bundle.h"

using namespace dtn::data;
using namespace dtn::core;

namespace dtn
{
namespace testsuite
{
	class SQLiteTestSuite
	{
		public:
			SQLiteTestSuite();

			~SQLiteTestSuite();

			bool runAllTests();

		private:
			bool testSimpleBundleStorage();
			bool StoreAndRetrieveTest(SQLiteBundleStorage &storage);
			bool fragmentationTest(SQLiteBundleStorage &storage);
	};
}
}

#endif /* SQLITETESTSUITE_H_ */

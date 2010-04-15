/*
 * BlockTestSuite.h
 *
 *  Created on: 10.03.2009
 *      Author: morgenro
 */

#include "ibrdtn/default.h"

#ifndef BLOCKTESTSUITE_H_
#define BLOCKTESTSUITE_H_


#include "ibrdtn/data/Block.h"

using namespace dtn::data;

namespace dtn
{
namespace testsuite
{
	class BlockTestSuite
	{
		public:
			BlockTestSuite();

			virtual ~BlockTestSuite();

			bool runAllTests();

		private:
			bool createTest();
	};
}
}

#endif /* BLOCKTESTSUITE_H_ */

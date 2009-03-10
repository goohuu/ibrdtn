/*
 * BlockTestSuite.h
 *
 *  Created on: 10.03.2009
 *      Author: morgenro
 */

#include "config.h"

#ifndef BLOCKTESTSUITE_H_
#define BLOCKTESTSUITE_H_


#include "data/Block.h"

using namespace dtn::data;

namespace dtn
{
namespace testsuite
{
	class BlockTestSuite
	{
		public:
			BlockTestSuite();

			~BlockTestSuite();

			bool runAllTests();

		private:
			bool createTest();
			bool copyTest();
	};
}
}

#endif /* BLOCKTESTSUITE_H_ */

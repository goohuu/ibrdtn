/*
 * BlockTestSuite.cpp
 *
 *  Created on: 10.03.2009
 *      Author: morgenro
 */

#include "testsuite/BlockTestSuite.h"

#include "ibrdtn/data/PayloadBlock.h"
#include <iostream>
#include <typeinfo>

using namespace dtn::data;
using namespace std;

namespace dtn
{
	namespace testsuite
	{
		BlockTestSuite::BlockTestSuite()
		{
		}

		BlockTestSuite::~BlockTestSuite()
		{
		}

		bool BlockTestSuite::runAllTests()
		{
			bool ret = true;
			cout << "BlockTestSuite... ";

			if ( !createTest() )
			{
				cout << endl << "createTest failed" << endl;
				ret = false;
			}

			if (ret) cout << "\t\t\tpassed" << endl;

			return ret;
		}

		bool BlockTestSuite::createTest()
		{
			bool ret = true;

			Block *block = (Block*)new PayloadBlock();

			if (dynamic_cast<PayloadBlock*>(block) == NULL)
			{
				ret = false;
			}

			delete block;

			return ret;
		}
	}
}

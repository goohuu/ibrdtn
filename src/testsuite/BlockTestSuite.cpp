/*
 * BlockTestSuite.cpp
 *
 *  Created on: 10.03.2009
 *      Author: morgenro
 */

#include "testsuite/BlockTestSuite.h"
#include "data/BundleFactory.h"
#include "data/PayloadBlock.h"
#include "data/PayloadBlockFactory.h"
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

			if ( !copyTest() )
			{
				cout << endl << "copyTest failed" << endl;
				ret = false;
			}

			if (ret) cout << "\t\t\tpassed" << endl;

			return ret;
		}

		bool BlockTestSuite::createTest()
		{
			BundleFactory &fac = BundleFactory::getInstance();
			bool ret = true;

			string data = "Hallo Welt";
			Block *block = PayloadBlockFactory::newPayloadBlock((unsigned char*)data.c_str(), data.length());

			if (dynamic_cast<PayloadBlock*>(block) == NULL)
			{
				ret = false;
			}

			delete block;

			return ret;
		}

		bool BlockTestSuite::copyTest()
		{
			BundleFactory &fac = BundleFactory::getInstance();
			bool ret = true;

			string data = "Hallo Welt";
			Block *block = PayloadBlockFactory::newPayloadBlock((unsigned char*)data.c_str(), data.length());

			// make a copy
			Block *copy = fac.copyBlock(*block);

			try {
				PayloadBlock *pblock = dynamic_cast<PayloadBlock *>(copy);

				if (pblock == NULL)
				{
					ret = false;
				}
			} catch (std::bad_cast) {
				ret = false;
			}

			delete copy;
			delete block;

			return ret;
		}
	}
}

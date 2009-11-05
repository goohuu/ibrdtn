#include "testsuite/BundleTestSuite.h"
#include "testsuite/SelfTestSuite.h"
#include "ibrdtn/data/Bundle.h"

#include "testsuite/TestUtils.h"
#include "ibrdtn/data/CustodySignalBlock.h"
#include <iostream>
#include <list>
#include "ibrdtn/utils/Utils.h"
#include <limits.h>
#include <cstdlib>

using namespace dtn::data;

namespace dtn
{
	namespace testsuite
	{
		BundleTestSuite::BundleTestSuite()
		{
		}

		BundleTestSuite::~BundleTestSuite()
		{
		}

		bool BundleTestSuite::runAllTests()
		{
			bool ret = true;
			cout << "BundleTestSuite... ";

			if ( !mergeTest() )
			{
				cout << endl << "mergeTest failed" << endl;
				ret = false;
			}

			if (ret) cout << "\t\t\tpassed" << endl;

			return ret;
		}

		bool BundleTestSuite::mergeTest()
		{
			bool ret = true;

			// Erstelle ein "großes" Bundle
			Bundle bundle = TestUtils::createTestBundle(1024);
			PayloadBlock *block = utils::Utils::getPayloadBlock( bundle );

			unsigned int offset = 0;
			pair<PayloadBlock*, PayloadBlock*> sblocks = utils::Utils::split(block, 512);

			// check for correct merge
			if (sblocks.first->getBLOBReference().getSize() + sblocks.second->getBLOBReference().getSize() != 1024)
			{
				cout << "split failed" << endl;
				return false;
			}

			Bundle first = bundle;
			first.clearBlocks();
			first._procflags += Bundle::FRAGMENT;
			first._appdatalength = block->getBLOBReference().getSize();
			first.addBlock(sblocks.first);

			Bundle second = bundle;
			second.clearBlocks();
			second._procflags += Bundle::FRAGMENT;
			second._fragmentoffset = 512;
			second._appdatalength = first._appdatalength;
			second.addBlock(sblocks.second);

			// Merge
			Bundle merged = utils::Utils::merge( first, second );

			// check payload size
			if ( utils::Utils::getPayloadBlock(merged)->getBLOBReference().getSize() != 1024 )
			{
				cout << "payload size isn't correct" << endl;
				return false;
			}

			if ( merged._procflags & Bundle::FRAGMENT )
			{
				ret = false;
				cout << "bundle is still a fragment!" << endl;
			}

			if (!TestUtils::compareBundles(bundle, merged))
			{
				ret = false;
				cout << "bundles doesn't match" << endl;
			}

			return ret;
		}
	}
}

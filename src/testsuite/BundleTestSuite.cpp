#include "testsuite/BundleTestSuite.h"
#include "testsuite/SelfTestSuite.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/data/PayloadBlock.h"
#include "ibrcommon/data/BLOB.h"
#include "ibrcommon/data/File.h"

#include "testsuite/TestUtils.h"
#include "ibrdtn/data/CustodySignalBlock.h"
#include <iostream>
#include <list>
#include "ibrdtn/utils/Utils.h"
#include <limits.h>
#include <cstdlib>
#include <fstream>

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

//			if ( !mergeTest() )
//			{
//				cout << endl << "mergeTest failed" << endl;
//				ret = false;
//			}

//			if ( !serializeTest() )
//			{
//				cout << endl << "serializeTest failed" << endl;
//				ret = false;
//			}

			if (ret) cout << "\t\t\tpassed" << endl;

			return ret;
		}

		bool BundleTestSuite::serializeTest()
		{
			bool ret = true;

			// write a bundle into a file
			{
				dtn::data::Bundle b;

				ibrcommon::BLOB::Reference fb = ibrcommon::FileBLOB::create(ibrcommon::File("testfile"));
				dtn::data::PayloadBlock *p = new dtn::data::PayloadBlock(fb);

				b.addBlock(p);

				std::fstream file("outfile1.test", ios::in|ios::out|ios::binary|ios::trunc);

				file << b;
				file.close();
			}

			// read the bundle
			{
				dtn::data::Bundle b;

				std::fstream file("outfile1.test", ios::in|ios::binary);
				file >> b;
				file.close();

				std::list<dtn::data::PayloadBlock> blist = b.getBlocks<dtn::data::PayloadBlock>();

				std::fstream file2("outfile2.test", ios::in|ios::out|ios::binary|ios::trunc);

				blist.front().getBLOB().read(file2);
				file2.close();
			}

			return ret;
		}

//		bool BundleTestSuite::mergeTest()
//		{
//			bool ret = true;
//
//			// Erstelle ein "großes" Bundle
//			Bundle bundle = TestUtils::createTestBundle(1024);
//			PayloadBlock *block = utils::Utils::getPayloadBlock( bundle );
//
//			unsigned int offset = 0;
//			pair<PayloadBlock*, PayloadBlock*> sblocks = block->split(512);
//
//			// check for correct merge
//			if (sblocks.first->getBLOB().getSize() + sblocks.second->getBLOB().getSize() != 1024)
//			{
//				cout << "split failed" << endl;
//				return false;
//			}
//
//			Bundle first = bundle;
//			first.clearBlocks();
//			first._procflags += Bundle::FRAGMENT;
//			first._appdatalength = block->getBLOB().getSize();
//			first.addBlock(sblocks.first);
//
//			Bundle second = bundle;
//			second.clearBlocks();
//			second._procflags += Bundle::FRAGMENT;
//			second._fragmentoffset = 512;
//			second._appdatalength = first._appdatalength;
//			second.addBlock(sblocks.second);
//
//			// Merge
//			Bundle merged = utils::Utils::merge( first, second );
//
//			// check payload size
//			if ( utils::Utils::getPayloadBlock(merged)->getBLOB().getSize() != 1024 )
//			{
//				cout << "payload size isn't correct" << endl;
//				return false;
//			}
//
//			if ( merged._procflags & Bundle::FRAGMENT )
//			{
//				ret = false;
//				cout << "bundle is still a fragment!" << endl;
//			}
//
//			if (!TestUtils::compareBundles(bundle, merged))
//			{
//				ret = false;
//				cout << "bundles doesn't match" << endl;
//			}
//
//			return ret;
//		}
	}
}

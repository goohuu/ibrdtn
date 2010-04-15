#include "testsuite/StorageTestSuite.h"
#include "core/SimpleBundleStorage.h"
#include "ibrdtn/data/Exceptions.h"
#include "testsuite/TestUtils.h"
#include <iostream>
#include "ibrdtn/utils/Utils.h"

using namespace dtn::data;
using namespace dtn::core;

namespace dtn
{
namespace testsuite
{
	StorageTestSuite::StorageTestSuite()
	{
	}

	StorageTestSuite::~StorageTestSuite()
	{
	}

	bool StorageTestSuite::runAllTests()
	{
		bool ret = true;
		cout << "StorageTestSuite... ";

		// create a SimpleBundleStorage
		SimpleBundleStorage storage;

		if ( !testBundleStorage( storage ) )
		{
			cout << endl << "SimpleBundleStorage failed" << endl;
			ret = false;
		}

//#ifdef HAVE_LIBSQLITE3
//		SQLiteBundleStorage sqlite("database.db", true);
//
//		if ( !testBundleStorage( sqlite ) )
//		{
//			cout << endl << "SQLiteBundleStorage failed" << endl;
//			ret = false;
//		}
//#endif

		if (ret) cout << "\t\t\tpassed" << endl;

		return ret;
	}

	bool StorageTestSuite::testBundleStorage(BundleStorage &storage)
	{
		bool ret = true;

		if (!StoreAndRetrieveTest(storage))
		{
			cout << "store'n'retrieve test failed" << endl;
			ret = false;
		}

//		if (!fragmentationTest(storage))
//		{
//			cout << "fragmentation test failed" << endl;
//			ret = false;
//		}

		return ret;
	}

	bool StorageTestSuite::StoreAndRetrieveTest(BundleStorage &storage)
	{
		bool ret = true;

		Bundle b1;

		for (int i = 0; i < 10; i++)
		{
			storage.store(b1);
		}

		try {
			Bundle b2 = storage.get( BundleID(b1) );

		} catch (dtn::exceptions::NoBundleFoundException ex) {
			ret = false;
		}

		return ret;
	}

//	bool StorageTestSuite::fragmentationTest(BundleStorage &storage)
//	{
//		// create a bundle
//		Bundle origin = TestUtils::createTestBundle(1024);
//		PayloadBlock *origin_payload = utils::Utils::getPayloadBlock(origin);
//		size_t applength = origin_payload->getBLOBReference().getSize();
//
//		Bundle merged;
//		list<Bundle> fragments;
//		pair<PayloadBlock*, PayloadBlock*> blocks;
//
//		PayloadBlock *tail = origin_payload;
//
//		// create fragments of the bundle
//		for (int i = 128; i <= 1024; i += 128)
//		{
//			pair<PayloadBlock*, PayloadBlock*> blocks = utils::Utils::split(tail, 128);
//
//			// do not delete the first tail object, it is origin_payload!
//			if (i != 128) delete tail;
//
//			Bundle fbundle = origin;
//
//			fbundle._procflags += Bundle::FRAGMENT;
//			fbundle._fragmentoffset = i - 128;
//			fbundle._appdatalength = applength;
//
//			fbundle.clearBlocks();
//			fbundle.addBlock(blocks.first);
//
//			// set the second part as tail
//			tail = blocks.second;
//
//			try {
//				Bundle m; storage.store( fbundle );
//
//				// delete the tail
//				if (i != 128) delete tail;
//
//				// check m, it has to be equal to origin
//				if (TestUtils::compareBundles(m, origin)) return true;
//			} catch (exceptions::MissingObjectException ex) {
//
//			}
//		}
//
//		return false;
//	}
}
}

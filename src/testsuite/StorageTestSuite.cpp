#include "testsuite/StorageTestSuite.h"
#include "core/SimpleBundleStorage.h"
#include "data/BundleFactory.h"
#include "testsuite/TestUtils.h"
#include <iostream>
#include "utils/Utils.h"

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

		if ( !testSimpleBundleStorage() )
		{
			cout << endl << "testSimpleBundleStorage failed" << endl;
			ret = false;
		}

		if (ret) cout << "\t\t\tpassed" << endl;

		return ret;
	}

	bool StorageTestSuite::testSimpleBundleStorage()
	{
		bool ret = true;

		BundleCore core("dtn://node1");

		// Erstelle eine SimpleBundleStorage
		SimpleBundleStorage storage(core);

		if (!fragmentationTest(storage))
		{
			cout << "Fragmentation Test for SimpleBundleStorage failed" << endl;
			ret = false;
		}

		return ret;
	}

	bool StorageTestSuite::fragmentationTest(BundleStorage &storage)
	{
		bool ret = true;

		// Erstelle ein Bündel
		Bundle *origin = TestUtils::createTestBundle(18196);
		Bundle *merged = NULL;

		// Erzeuge dazu Fragmente
		list<Bundle*> fragments = BundleFactory::split(*origin, 1200);

		unsigned int size = fragments.size();

		for (unsigned int i = 0; i < size; i++)
		{
			if ( (i % 2) == 0 )
			{
				merged = storage.storeFragment( fragments.front() );
				fragments.pop_front();
			}
			else
			{
				merged = storage.storeFragment( fragments.back() );
				fragments.pop_back();
			}
		}

		// Bei der Übergabe des letzten Fragments muss ein komplettes Bündel zurückgegeben werden
		if (merged == NULL)
		{
			cout << "NULL returned" << endl;
			ret = false;
		}
		else
		{
			if (!TestUtils::compareBundles(origin, merged))
			{
				ret = false;
			}
			delete merged;
		}

		delete origin;

		return ret;
	}
}
}

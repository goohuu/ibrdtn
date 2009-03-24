#include "testsuite/SQLiteTestSuite.h"
#include "core/SQLiteBundleStorage.h"
#include "data/BundleFactory.h"
#include "testsuite/TestUtils.h"
#include "utils/Utils.h"
#include <iostream>

using namespace dtn::data;

#ifdef HAVE_LIBSQLITE3

namespace dtn
{
	namespace testsuite
	{
		SQLiteTestSuite::SQLiteTestSuite()
		{
		}

		SQLiteTestSuite::~SQLiteTestSuite()
		{
		}

		bool SQLiteTestSuite::runAllTests()
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

		bool SQLiteTestSuite::testSimpleBundleStorage()
		{
			bool ret = true;

			// Erstelle eine SimpleBundleStorage
			SQLiteBundleStorage storage("database.db", true);

			if (!StoreAndRetrieveTest(storage))
			{
				cout << "store'n'retrieve Test for SQLiteBundleStorage failed" << endl;
				ret = false;
			}

			if (!fragmentationTest(storage))
			{
				cout << "Fragmentation Test for SQLiteBundleStorage failed" << endl;
				ret = false;
			}

			return ret;
		}

		bool SQLiteTestSuite::StoreAndRetrieveTest(SQLiteBundleStorage &storage)
		{
			bool ret = true;

			Bundle *b;

			for (int i = 0; i < 10; i++)
			{
				b = TestUtils::createTestBundle(1024);
				b->setSource("dtn://test1");
				BundleSchedule s(*b, 4, "dtn://test2");
				storage.store(s);
				delete b;
			}

			try {
				BundleSchedule s = storage.getSchedule(2);
				ret = false;
			} catch (dtn::exceptions::NoScheduleFoundException ex) {

			}

			try {
				BundleSchedule s = storage.getSchedule(4);
			} catch (dtn::exceptions::NoScheduleFoundException ex) {
				ret = false;
			}

			return ret;
		}

		bool SQLiteTestSuite::fragmentationTest(SQLiteBundleStorage &storage)
		{
			bool ret = true;

			// Erstelle ein Bündel
			Bundle *origin = TestUtils::createTestBundle(1024);
			Bundle *merged = NULL;

			// Erzeuge dazu Fragmente
			list<Bundle> fragments = BundleFactory::split(*origin, 128);
			delete origin;

			// Füttere die Storage mit den Fragmenten
			list<Bundle>::const_iterator iter = fragments.begin();

			while (iter != fragments.end())
			{
				merged = storage.storeFragment(*iter);
				iter++;
			}

			// Bei der Übergabe des letzten Fragments muss ein komplettes Bündel zurückgegeben werden
			if (merged == NULL)
			{
				ret = false;
			}
			else
			{
				delete merged;
			}

			return ret;
		}
	}
}

#endif

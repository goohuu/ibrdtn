#include "testsuite/PerformanceTestSuite.h"
#include "data/BundleFactory.h"
#include "data/PayloadBlockFactory.h"
#include "core/SQLiteBundleStorage.h"
#include <vector>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include "utils/Utils.h"

#include "data/Bundle.h"
#include <ctime>

using namespace dtn::data;

namespace dtn
{
namespace testsuite
{
	PerformanceTestSuite::PerformanceTestSuite()
	{
	}

	PerformanceTestSuite::~PerformanceTestSuite()
	{
	}

	bool PerformanceTestSuite::runAllTests()
	{
		bool ret = true;
		cout << "PerformanceTestSuite... ";

		if ( !createBundleTest() )
		{
			cout << endl << "createBundleTest() failed" << endl;
			ret = false;
		}

#ifdef HAVE_LIBSQLITE3
		if ( !SQLiteSpeedTest() )
		{
			cout << endl << "SQLiteSpeedTest() failed" << endl;
			ret = false;
		}
#endif

		if (ret) cout << "\t\t\tpassed" << endl;

		return ret;
	}

	bool PerformanceTestSuite::createBundleTest()
	{
		unsigned int size = 1024;

		// Generiere Testdaten
		unsigned char *data = (unsigned char*)calloc(size, sizeof(unsigned char));

		// DEADBEEF, DE = 222, AD = 173, BE = 190, EF = 239
		unsigned char deadbeef[5];
		deadbeef[0] = (unsigned char)(222);
		deadbeef[1] = (unsigned char)(173);
		deadbeef[2] = (unsigned char)(190);
		deadbeef[3] = (unsigned char)(239);
		deadbeef[4] = (unsigned char)(0);

		for (unsigned int i = 0; i < size; i++)
		{
			data[i] = deadbeef[i % 5];
		}

		vector<Bundle*> bundles;

		// Starte Zeitmessung
		clock_t start = clock();

		for (int i = 0; i < 1000; i++)
		{
			BundleFactory &fac = BundleFactory::getInstance();
			Bundle *pbundle = fac.newBundle();
			PayloadBlock *pblock = PayloadBlockFactory::newPayloadBlock(data, size);
			pbundle->appendBlock(pblock);
			bundles.push_back( pbundle );
		}

		// Beende Zeitmessung
		clock_t finish = clock();

		free(data);

		// Bündel löschen
		vector<Bundle*>::iterator iter = bundles.begin();

		while (iter != bundles.end())
		{
			delete (*iter);
			iter++;
		}

		// Auswertung der Zeitmessung
		double time = (double(finish)-double(start)) / CLOCKS_PER_SEC;
		cout << endl << "completed with " << time << " seconds for 1000 bundles." << endl;

		return true;
	}

#ifdef HAVE_LIBSQLITE3

	bool PerformanceTestSuite::SQLiteSpeedTest()
	{
		unsigned int size = 1024;

		// Generiere Testdaten
		unsigned char *data = (unsigned char*)calloc(size, sizeof(unsigned char));

		// DEADBEEF, DE = 222, AD = 173, BE = 190, EF = 239
		unsigned char deadbeef[5];
		deadbeef[0] = (unsigned char)(222);
		deadbeef[1] = (unsigned char)(173);
		deadbeef[2] = (unsigned char)(190);
		deadbeef[3] = (unsigned char)(239);
		deadbeef[4] = (unsigned char)(0);

		for (unsigned int i = 0; i < size; i++)
		{
			data[i] = deadbeef[i % 5];
		}

		vector<Bundle*> bundles;
		for (int i = 0; i < 100; i++)
		{
			BundleFactory &fac = BundleFactory::getInstance();
			Bundle *pbundle = fac.newBundle();
			PayloadBlock *pblock = PayloadBlockFactory::newPayloadBlock(data, size);
			pbundle->appendBlock(pblock);
			bundles.push_back( pbundle );
		}

		// init test storage
		dtn::protocol::SQLiteBundleStorage store("database.db", true);

		// Starte Zeitmessung
		clock_t start = clock();

		// Bündel in der Storage ablegen
		vector<Bundle*>::iterator iter = bundles.begin();
		while (iter != bundles.end())
		{
			store.store( dtn::protocol::BundleSchedule( (*iter), 5, "dtn://test" ) );
			iter++;
		}

		// Beende Zeitmessung
		clock_t finish = clock();

		// vector löschen
		bundles.clear();

		double time = (double(finish)-double(start)) / CLOCKS_PER_SEC;
		cout << endl << "completed with " << time << " seconds for storing 1000 bundles." << endl;

		free(data);

		start = clock();

		try {
			while (true)
			{
				dtn::protocol::BundleSchedule s = store.getSchedule(10);
				delete s.getBundle();
			}
		} catch (dtn::exceptions::NoScheduleFoundException ex) {

		}

		finish = clock();

		// Auswertung der Zeitmessung
		time = (double(finish)-double(start)) / CLOCKS_PER_SEC;
		cout << endl << "completed with " << time << " seconds for getting 1000 bundles." << endl;

		return true;
	}
#endif

}
}

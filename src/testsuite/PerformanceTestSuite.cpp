#include "testsuite/PerformanceTestSuite.h"

#include <vector>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include "ibrdtn/utils/Utils.h"

#include "ibrdtn/data/Bundle.h"
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
//			if ( !SQLiteSpeedTest() )
//			{
//				cout << endl << "SQLiteSpeedTest() failed" << endl;
//				ret = false;
//			}
	#endif

			if (ret) cout << "\t\t\tpassed" << endl;

			return ret;
		}

		bool PerformanceTestSuite::createBundleTest()
		{
			unsigned int size = 1024;

			// Generiere Testdaten
			char *data = (char*)calloc(size, sizeof(char));

			// DEADBEEF, DE = 222, AD = 173, BE = 190, EF = 239
			char deadbeef[5];
			deadbeef[0] = (char)(222);
			deadbeef[1] = (char)(173);
			deadbeef[2] = (char)(190);
			deadbeef[3] = (char)(239);
			deadbeef[4] = (char)(0);

			for (unsigned int i = 0; i < size; i++)
			{
				data[i] = deadbeef[i % 5];
			}

			vector<Bundle> bundles;

			// Starte Zeitmessung
			clock_t start = clock();

			for (int i = 0; i < 1000; i++)
			{
				Bundle pbundle;
				PayloadBlock *pblock = new PayloadBlock();
				(*pblock->getBLOB()).write(data, size);
				pbundle.addBlock(pblock);
				bundles.push_back( pbundle );
			}

			// Beende Zeitmessung
			clock_t finish = clock();

			free(data);

			// Auswertung der Zeitmessung
			double time = (double(finish)-double(start)) / CLOCKS_PER_SEC;
			cout << endl << "completed with " << time << " seconds for 1000 bundles." << endl;

			return true;
		}
	}
}

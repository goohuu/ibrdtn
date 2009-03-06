#include "testsuite/SelfTestSuite.h"
#include "testsuite/DictionaryTestSuite.h"
#include "testsuite/BundleTestSuite.h"
#include "testsuite/StorageTestSuite.h"
#include "testsuite/NetworkFrameTestSuite.h"
#include "testsuite/CustodySignalBlockTestSuite.h"
#include "testsuite/PerformanceTestSuite.h"
#include "testsuite/ServiceTestSuite.h"
#include "testsuite/MeasurementTestSuite.h"
#include "testsuite/SQLiteTestSuite.h"
#include "testsuite/DiscoveryBlockTestSuite.h"
#include "utils/Utils.h"
#include "data/BundleFactory.h"

#include <iostream>

using namespace std;

namespace dtn
{
namespace testsuite
{
	SelfTestSuite::SelfTestSuite()
	{
	}

	SelfTestSuite::~SelfTestSuite()
	{
	}

	bool SelfTestSuite::runAllTests()
	{
		bool ret = true;

		NetworkFrameTestSuite frametest;
		if ( !frametest.runAllTests() ) ret = false;

		ServiceTestSuite servicetest;
		if ( !servicetest.runAllTests() ) ret = false;

		DictionaryTestSuite dicttest;
		if ( !dicttest.runAllTests() ) ret = false;

		BundleTestSuite bundletest;
		if ( !bundletest.runAllTests() ) ret = false;

		StorageTestSuite storagetest;
		if ( !storagetest.runAllTests() ) ret = false;

		CustodySignalBlockTestSuite custodytest;
		if ( !custodytest.runAllTests() ) ret = false;

#ifdef USE_EMMA_CODE
		MeasurementTestSuite measurementtest;
		if ( !measurementtest.runAllTests() ) ret = false;

		DiscoveryBlockTestSuite discoverytest;
		if ( !discoverytest.runAllTests() ) ret = false;
#endif

//		PerformanceTestSuite performtest;
//		if ( !performtest.runAllTests() ) ret = false;

#ifdef WITH_SQLITE
		SQLiteTestSuite sqlitetest;
		if ( !sqlitetest.runAllTests() ) ret = false;
#endif


		cout << "TestSuite completed!" << endl;

		return ret;
	}
}
}

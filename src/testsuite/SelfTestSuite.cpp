#include "testsuite/SelfTestSuite.h"
#include "testsuite/BundleTestSuite.h"
#include "testsuite/StorageTestSuite.h"
#include "testsuite/PerformanceTestSuite.h"
#include "testsuite/MeasurementTestSuite.h"
#include "testsuite/BlockTestSuite.h"
#include "testsuite/BLOBTestSuite.h"
#include "testsuite/BundleStreamTestSuite.h"
#include "testsuite/SerializeTestSuite.h"
#include "testsuite/StreamingTestSuite.h"
#include "ibrdtn/utils/Utils.h"


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

		SerializeTestSuite serializetest;
		if ( !serializetest.runAllTests() ) ret = false;

		BLOBTestSuite blobtest;
		if ( !blobtest.runAllTests() ) ret = false;

		BlockTestSuite blocktest;
		if ( !blocktest.runAllTests() ) ret = false;

		BundleTestSuite bundletest;
		if ( !bundletest.runAllTests() ) ret = false;

		BundleStreamTestSuite streamtest;
		if ( !streamtest.runAllTests() ) ret = false;

		StorageTestSuite storagetest;
		if ( !storagetest.runAllTests() ) ret = false;

		StreamingTestSuite streamingtest;
		if ( !streamingtest.runAllTests() ) ret = false;

#ifdef USE_EMMA_CODE
		MeasurementTestSuite measurementtest;
		if ( !measurementtest.runAllTests() ) ret = false;
#endif

		PerformanceTestSuite performtest;
		if ( !performtest.runAllTests() ) ret = false;

		cout << "TestSuite completed!" << endl;

		return ret;
	}
}
}

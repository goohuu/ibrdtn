#include "testsuite/ServiceTestSuite.h"
#include "testsuite/TestService.h"
#include <iostream>
#include "utils/Utils.h"

using namespace std;

namespace dtn
{
namespace testsuite
{
	ServiceTestSuite::ServiceTestSuite()
	{
	}

	ServiceTestSuite::~ServiceTestSuite()
	{
	}

	bool ServiceTestSuite::runAllTests()
	{
		bool ret = true;
		cout << "ServiceTestSuite... ";

		if ( !testTwoThreads() )
		{
			cout << endl << "testTwoThreads failed" << endl;
			ret = false;
		}

		if (ret) cout << "\t\t\tpassed" << endl;

		return ret;
	}

	bool ServiceTestSuite::testTwoThreads()
	{
		TestService t1("Test 1");
		TestService t2("Test 2");

		t1.start();
		t2.start();

		t1.waitFor();
		t2.waitFor();

		if (t1.worked() && t2.worked()) return true;

		return false;
	}
}
}

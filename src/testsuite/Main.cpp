#include <iostream>
#include "testsuite/SelfTestSuite.h"

using namespace std;
using namespace dtn;

int main(int argc, char *argv[])
{
	// run self test
	testsuite::SelfTestSuite test;
	if ( !test.runAllTests() )
	{
		cout << "Self-TestSuite failed!" << endl;
		cout << "Abort!" << endl;
		return -1;
	}

	return 0;
};

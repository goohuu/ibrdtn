#ifndef TESTUTILS_H_
#define TESTUTILS_H_

#include "data/BundleFactory.h"
#include "data/Bundle.h"

using namespace dtn::data;

namespace dtn
{
namespace testsuite
{
	class TestUtils
	{
		public:
			~TestUtils() {};
			static Bundle* createTestBundle(unsigned int size = 4);
			static void debugData(unsigned char* data, unsigned int size);
			static bool compareData(unsigned char *data1, unsigned char *data2, unsigned int size);
			static bool compareBundles(Bundle *b1, Bundle *b2);

		private:
			TestUtils();
	};
}
}

#endif /*TESTUTILS_H_*/

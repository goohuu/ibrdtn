#ifndef NETWORKFRAMETESTSUITE_H_
#define NETWORKFRAMETESTSUITE_H_

namespace dtn
{
namespace testsuite
{
	class NetworkFrameTestSuite
	{
		public:
			NetworkFrameTestSuite();

			~NetworkFrameTestSuite();

			bool runAllTests();

		private:
			bool instanceTest();
			bool appendDataTest();
			bool copyTest();
			bool removeTest();
			bool insertTest();
			bool sdnvTest();
	};
}
}

#endif /*NETWORKFRAMETESTSUITE_H_*/


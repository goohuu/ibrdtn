#ifndef CUSTODYSIGNALBLOCKTESTSUITE_H_
#define CUSTODYSIGNALBLOCKTESTSUITE_H_

namespace dtn
{
namespace testsuite
{
	class CustodySignalBlockTestSuite
	{
		public:
			CustodySignalBlockTestSuite();

			~CustodySignalBlockTestSuite();

			bool runAllTests();

		private:
			bool createTest();
			bool parseTest();
			bool fragmentTest();
			bool matchTest();
			bool sdnvTest();
	};
}
}

#endif /*CUSTODYSIGNALBLOCKTESTSUITE_H_*/

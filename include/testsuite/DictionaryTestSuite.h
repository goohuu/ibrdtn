#ifndef DICTIONARYTESTSUITE_H_
#define DICTIONARYTESTSUITE_H_

namespace dtn
{
namespace testsuite
{
	class DictionaryTestSuite
	{
		public:
			DictionaryTestSuite();
	
			~DictionaryTestSuite();
			
			bool runAllTests();
			
		private:
			bool addTest();
			bool readWriteTest();
	};
}
}

#endif /*DICTIONARYTESTSUITE_H_*/

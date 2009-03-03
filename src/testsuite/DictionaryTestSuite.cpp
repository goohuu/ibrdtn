#include "testsuite/DictionaryTestSuite.h"
#include "data/Dictionary.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include "utils/Utils.h"

using namespace dtn::data;

namespace dtn
{
namespace testsuite
{
	DictionaryTestSuite::DictionaryTestSuite()
	{
	}

	DictionaryTestSuite::~DictionaryTestSuite()
	{
	}

	bool DictionaryTestSuite::runAllTests()
	{
		bool ret = true;
		cout << "DictionaryTestSuite... ";

		if ( !addTest() )
		{
			cout << endl << "addTest() failed" << endl;
			ret = false;
		}

		if ( !readWriteTest() )
		{
			cout << endl << "readWriteTest() failed" << endl;
			ret = false;
		}

		if (ret) cout << "\t\t\tpassed" << endl;

		return ret;
	}

	bool DictionaryTestSuite::addTest()
	{
		Dictionary dict;
		string eid = "dtn:local";

		dict.add(eid);
		dict.add(eid);

		if ( dict.getLength() != 10 ) return false;

		return true;
	}

	bool DictionaryTestSuite::readWriteTest()
	{
		Dictionary dict;

		// Zwei Testeinträge anlegen
		dict.add("dtn:local");
		dict.add("dtn:network");

		// Länge des Dictionary merken
		unsigned int length = dict.getLength();

		unsigned char* dict_data = (unsigned char*)calloc(length, sizeof(unsigned char));
		unsigned int dict_size = dict.write(dict_data);

		dict.read(dict_data, dict_size);

		if ( length != dict.getLength() ) return false;

		free (dict_data);

		return true;
	}
}
}

/*
 * BLOBTestSuite.cpp
 *
 *  Created on: 19.06.2009
 *      Author: morgenro
 */

#include "testsuite/BLOBTestSuite.h"
#include "ibrdtn/data/BLOBManager.h"
#include "ibrdtn/data/BLOBReference.h"

using namespace dtn::blob;

namespace dtn
{
	namespace testsuite
	{
		BLOBTestSuite::BLOBTestSuite()
		{
		}

		BLOBTestSuite::~BLOBTestSuite()
		{
		}

		bool BLOBTestSuite::runAllTests()
		{
			bool ret = true;
			cout << "BLOBTestSuite... ";

			if ( !appendTest() )
			{
				cout << endl << "appendTest failed" << endl;
				ret = false;
			}

			if ( !writeTest() )
			{
				cout << endl << "writeTest failed" << endl;
				ret = false;
			}

			if ( !splitTest() )
			{
				cout << endl << "splitTest failed" << endl;
				ret = false;
			}

			if ( !mergeTest() )
			{
				cout << endl << "mergeTest failed" << endl;
				ret = false;
			}

			if (ret) cout << "\t\t\tpassed" << endl;

			return ret;
		}

		bool BLOBTestSuite::splitTest()
		{
			BLOBReference ref = BLOBManager::_instance.create(BLOBManager::BLOB_MEMORY);

			// create some testing data
			string test_string = "Hallo Welt!";

			// append data to blob
			ref.write(test_string.c_str(), 0, test_string.length());

			// split the data
			pair<BLOBReference, BLOBReference> refs = ref.split(5);

			if (refs.first.getSize() != 5)
			{
				cout << "wrong size of splitted blob" << endl;
				return false;
			}

			if (refs.second.getSize() != 6)
			{
				cout << "wrong size of splitted blob" << endl;
				return false;
			}

			// check the data
			char buffer[64];
			refs.first.read(buffer, 0, 5);

			buffer[5] = '\0';

			if (string(buffer) != "Hallo")
			{
				cout << "content not equal!" << endl;
				return false;
			}

			// check the data
			refs.second.read(buffer, 0, 6);

			buffer[6] = '\0';

			if (string(buffer) != " Welt!")
			{
				cout << "content not equal!" << endl;
				return false;
			}

			return true;
		}

		bool BLOBTestSuite::mergeTest()
		{
			BLOBReference ref1 = BLOBManager::_instance.create(BLOBManager::BLOB_MEMORY);
			BLOBReference ref2 = BLOBManager::_instance.create(BLOBManager::BLOB_MEMORY);

			// create some testing data
			string test_string1 = "Hallo ";
			string test_string2 = "Welt!";

			// append data to blob
			ref1.write(test_string1.c_str(), 0, test_string1.length());
			ref2.write(test_string2.c_str(), 0, test_string2.length());

			// merge both
			ref1.append(ref2);

			// check the size
			if (ref1.getSize() != test_string1.length() + test_string2.length())
			{
				cout << "wrong size of blob" << endl;
				return false;
			}

			// check the data
			char buffer[64];
			ref1.read(buffer, 0, 11);

			buffer[11] = '\0';

			if (string(buffer) != "Hallo Welt!")
			{
				cout << "content not equal!" << endl;
				return false;
			}

			return true;
		}

		bool BLOBTestSuite::writeTest()
		{
			BLOBReference ref = BLOBManager::_instance.create(BLOBManager::BLOB_MEMORY);

			// create some testing data
			string test_string = "Hallo Welt!";

			// append data to blob
			ref.write(test_string.c_str(), 0, test_string.length());

			// check the size
			if (ref.getSize() != test_string.length())
			{
				cout << "wrong size of blob" << endl;
				return false;
			}

			// append data to blob a second time
			ref.write(test_string.c_str(), test_string.length(), test_string.length());

			// check the size
			if (ref.getSize() != (2 * test_string.length()) )
			{
				cout << "wrong size of blob" << endl;
				return false;
			}

			char buffer[64];
			ref.read(buffer, 0, 64);

			buffer[2 * test_string.length()] = '\0';

			if (string(buffer) != (test_string + test_string))
			{
				cout << "content not equal!" << endl;
				return false;
			}

			return true;
		}

		bool BLOBTestSuite::appendTest()
		{
			BLOBReference ref = BLOBManager::_instance.create(BLOBManager::BLOB_MEMORY);

			// create some testing data
			string test_string = "Hallo Welt!";

			// append data to blob
			ref.append(test_string.c_str(), test_string.length());

			// check the size
			if (ref.getSize() != test_string.length())
			{
				cout << "wrong size of blob" << endl;
				return false;
			}

			// append data to blob a second time
			ref.append(test_string.c_str(), test_string.length());

			// check the size
			if (ref.getSize() != (2 *test_string.length()) )
			{
				cout << "wrong size of blob" << endl;
				return false;
			}

			// clear the blob
			ref.clear();

			// append data to blob
			ref.append(test_string.c_str(), test_string.length());

			// check the size
			if (ref.getSize() != test_string.length())
			{
				cout << "wrong size of blob" << endl;
				return false;
			}

			char buffer[64];
			ref.read(buffer, 0, 64);

			buffer[test_string.length()] = '\0';

			if (string(buffer) != test_string)
			{
				cout << "content not equal!" << endl;
				return false;
			}

			return true;
		}
	}
}

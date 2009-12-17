#include "testsuite/TestUtils.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include "ibrdtn/utils/Utils.h"

namespace dtn
{
namespace testsuite
{
	TestUtils::TestUtils()
	{

	}

	bool TestUtils::compareData(unsigned char *data1, unsigned char *data2, unsigned int size)
	{
		for (unsigned int i = 0; i < size; i++)
		{
			if ( data1[i] != data2[i] )
			{
				return false;
			}
		}

		return true;
	}

	Bundle TestUtils::createTestBundle(unsigned int size)
	{
		// Sende ein Testbundle
		char *data = (char*)calloc(size, sizeof(char));

		// DEADBEEF, DE = 222, AD = 173, BE = 190, EF = 239
		char deadbeef[5];
		deadbeef[0] = (char)(222);
		deadbeef[1] = (char)(173);
		deadbeef[2] = (char)(190);
		deadbeef[3] = (char)(239);
		deadbeef[4] = (char)(0);

		for (unsigned int i = 0; i < size; i++)
		{
			data[i] = deadbeef[i % 5];
		}

		dtn::data::Bundle out;
		Block *block = new PayloadBlock(ibrcommon::TmpFileBLOB::create());
		(*block->getBLOB()).write(data, size);

		out.addBlock( block );
		free(data);

		return out;
	}

	void TestUtils::debugData(unsigned char* data, unsigned int size)
	{
		//dtn::cout << "|";
		for (unsigned int i = 0; i < size; i++)
		{
			printf("%02X", (*data));
			//cout << (*data);
			data++;
		}
		//dtn::cout << "|" << endl;
	}

	bool TestUtils::compareBundles(Bundle b1, Bundle b2)
	{
		bool ret = true;

		if (b1._procflags != b2._procflags)
		{
			ret = false;
			cout << "primary flags not equal." << endl;
		}

		if (b1._procflags & Bundle::FRAGMENT)
		{
			if (b1._fragmentoffset != b2._fragmentoffset)
			{
				ret = false;
				cout << "fragmentation offset not equal." << endl;
			}

			if (b1._appdatalength != b2._appdatalength)
			{
				ret = false;
				cout << "application data length not equal." << endl;
			}
		}

		if (b1._source != b2._source)
		{
			ret = false;
			cout << "source eid not equal." << endl;
		}

		if (b1._destination != b2._destination)
		{
			ret = false;
			cout << "destination eid not equal." << endl;
		}

		if (b1._reportto != b2._reportto)
		{
			ret = false;
			cout << "reportto eid not equal." << endl;
		}

		if (b1._custodian != b2._custodian)
		{
			ret = false;
			cout << "custodian eid not equal." << endl;
		}

		dtn::data::PayloadBlock *p1 = utils::Utils::getPayloadBlock( b1 );
		dtn::data::PayloadBlock *p2 = utils::Utils::getPayloadBlock( b2 );

		if (p1->getSize() != p2->getSize())
		{
			ret = false;
			cout << "size of payload not equal." << endl;
		}

		if (p1->_procflags != p2->_procflags)
		{
			ret = false;
			cout << "block flags of the payload block not equal." << endl;
		}

		return ret;
	}
}
}

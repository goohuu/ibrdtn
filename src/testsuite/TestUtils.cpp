#include "testsuite/TestUtils.h"
#include "data/PayloadBlockFactory.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include "utils/Utils.h"

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

	Bundle* TestUtils::createTestBundle(unsigned int size)
	{
		// Sende ein Testbundle
		unsigned char *data = (unsigned char*)calloc(size, sizeof(unsigned char));

		// DEADBEEF, DE = 222, AD = 173, BE = 190, EF = 239
		unsigned char deadbeef[5];
		deadbeef[0] = (unsigned char)(222);
		deadbeef[1] = (unsigned char)(173);
		deadbeef[2] = (unsigned char)(190);
		deadbeef[3] = (unsigned char)(239);
		deadbeef[4] = (unsigned char)(0);

		for (unsigned int i = 0; i < size; i++)
		{
			data[i] = deadbeef[i % 5];
		}

		BundleFactory &fac = BundleFactory::getInstance();

		dtn::data::Bundle *out = fac.newBundle();
		out->appendBlock( PayloadBlockFactory::newPayloadBlock(data, size) );
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

	bool TestUtils::compareBundles(Bundle *b1, Bundle *b2)
	{
		bool ret = true;

		dtn::data::PrimaryFlags pb1 = b1->getPrimaryFlags();
		dtn::data::PrimaryFlags pb2 = b2->getPrimaryFlags();

		if (pb1.getValue() != pb2.getValue())
		{
			ret = false;
			cout << "primary flags not equal." << endl;
		}

		if (pb1.isFragment())
		{
			if (b1->getInteger(FRAGMENTATION_OFFSET) != b2->getInteger(FRAGMENTATION_OFFSET))
			{
				ret = false;
				cout << "fragmentation offset not equal." << endl;
			}

			if (b1->getInteger(APPLICATION_DATA_LENGTH) != b2->getInteger(APPLICATION_DATA_LENGTH))
			{
				ret = false;
				cout << "application data length not equal." << endl;
			}
		}

		if (b1->getSource() != b2->getSource())
		{
			ret = false;
			cout << "source eid not equal." << endl;
		}

		if (b1->getDestination() != b2->getDestination())
		{
			ret = false;
			cout << "destination eid not equal." << endl;
		}

		if (b1->getReportTo() != b2->getReportTo())
		{
			ret = false;
			cout << "reportto eid not equal." << endl;
		}

		if (b1->getCustodian() != b2->getCustodian())
		{
			ret = false;
			cout << "custodian eid not equal." << endl;
		}

		dtn::data::PayloadBlock *p1 = b1->getPayloadBlock();
		dtn::data::PayloadBlock *p2 = b2->getPayloadBlock();

		if (p1->getLength() != p2->getLength())
		{
			ret = false;
			cout << "size of payload not equal." << endl;
		}

		if (p1->getBlockFlags().getValue() != p2->getBlockFlags().getValue())
		{
			ret = false;
			cout << "block flags of the payload block not equal." << endl;
		}

		unsigned char *data1 = b1->getData();
		unsigned char *data2 = b2->getData();

		if ( !TestUtils::compareData(data1, data2, b1->getLength() ) )
		{
			ret = false;
			cout << endl;
			TestUtils::debugData(data1, 60);
			cout << endl;
			TestUtils::debugData(data2, 60);
			cout << endl;

			cout << "bundle data not equal!" << endl;
		}

		free(data1);
		free(data2);

		return ret;
	}
}
}

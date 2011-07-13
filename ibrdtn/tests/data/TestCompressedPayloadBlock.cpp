/*
 * TestCompressedPayloadBlock.cpp
 *
 *  Created on: 20.04.2011
 *      Author: morgenro
 */

#include "data/TestCompressedPayloadBlock.h"
#include <ibrdtn/data/Bundle.h>
#include <ibrdtn/data/PayloadBlock.h>
#include <ibrcommon/data/BLOB.h>

CPPUNIT_TEST_SUITE_REGISTRATION (TestCompressedPayloadBlock);

void TestCompressedPayloadBlock::setUp()
{
}

void TestCompressedPayloadBlock::tearDown()
{
}

void TestCompressedPayloadBlock::compressTest(void)
{
	dtn::data::Bundle b;
	ibrcommon::BLOB::Reference ref = ibrcommon::BLOB::create();

	// generate some test data
	{
		ibrcommon::BLOB::iostream stream = ref.iostream();
		for (int i = 0; i < 10000; i++)
		{
			(*stream) << "0123456789";
		}
	}

	// add a payload block
	size_t origin_psize = b.push_back(ref).getLength();

	dtn::data::CompressedPayloadBlock::compress(b, dtn::data::CompressedPayloadBlock::COMPRESSION_ZLIB);

	dtn::data::PayloadBlock &p = b.getBlock<dtn::data::PayloadBlock>();

	CPPUNIT_ASSERT(origin_psize > p.getLength());
}

void TestCompressedPayloadBlock::extractTest(void)
{
	dtn::data::Bundle b;
	ibrcommon::BLOB::Reference ref = ibrcommon::BLOB::create();

	// generate some test data
	{
		ibrcommon::BLOB::iostream stream = ref.iostream();
		for (int i = 0; i < 10000; i++)
		{
			(*stream) << "0123456789";
		}
	}

	// add a payload block
	size_t origin_psize = b.push_back(ref).getLength();

	dtn::data::CompressedPayloadBlock::compress(b, dtn::data::CompressedPayloadBlock::COMPRESSION_ZLIB);
	dtn::data::CompressedPayloadBlock::extract(b);

	dtn::data::PayloadBlock &p = b.getBlock<dtn::data::PayloadBlock>();

	CPPUNIT_ASSERT_EQUAL(origin_psize, p.getLength());

	// detailed check of the payload
	{
		ibrcommon::BLOB::iostream stream = p.getBLOB().iostream();
		for (int i = 0; i < 10000; i++)
		{
			char buf[10];
			(*stream).read(buf, 10);
			CPPUNIT_ASSERT_EQUAL((size_t)(*stream).gcount(), (size_t)10);
			CPPUNIT_ASSERT_EQUAL(std::string(buf, 10), std::string("0123456789"));
		}
	}
}

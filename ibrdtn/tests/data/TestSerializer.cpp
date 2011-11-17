/*
 * TestSerializer.cpp
 *
 *  Created on: 30.07.2010
 *      Author: morgenro
 */

#include "data/TestSerializer.h"
#include <ibrdtn/data/EID.h>
#include <cppunit/extensions/HelperMacros.h>

#include <ibrcommon/thread/MutexLock.h>
#include <ibrdtn/data/Bundle.h>
#include <ibrdtn/data/Serializer.h>
#include <ibrdtn/data/BundleFragment.h>
#include <iostream>
#include <sstream>

CPPUNIT_TEST_SUITE_REGISTRATION (TestSerializer);

void TestSerializer::setUp(void)
{
}

void TestSerializer::tearDown(void)
{
}

void TestSerializer::serializer_separate01(void)
{
	dtn::data::Bundle b;
	ibrcommon::BLOB::Reference ref = ibrcommon::BLOB::create();

	stringstream ss1;
	dtn::data::PayloadBlock &p1 = b.push_back(ref);
	p1.addEID(dtn::data::EID("dtn://test1234/app1234"));

	stringstream ss2;
	dtn::data::PayloadBlock &p2 = b.push_back(ref);
	p2.addEID(dtn::data::EID("dtn://test1234/app1234"));

	dtn::data::SeparateSerializer(ss1) << p1;
	dtn::data::SeparateSerializer(ss2) << p2;

	ss1.clear(); ss1.seekp(0); ss1.seekg(0);
	ss2.clear(); ss2.seekp(0); ss2.seekg(0);

	dtn::data::Bundle b2;

	dtn::data::SeparateDeserializer(ss1, b2).readBlock();
	dtn::data::SeparateDeserializer(ss2, b2).readBlock();

	CPPUNIT_ASSERT_EQUAL( b2.getBlocks().size(), b.getBlocks().size() );
}

void TestSerializer::serializer_cbhe01(void)
{
	dtn::data::Bundle b;

	dtn::data::EID src("ipn:1.2");
	dtn::data::EID dst("ipn:2.3");
	dtn::data::EID report("ipn:6.1");

	std::pair<size_t, size_t> cbhe_eid = src.getCompressed();

	b._source = src;
	b._destination = dst;
	b._reportto = report;

	ibrcommon::BLOB::Reference ref = ibrcommon::BLOB::create();
	b.push_back(ref);

	std::stringstream ss;
	dtn::data::DefaultSerializer(ss) << b;

	dtn::data::Bundle b2;
	ss.clear(); ss.seekp(0); ss.seekg(0);
	dtn::data::DefaultDeserializer(ss) >> b2;

	CPPUNIT_ASSERT( b._source == b2._source );
	CPPUNIT_ASSERT( b._destination == b2._destination );
	CPPUNIT_ASSERT( b._reportto == b2._reportto );
	CPPUNIT_ASSERT( b._custodian == b2._custodian );
}

void TestSerializer::serializer_cbhe02(void)
{
	dtn::data::Bundle b;

	dtn::data::EID src("ipn:1.2");
	dtn::data::EID dst("ipn:2.3");

	b._source = src;
	b._destination = dst;

	ibrcommon::BLOB::Reference ref = ibrcommon::BLOB::create();

	{
		ibrcommon::BLOB::iostream stream = ref.iostream();
		(*stream) << "0123456789";
	}

	b.push_back(ref);

	std::stringstream ss;
	dtn::data::DefaultSerializer(ss) << b;

	CPPUNIT_ASSERT_EQUAL((size_t)33, ss.str().length());
}

void TestSerializer::serializer_bundle_length(void)
{
	dtn::data::Bundle b;
	b._source = dtn::data::EID("dtn://node1/app1");
	b._destination = dtn::data::EID("dtn://node2/app2");
	b._lifetime = 3600;
	b._timestamp = 12345678;
	b._sequencenumber = 1234;

	std::stringstream ss;
	dtn::data::DefaultSerializer ds(ss);
	ds << b;

	CPPUNIT_ASSERT_EQUAL(ds.getLength(b), ss.str().length());
}

void TestSerializer::serializer_fragment_one(void)
{
	dtn::data::Bundle b;
	b._source = dtn::data::EID("dtn://node1/app1");
	b._destination = dtn::data::EID("dtn://node2/app2");
	b._lifetime = 3600;
	b._timestamp = 12345678;
	b._sequencenumber = 1234;

	std::stringstream ss;
	dtn::data::DefaultSerializer ds(ss);

	ibrcommon::BLOB::Reference ref = ibrcommon::BLOB::create();

	// generate some payload data
	{
		ibrcommon::BLOB::iostream ios = ref.iostream();
		for (int i = 0; i < 100; i++)
		{
			(*ios) << "0123456789";
		}
	}

	// add a payload block to the bundle
	b.push_back(ref);

	// serialize the bundle in fragments with only 100 bytes of payload
	for (int i = 0; i < 10; i++)
	{
		ds << dtn::data::BundleFragment(b, i * 100, 100);
	}

	// jump to the first byte in the stringstream
	ss.seekg(0);

	for (int i = 0; i < 10; i++)
	{
		dtn::data::Bundle fb;
		dtn::data::DefaultDeserializer(ss) >> fb;
		CPPUNIT_ASSERT(fb.get(dtn::data::PrimaryBlock::FRAGMENT));
		CPPUNIT_ASSERT_EQUAL(fb.getBlock<dtn::data::PayloadBlock>().getLength(), (size_t)100);
	}
}

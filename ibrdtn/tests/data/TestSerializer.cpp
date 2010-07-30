/*
 * TestSerializer.cpp
 *
 *  Created on: 30.07.2010
 *      Author: morgenro
 */

#include "data/TestSerializer.h"
#include <ibrdtn/data/EID.h>
#include <cppunit/extensions/HelperMacros.h>

#include <ibrdtn/data/Bundle.h>
#include <ibrdtn/data/Serializer.h>
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
	ibrcommon::BLOB::Reference ref = ibrcommon::TmpFileBLOB::create();

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

	assert( b2.getBlocks().size() == b.getBlocks().size() );
}

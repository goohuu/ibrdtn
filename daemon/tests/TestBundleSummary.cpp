/*
 * TestBundleSummary.cpp
 *
 *  Created on: 27.07.2010
 *      Author: morgenro
 */

#include "tests/TestBundleSummary.h"
#include <ibrdtn/utils/Clock.h>
#include "src/routing/BundleSummary.h"
#include <sstream>

CPPUNIT_TEST_SUITE_REGISTRATION (TestBundleSummary);

void TestBundleSummary::setUp()
{
	dtn::utils::Clock::quality = 1;
}

void TestBundleSummary::tearDown()
{
}

void TestBundleSummary::expireTest(void)
{
	dtn::routing::BundleSummary l;

	CPPUNIT_ASSERT(l.size() == 0);

	genbundles(l, 1000, 0, 500);
	genbundles(l, 1000, 600, 1000);

	for (int i = 0; i < 550; i++)
	{
		l.expire(i);
	}

	CPPUNIT_ASSERT(l.size() == 1000);

	for (int i = 0; i < 1050; i++)
	{
		l.expire(i);
	}

	CPPUNIT_ASSERT(l.size() == 0);

//	std::stringstream bf; bf << l.getSummaryVector() << std::flush;
//	std::cout << getHex(bf) << std::endl;
}

void TestBundleSummary::genbundles(dtn::routing::BundleSummary &l, int number, int offset, int max)
{
	int range = max - offset;
	dtn::data::Bundle b;

	for (int i = 0; i < number; i++)
	{
		int random_integer = offset + (rand() % range);

		b._lifetime = random_integer;
		b._timestamp = 0;
		b._sequencenumber = random_integer;

		stringstream ss; ss << rand();

		b._source = dtn::data::EID("dtn://node" + ss.str() + "/application");

		l.add(b);
	}
}

std::string TestBundleSummary::getHex(std::istream &stream)
{
	std::stringstream buf;
	unsigned int c = stream.get();

	while (!stream.eof())
	{
		buf << std::hex << c << ":";
		c = stream.get();
	}

	buf << std::flush;

	return buf.str();
}


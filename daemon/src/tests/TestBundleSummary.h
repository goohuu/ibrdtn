/*
 * TestBundleSummary.h
 *
 *  Created on: 27.07.2010
 *      Author: morgenro
 */

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "routing/BundleSummary.h"
#include <iostream>

#ifndef TESTBUNDLESUMMARY_H_
#define TESTBUNDLESUMMARY_H_

class TestBundleSummary : public CPPUNIT_NS :: TestFixture
{
	CPPUNIT_TEST_SUITE (TestBundleSummary);
	CPPUNIT_TEST (expireTest);
	CPPUNIT_TEST_SUITE_END ();

public:
	void setUp (void);
	void tearDown (void);

protected:
	void expireTest(void);

private:
	void genbundles(dtn::routing::BundleSummary &l, int number, int offset, int max);
	std::string getHex(std::istream &stream);
};

#endif /* TESTBUNDLESUMMARY_H_ */

/*
 * DataStorageTest.h
 *
 *  Created on: 22.11.2010
 *      Author: morgenro
 */

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#ifndef DATASTORAGETEST_H_
#define DATASTORAGETEST_H_

class DataStorageTest : public CppUnit::TestFixture
{
public:
	void testBasics();

	void setUp();
	void tearDown();

	CPPUNIT_TEST_SUITE(DataStorageTest);
	CPPUNIT_TEST(testBasics);
	CPPUNIT_TEST_SUITE_END();
};

#endif /* DATASTORAGETEST_H_ */

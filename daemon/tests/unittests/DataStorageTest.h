/*
 * DataStorageTest.h
 *
 *  Created on: 22.11.2010
 *      Author: morgenro
 */

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "src/core/DataStorage.h"

#ifndef DATASTORAGETEST_H_
#define DATASTORAGETEST_H_

class DataStorageTest : public CppUnit::TestFixture
{
public:
	void testHashTest();
	void testStoreTest();
	void testRemoveTest();

	void setUp();
	void tearDown();

	CPPUNIT_TEST_SUITE(DataStorageTest);
	CPPUNIT_TEST(testHashTest);
	CPPUNIT_TEST(testStoreTest);
	CPPUNIT_TEST(testRemoveTest);
	CPPUNIT_TEST_SUITE_END();

private:
	class DataCallbackDummy : public dtn::core::DataStorage::Callback
	{
	public:
		DataCallbackDummy() {};
		virtual ~DataCallbackDummy() {};

		void eventDataStorageStored(const dtn::core::DataStorage::Hash&) {};
		void eventDataStorageStoreFailed(const dtn::core::DataStorage::Hash&) {};
		void eventDataStorageRemoved(const dtn::core::DataStorage::Hash&) {};
		void eventDataStorageRemoveFailed(const dtn::core::DataStorage::Hash&) {};
		void iterateDataStorage(const dtn::core::DataStorage::Hash&, dtn::core::DataStorage::istream&) {};
	};
};

#endif /* DATASTORAGETEST_H_ */

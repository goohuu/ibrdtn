/*
 * DataStorageTest.cpp
 *
 *  Created on: 22.11.2010
 *      Author: morgenro
 */

#include "DataStorageTest.h"
#include "src/core/DataStorage.h"

CPPUNIT_TEST_SUITE_REGISTRATION(DataStorageTest);

void DataStorageTest::testBasics()
{
	class DataCallback : public dtn::core::DataStorage::Callback
	{
	public:
		DataCallback() {};
		virtual ~DataCallback() {};

		void eventDataStorageStored(const dtn::core::DataStorage::Hash&) {};
		void eventDataStorageStoreFailed(const dtn::core::DataStorage::Hash&) {};
		void eventDataStorageRemoved(const dtn::core::DataStorage::Hash&) {};
		void eventDataStorageRemoveFailed(const dtn::core::DataStorage::Hash&) {};
		void iterateDataStorage(const dtn::core::DataStorage::Hash&, dtn::core::DataStorage::istream&) {};
	};

	class DataContainer : public dtn::core::DataStorage::Container
	{
	public:
		DataContainer() {};
		~DataContainer() {};

		std::string getKey() const
		{
			return "[123456789.12] dtn://test/tester";
		}

		std::ostream& serialize(std::ostream &stream)
		{
			stream << "data";
		}
	};

	DataCallback callback;
	ibrcommon::File datapath("/tmp/datastorage");
	dtn::core::DataStorage storage(callback, datapath, true);

	dtn::core::DataStorage::Hash h = storage.store(new DataContainer());

	CPPUNIT_ASSERT_EQUAL(std::string("5b3132333435363738392e31325d2064746e3a2f2f746573742f746573746572"), h.value);
}

void DataStorageTest::setUp()
{
}

void DataStorageTest::tearDown()
{
}

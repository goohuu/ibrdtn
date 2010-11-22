/*
 * DataStorageTest.cpp
 *
 *  Created on: 22.11.2010
 *      Author: morgenro
 */

#include "DataStorageTest.h"
#include <ibrcommon/thread/Mutex.h>
#include <ibrcommon/thread/MutexLock.h>
#include <ibrcommon/thread/Conditional.h>

CPPUNIT_TEST_SUITE_REGISTRATION(DataStorageTest);

void DataStorageTest::testHashTest()
{
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
			return stream;
		}
	};

	DataCallbackDummy callback;
	ibrcommon::File datapath("/tmp/datastorage");
	dtn::core::DataStorage storage(callback, datapath, true);

	dtn::core::DataStorage::Hash h = storage.store(new DataContainer());

	CPPUNIT_ASSERT_EQUAL(std::string("5b3132333435363738392e31325d2064746e3a2f2f746573742f746573746572"), h.value);
}

void DataStorageTest::testStoreTest()
{
	class DataCallback : public dtn::core::DataStorage::Callback
	{
	public:
		DataCallback() : stored(false) {};
		virtual ~DataCallback() {};

		void wait()
		{
			ibrcommon::MutexLock l(_cond);
			if (stored) return;
			_cond.wait();
		}

		void eventDataStorageStored(const dtn::core::DataStorage::Hash&)
		{
			ibrcommon::MutexLock l(_cond);
			stored = true;
			_cond.signal(true);
		};

		void eventDataStorageStoreFailed(const dtn::core::DataStorage::Hash&) {};
		void eventDataStorageRemoved(const dtn::core::DataStorage::Hash&) {};
		void eventDataStorageRemoveFailed(const dtn::core::DataStorage::Hash&) {};
		void iterateDataStorage(const dtn::core::DataStorage::Hash&, dtn::core::DataStorage::istream&) {};

		bool stored;
		ibrcommon::Conditional _cond;
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
			return stream;
		}
	};

	DataCallback callback;
	ibrcommon::File datapath("/tmp/datastorage");
	dtn::core::DataStorage storage(callback, datapath, true);

	// startup the storage
	storage.start();

	// store some data in the storage
	dtn::core::DataStorage::Hash h = storage.store(new DataContainer());

	// wait until all data is stored
	callback.wait();

	// get the data
	dtn::core::DataStorage::istream s = storage.retrieve(h);

	// copy the data
	std::stringstream ss; ss << (*s).rdbuf();

	// check the data
	CPPUNIT_ASSERT_EQUAL(std::string("data"), ss.str());
}

void DataStorageTest::testRemoveTest()
{
	class DataCallback : public dtn::core::DataStorage::Callback
	{
	public:
		DataCallback() : stored(false) {};
		virtual ~DataCallback() {};

		void wait()
		{
			ibrcommon::MutexLock l(_cond);
			if (stored) return;
			_cond.wait();
			stored = false;
		}

		void eventDataStorageStored(const dtn::core::DataStorage::Hash&)
		{
			ibrcommon::MutexLock l(_cond);
			stored = true;
			_cond.signal(true);
		};

		void eventDataStorageStoreFailed(const dtn::core::DataStorage::Hash&) {};
		void eventDataStorageRemoved(const dtn::core::DataStorage::Hash&)
		{
			ibrcommon::MutexLock l(_cond);
			stored = true;
			_cond.signal(true);
		};

		void eventDataStorageRemoveFailed(const dtn::core::DataStorage::Hash&) {};
		void iterateDataStorage(const dtn::core::DataStorage::Hash&, dtn::core::DataStorage::istream&) {};

		bool stored;
		ibrcommon::Conditional _cond;
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
			return stream;
		}
	};

	DataCallback callback;
	ibrcommon::File datapath("/tmp/datastorage");
	dtn::core::DataStorage storage(callback, datapath, true);

	// startup the storage
	storage.start();

	// store some data in the storage
	dtn::core::DataStorage::Hash h = storage.store(new DataContainer());

	// wait until all data is stored
	callback.wait();

	// get the data
	CPPUNIT_ASSERT_NO_THROW(storage.retrieve(h));

	// delete the data
	storage.remove(h);

	// wait until all data is removed
	callback.wait();

	// the data should be gone
	CPPUNIT_ASSERT_THROW(storage.retrieve(h), dtn::core::DataStorage::DataNotAvailableException);
}

void DataStorageTest::setUp()
{
}

void DataStorageTest::tearDown()
{
}

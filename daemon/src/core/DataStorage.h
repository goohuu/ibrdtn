/*
 * DataStorage.h
 *
 *  Created on: 22.11.2010
 *      Author: morgenro
 */

#include <iostream>
#include <fstream>
#include <string>
#include <ibrcommon/data/File.h>
#include <ibrcommon/thread/Mutex.h>
#include <ibrcommon/thread/MutexLock.h>
#include <ibrcommon/thread/Queue.h>
#include <ibrcommon/thread/Thread.h>
#include <ibrcommon/thread/Semaphore.h>

#ifndef DATASTORAGE_H_
#define DATASTORAGE_H_

namespace dtn
{
	namespace core
	{
		class DataStorage : public ibrcommon::JoinableThread
		{
		public:
			class DataNotAvailableException : public ibrcommon::Exception
			{
			public:
				DataNotAvailableException(string what = "Requested data is not available.") throw() : Exception(what)
				{ };
			};

			class Container
			{
			public:
				virtual ~Container() = 0;
				virtual std::string getKey() const = 0;
				virtual std::ostream& serialize(std::ostream &stream) = 0;
			};

			class Hash
			{
			public:
				Hash();
				Hash(const std::string &key);
				Hash(const DataStorage::Container &container);
				Hash(const ibrcommon::File &file);
				virtual ~Hash();

				bool operator==(const Hash &other) const;
				bool operator<(const Hash &other) const;

				std::string value;

			private:
				static std::string hash(const std::string &value);
			};

			class istream : public ibrcommon::File
			{
			public:
				istream(ibrcommon::Mutex &mutex, const ibrcommon::File &file);
				virtual ~istream();
				std::istream& operator*();

			private:
				std::ifstream *_stream;
				ibrcommon::Mutex &_lock;
			};

			class Callback
			{
			public:
				virtual void eventDataStorageStored(const Hash &hash) = 0;
				virtual void eventDataStorageStoreFailed(const Hash &hash, const ibrcommon::Exception&) = 0;
				virtual void eventDataStorageRemoved(const Hash &hash) = 0;
				virtual void eventDataStorageRemoveFailed(const Hash &hash, const ibrcommon::Exception&) = 0;
				virtual void iterateDataStorage(const Hash &hash, DataStorage::istream &stream) = 0;
			};

			DataStorage(Callback &callback, const ibrcommon::File &path, size_t write_buffer = 0, bool initialize = false);
			virtual ~DataStorage();

			const Hash store(Container *data);
			void store(const Hash &hash, Container *data);

			DataStorage::istream retrieve(const Hash &hash) throw (DataNotAvailableException);
			void remove(const Hash &hash);

			void iterateAll();

		protected:
			void run();
			bool __cancellation();

		private:
			class Task
			{
			public:
				virtual ~Task() = 0;
			};

			class StoreDataTask : public Task
			{
			public:
				StoreDataTask(const Hash &h, Container *c);
				virtual ~StoreDataTask();

				const Hash hash;
				Container *container;
			};

			class RemoveDataTask : public Task
			{
			public:
				RemoveDataTask(const Hash &h);
				virtual ~RemoveDataTask();

				const Hash hash;
			};

			Callback &_callback;
			ibrcommon::File _path;
			ibrcommon::Queue< Task* > _tasks;
			ibrcommon::Semaphore _store_sem;
			bool _store_limited;

			ibrcommon::Mutex _global_mutex;
		};
	}
}

#endif /* DATASTORAGE_H_ */

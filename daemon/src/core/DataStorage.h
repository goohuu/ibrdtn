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

			class Hash
			{
			public:
				Hash(const std::string &value);
				Hash(const ibrcommon::File &file);
				virtual ~Hash();

				bool operator==(const Hash &other) const;
				bool operator<(const Hash &other) const;

				const std::string value;
			};

			class Container
			{
			public:
				Container() {};
				virtual ~Container() = 0;
				virtual std::ostream& serialize(std::ostream &stream) = 0;
			};

			class istream
			{
			public:
				istream(const istream &other);
				istream(ibrcommon::Mutex &mutex, const ibrcommon::File &file);
				istream(ibrcommon::Mutex &mutex, const ibrcommon::File &file, bool dummy);
				virtual ~istream();
				std::istream& operator*();

			private:
				std::ifstream *_stream;
				ibrcommon::File _file;
				ibrcommon::Mutex &_lock;
			};

			class Callback
			{
			public:
				virtual void eventDataStorageStored(const Hash &hash) = 0;
				virtual void eventDataStorageStoreFailed(const Hash &hash) = 0;
				virtual void eventDataStorageRemoved(const Hash &hash) = 0;
				virtual void eventDataStorageRemoveFailed(const Hash &hash) = 0;
				virtual void iterateDataStorage(const Hash &hash, DataStorage::istream &stream);
			};

			DataStorage(Callback &callback, const ibrcommon::File &path, bool initialize = false);
			virtual ~DataStorage();

			const Hash store(Container *data);
			DataStorage::istream retrieve(const Hash &hash) throw (DataNotAvailableException);
			void remove(const Hash &hash);

			void iterateAll();

		protected:
			void run();

		private:
			class Task
			{
			public:
				virtual ~Task() {};
			};

			class StoreDataTask : public Task
			{
			public:
				StoreDataTask(const Hash &h, Container *c)
				 : hash(h), container(c) {};

				virtual ~StoreDataTask() {
					delete container;
				};

				const Hash hash;
				Container *container;
			};

			class RemoveDataTask : public Task
			{
			public:
				RemoveDataTask(const Hash &h) : hash(h) {};
				virtual ~RemoveDataTask() {};

				const Hash hash;
			};

			Callback &_callback;
			ibrcommon::File _path;
			ibrcommon::Queue< Task* > _tasks;

			ibrcommon::Mutex _global_mutex;
		};
	}
}

#endif /* DATASTORAGE_H_ */

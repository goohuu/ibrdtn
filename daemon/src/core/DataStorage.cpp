/*
 * DataStorage.cpp
 *
 *  Created on: 22.11.2010
 *      Author: morgenro
 */

#include "core/DataStorage.h"
#include <typeinfo>
#include <list>

namespace dtn
{
	namespace core
	{
		DataStorage::Hash::Hash(const std::string &v) : value(v) {};
		DataStorage::Hash::Hash(const ibrcommon::File &file) : value(file.getBasename()) {};
		DataStorage::Hash::~Hash() {};

		bool DataStorage::Hash::operator==(const DataStorage::Hash &other) const
		{
			return (value == other.value);
		}

		bool DataStorage::Hash::operator<(const DataStorage::Hash &other) const
		{
			return (value < other.value);
		}

		DataStorage::istream::istream(ibrcommon::Mutex &mutex, const ibrcommon::File &file)
		 : _stream(NULL), _file(file), _lock(mutex)
		{
			_lock.enter();
			_stream = new std::ifstream(file.getPath().c_str(), ios_base::in | ios_base::binary);
		};

		DataStorage::istream::istream(ibrcommon::Mutex &mutex, const ibrcommon::File &file, bool)
		 : _stream(NULL), _file(file), _lock(mutex)
		{
		}

		DataStorage::istream::~istream()
		{
			if (_stream != NULL)
			{
				delete _stream;
				_lock.leave();
			}
		};

		DataStorage::istream::istream(const istream &other)
		 : _file(other._file), _lock(other._lock)
		{
			_lock.enter();
			_stream = new std::ifstream(_file.getPath().c_str(), ios_base::in | ios_base::binary);
		}

		std::istream& DataStorage::istream::operator*()
		{ return *_stream; }

		DataStorage::DataStorage(Callback &callback, const ibrcommon::File &path, bool initialize)
		 : _callback(callback), _path(path)
		{
			// initialize the storage
			if (initialize)
			{
				if (_path.exists())
				{
					// remove all files in the path
					std::list<ibrcommon::File> files;
					_path.getFiles(files);

					for (std::list<ibrcommon::File>::const_iterator iter = files.begin(); iter != files.end(); iter++)
					{
						//(*iter).remove(true);
						std::cout << "removing file " << (*iter).getPath() << std::endl;
					}
				}
				else
				{
					// create the path
					ibrcommon::File::createDirectory(_path);
				}
			}
		}

		DataStorage::~DataStorage()
		{
			_tasks.abort();
			join();
		}

		void DataStorage::iterateAll()
		{
			std::list<ibrcommon::File> files;
			_path.getFiles(files);

			for (std::list<ibrcommon::File>::const_iterator iter = files.begin(); iter != files.end(); iter++)
			{
				DataStorage::Hash hash(*iter);
				DataStorage::istream stream(_global_mutex, *iter);
				_callback.iterateDataStorage(hash, stream);
			}
		}

		const DataStorage::Hash DataStorage::store(DataStorage::Container *data)
		{
			// create a corresponding hash
			DataStorage::Hash hash("12345");

			_tasks.push( new StoreDataTask(hash, data) );

			return hash;
		}

		DataStorage::istream DataStorage::retrieve(const DataStorage::Hash &hash) throw (DataNotAvailableException)
		{
			ibrcommon::File file = _path.get(hash.value);

			if (!file.exists())
			{
				throw DataNotAvailableException();
			}

			// create a dummy, this object will lock if it gets copied
			return DataStorage::istream(_global_mutex, file, true);
		}

		void DataStorage::remove(const DataStorage::Hash &hash)
		{
			_tasks.push( new RemoveDataTask(hash) );
		}

		void DataStorage::run()
		{
			try {
				while (true)
				{
					Task *t = _tasks.getnpop(true);

					try {
						StoreDataTask &store = dynamic_cast<StoreDataTask&>(*t);

						try {
							ibrcommon::File destination = _path.get(store.hash.value);
							std::ofstream stream(destination.getPath().c_str(), ios_base::out | ios_base::binary);
							store.container->serialize(stream);
						} catch (const ibrcommon::Exception&) {
							_callback.eventDataStorageStoreFailed(store.hash);
						}
					} catch (const std::bad_cast&) {

					}

					try {
						RemoveDataTask &remove = dynamic_cast<RemoveDataTask&>(*t);

						try {
							ibrcommon::File destination = _path.get(remove.hash.value);
							if (!destination.exists())
							{
								throw DataNotAvailableException();
							}
							destination.remove();
						} catch (const ibrcommon::Exception&) {
							_callback.eventDataStorageRemoveFailed(remove.hash);
						}
					} catch (const std::bad_cast&) {

					}

					delete t;
				}
			} catch (const ibrcommon::QueueUnblockedException&) {
				// exit
			}
		}
	}
}

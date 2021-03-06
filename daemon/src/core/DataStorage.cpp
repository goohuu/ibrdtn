/*
 * DataStorage.cpp
 *
 *  Created on: 22.11.2010
 *      Author: morgenro
 */

#include "core/DataStorage.h"
#include <typeinfo>
#include <sstream>
#include <iomanip>
#include <list>

#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cerrno>

namespace dtn
{
	namespace core
	{
		DataStorage::Hash::Hash()
		 : value("this-hash-value-is-empty")
		{}

		DataStorage::Hash::Hash(const std::string &key)
		 : value(DataStorage::Hash::hash(key))
		{ }

		DataStorage::Hash::Hash(const DataStorage::Container &container)
		 : value(DataStorage::Hash::hash(container.getKey()))
		{ };

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

		std::string DataStorage::Hash::hash(const std::string &value)
		{
			std::stringstream ss;
			for (std::string::const_iterator iter = value.begin(); iter != value.end(); iter++)
			{
				ss << std::hex << std::setw( 2 ) << std::setfill( '0' ) << (int)(*iter);
			}
			return ss.str();
		}

		DataStorage::istream::istream(ibrcommon::Mutex &mutex, const ibrcommon::File &file)
		 : ibrcommon::File(file), _stream(NULL), _lock(mutex)
		{
			_lock.enter();
			_stream = new std::ifstream(getPath().c_str(), ios_base::in | ios_base::binary);
		};

		DataStorage::istream::~istream()
		{
			if (_stream != NULL)
			{
				delete _stream;
				_lock.leave();
			}
		};

		std::istream& DataStorage::istream::operator*()
		{ return *_stream; }

		DataStorage::DataStorage(Callback &callback, const ibrcommon::File &path, size_t write_buffer, bool initialize)
		 : _callback(callback), _path(path), _tasks(), _store_sem(write_buffer), _store_limited(write_buffer > 0)
		// limit the number of bundles in the write buffer
		{
			// initialize the storage
			if (initialize)
			{
				if (_path.exists())
				{
					// remove all files in the path
					std::list<ibrcommon::File> files;
					_path.getFiles(files);

					for (std::list<ibrcommon::File>::iterator iter = files.begin(); iter != files.end(); iter++)
					{
						(*iter).remove(true);
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

			// delete all task objects
			try {
				while (true)
				{
					Task *t = _tasks.getnpop(false);
					delete t;
				}
			} catch (const ibrcommon::QueueUnblockedException&) {
				// exit
			}
		}

		void DataStorage::iterateAll()
		{
			std::list<ibrcommon::File> files;
			_path.getFiles(files);

			for (std::list<ibrcommon::File>::const_iterator iter = files.begin(); iter != files.end(); iter++)
			{
				if (!(*iter).isSystem())
				{
					DataStorage::Hash hash(*iter);
					DataStorage::istream stream(_global_mutex, *iter);

					_callback.iterateDataStorage(hash, stream);
				}
			}
		}

		void DataStorage::store(const DataStorage::Hash &hash, DataStorage::Container *data)
		{
			// wait for resources
			if (_store_limited) _store_sem.wait();

			// put the task into the queue
			_tasks.push( new StoreDataTask(hash, data) );
		}

		const DataStorage::Hash DataStorage::store(DataStorage::Container *data)
		{
			// create a corresponding hash
			DataStorage::Hash hash(*data);
			store(hash, data);
			return hash;
		}

		DataStorage::istream DataStorage::retrieve(const DataStorage::Hash &hash) throw (DataNotAvailableException)
		{
			ibrcommon::File file = _path.get(hash.value);

			if (!file.exists())
			{
				throw DataNotAvailableException();
			}

			return DataStorage::istream(_global_mutex, file);
		}

		void DataStorage::remove(const DataStorage::Hash &hash)
		{
			_tasks.push( new RemoveDataTask(hash) );
		}

		bool DataStorage::__cancellation()
		{
			_tasks.abort();
			return true;
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

							{
								ibrcommon::MutexLock l(_global_mutex);
								std::ofstream stream(destination.getPath().c_str(), ios::out | ios::binary | ios::trunc);

								// check the streams health
								if (!stream.good())
								{
									std::stringstream ss; ss << "unable to open filestream [" << std::strerror(errno) << "]";
									throw ibrcommon::IOException(ss.str());
								}

								store.container->serialize(stream);
								stream.close();
							}

							// release resources
							if (_store_limited) _store_sem.post();

							// notify the stored item
							_callback.eventDataStorageStored(store.hash);
						} catch (const ibrcommon::Exception &ex) {
							// release resources
							if (_store_limited) _store_sem.post();

							// notify the fail of store action
							_callback.eventDataStorageStoreFailed(store.hash, ex);
						}
					} catch (const std::bad_cast&) {
					}

					try {
						RemoveDataTask &remove = dynamic_cast<RemoveDataTask&>(*t);

						try {
							ibrcommon::File destination = _path.get(remove.hash.value);
							{
								ibrcommon::MutexLock l(_global_mutex);
								if (!destination.exists())
								{
									throw DataNotAvailableException();
								}
								destination.remove();
							}
							_callback.eventDataStorageRemoved(remove.hash);
						} catch (const ibrcommon::Exception &ex) {
							_callback.eventDataStorageRemoveFailed(remove.hash, ex);
						}
					} catch (const std::bad_cast&) {

					}

					delete t;
				}
			} catch (const ibrcommon::QueueUnblockedException&) {
				// exit
			}
		}

		DataStorage::Container::~Container() {};
		DataStorage::Task::~Task() {};

		DataStorage::StoreDataTask::StoreDataTask(const Hash &h, Container *c)
		 : hash(h), container(c)
		{}

		DataStorage::StoreDataTask::~StoreDataTask()
		{
			delete container;
		}

		DataStorage::RemoveDataTask::RemoveDataTask(const Hash &h) : hash(h)
		{}

		DataStorage::RemoveDataTask::~RemoveDataTask()
		{
		}
	}
}

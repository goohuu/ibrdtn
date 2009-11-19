/*
 * BLOBManager.cpp
 *
 *  Created on: 28.05.2009
 *      Author: morgenro
 */

#include "ibrdtn/data/BLOBManager.h"
#include "ibrdtn/data/BLOBReference.h"
#include "ibrdtn/data/Exceptions.h"
#include "ibrdtn/utils/Mutex.h"
#include "ibrdtn/utils/MutexLock.h"
#include "ibrdtn/utils/Utils.h"

#include <functional>
#include <list>
#include <algorithm>

namespace dtn
{
	namespace blob
	{
		BLOBManager BLOBManager::_instance;

		BLOBManager::BLOB::BLOB(string key)
		 : _type(BLOB_MEMORY), _key(key), _refcount(0), _filename("null")
		{
			_sstream = new std::stringstream();
		}

		BLOBManager::BLOB::BLOB(string key, string filename, bool readonly)
		 : _type(BLOB_HARDDISK), _key(key), _refcount(0), _filename(filename)
		{
			if (readonly)
			{
				_type = BLOB_FILENAME;

				// open the file in read only mode
				_file = new fstream(filename.c_str(), ios::in|ios::binary);

				if (!_file->is_open())
				{
					throw exceptions::PermissionDeniedException("error while open file " + filename);
				}
			}
			else
			{
				// create new blob
				_file = new fstream(filename.c_str(), ios::in|ios::out|ios::binary|ios::trunc);

				if (!_file->is_open())
				{
					throw exceptions::PermissionDeniedException("error while open file " + filename);
				}
			}
		}

		BLOBManager::BLOB::~BLOB()
		{
			switch (_type)
			{
				case BLOB_FILENAME:
					delete _file;
					break;

				case BLOB_MEMORY:
					delete _sstream;
					break;

				case BLOB_HARDDISK:
					_file->close();
					::remove(_filename.c_str());
					delete _file;
					break;
			}
		}

		const string BLOBManager::BLOB::getKey() const
		{
			return _key;
		}

		istream& BLOBManager::BLOB::getInputStream() const
		{
			switch (_type)
			{
				case BLOB_FILENAME:
					return *_file;

				case BLOB_MEMORY:
					return *_sstream;

				case BLOB_HARDDISK:
					return *_file;
			}
		}

		ostream& BLOBManager::BLOB::getOutputStream() const
		{
			switch (_type)
			{
				case BLOB_FILENAME:
					throw dtn::exceptions::IOException("This is a read only stream.");

				case BLOB_MEMORY:
					return *_sstream;

				case BLOB_HARDDISK:
					return *_file;
			}
		}

		void BLOBManager::BLOB::increment()
		{
			dtn::utils::MutexLock l(_reflock);
			_refcount++;
		}

		void BLOBManager::BLOB::decrement()
		{
			dtn::utils::MutexLock l(_reflock);
			_refcount--;
		}

		bool BLOBManager::BLOB::isUnbound()
		{
			dtn::utils::MutexLock l(_reflock);
			if (_refcount == 0) return true;
			return false;
		}

		void BLOBManager::BLOB::clear()
		{
			switch (_type)
			{
			case BLOB_MEMORY:
				delete _sstream;
				_sstream = new stringstream();
				break;

			case BLOB_HARDDISK:
				_file->close();
				::remove(_filename.c_str());
				_file = new fstream(_filename.c_str(), ios::in|ios::binary);
				break;

			case BLOB_FILENAME:
				throw dtn::exceptions::IOException("This is a read only stream.");
				break;
			}
		}

		size_t BLOBManager::BLOB::getSize()
		{
			switch (_type)
			{
				case BLOB_FILENAME:
				case BLOB_HARDDISK:
					_file->seekg(0, ios_base::end);
					return _file->tellg();

				case BLOB_MEMORY:
					_sstream->seekg(0, ios_base::end);
					return _sstream->tellg();
			}
		}

		BLOBManager::BLOBManager()
		 : _memoryonly(true), _lastid(0)
		{ }

		BLOBManager::BLOBManager(string directory)
		 : _directory(directory), _memoryonly(false), _lastid(0)
		{ }

		BLOBManager::~BLOBManager()
		{ }

		void BLOBManager::remove(BLOB *b)
		{
			dtn::utils::MutexLock l(_blob_lock);
			_blobs.erase(b->getKey());
		}

		void BLOBManager::add(BLOB *b)
		{
			dtn::utils::MutexLock l(_blob_lock);
			_blobs[b->getKey()] = b;
		}

		void BLOBManager::init(string directory)
		{
			dtn::blob::BLOBManager::_instance = dtn::blob::BLOBManager("/tmp");
		}

		BLOBManager::BLOB* BLOBManager::getBLOB(const BLOBReference &ref) const
		{
			map<string, BLOB*>::const_iterator iter = _blobs.find( ref._key );
			if (iter == _blobs.end()) throw exceptions::Exception("ID not exists!");
			return (*iter).second;
		}

		BLOBReference BLOBManager::create(string filename)
		{
			string key = getFreeKey();
			BLOBManager::BLOB *b = new BLOBManager::BLOB(key, filename, true);
			add(b);

			return BLOBReference(this, key);
		}

		BLOBReference BLOBManager::create(BLOBManager::BLOB_TYPE type)
		{
			string key = getFreeKey();

			if (_memoryonly) type = BLOB_MEMORY;

			BLOBManager::BLOB *b = NULL;

			switch (type)
			{
				case BLOB_MEMORY:
					b = new BLOBManager::BLOB(key);
				break;

				case BLOB_HARDDISK:
				{
					// translate into a filename
					stringstream ss;
					ss << _directory << "/" << key << ".dtn" << endl;
					string filename;
					ss >> filename;

					b = new BLOBManager::BLOB(key, filename);
				}
				break;

				case BLOB_FILENAME:
					throw dtn::exceptions::NotImplementedException("This type can not be created with this function.");
			}

			add(b);

			return BLOBReference(this, key);
		}

		void BLOBManager::linkReference(BLOBReference &ref)
		{
			getBLOB(ref)->increment();
		}

		void BLOBManager::unlinkReference(BLOBReference &ref)
		{
			BLOBManager::BLOB *b = getBLOB(ref);

			b->decrement();

			if (b->isUnbound())
			{
				remove(b);
				delete b;
			}
		}

		string BLOBManager::getFreeKey()
		{
			dtn::utils::MutexLock l(_id_lock);

			_lastid++;

			stringstream ss;
			ss << "blob-" << dtn::utils::Utils::get_current_dtn_time() << "-" << _lastid;
			string key; ss >> key;

			return key;
		}

		size_t BLOBManager::append(BLOBReference &destination, BLOBReference &source)
		{
			return copyData( source, 0, destination, destination.getSize(), source.getSize() );
		}

		size_t BLOBManager::append(BLOBReference &ref, const char* data, size_t size)
		{
			ostream &stream = getBLOB(ref)->getOutputStream();

			stream.seekp( 0, ios::end );
			stream.write(data, size);

			return size;
		}

		void BLOBManager::clear(BLOBReference &ref)
		{
			getBLOB(ref)->clear();
		}

		size_t BLOBManager::read(BLOBReference &ref, char *data, size_t offset, size_t size) const
		{
			istream &stream = getBLOB(ref)->getInputStream();
			stream.seekg( offset );
			size_t ret = stream.readsome( data, size );
			return ret;
		}

		size_t BLOBManager::read(BLOBReference &ref, ostream &stream) const
		{
			istream &source = getBLOB(ref)->getInputStream();

			// seek to the end
			source.seekg( 0, ios::end );
			size_t len = source.tellg();

			// jump to the begin
			source.seekg(0);

			// write data from stream to stream
			char data[128];
			size_t size = len;

			while (size > 0)
			{
				if (size < 128)
				{
					source.read(data, size);
					stream.write(data, size);
					size = 0;
				}
				else
				{
					source.read(data, 128);
					stream.write(data, 128);
					size -= 128;
				}
			}

			return len;
		}

		size_t BLOBManager::write(BLOBReference &ref, const char *data, size_t offset, size_t size)
		{
			ostream &s = getBLOB(ref)->getOutputStream();
			s.seekp(offset);
			s.write(data, size);

			return size;
		}

		size_t BLOBManager::write(BLOBReference &ref, size_t offset, istream &stream)
		{
			dtn::utils::MutexLock l(ref);

			//char buffer[512];
			char bytes = 1;
			size_t ret = 0;
			ostream &s = getBLOB(ref)->getOutputStream();

			while (!stream.eof())
			{
				char buf;
				stream.get(buf);
				s.put(buf);
				ret++;
			}

//			while (bytes > 0)
//			{
//				bytes = stream.readsome(buffer, 512);
//				s.write(buffer, bytes);
//				ret += bytes;
//			}

			return ret;
		}

		size_t BLOBManager::getSize(const BLOBReference &ref) const
		{
			return getBLOB(ref)->getSize();
		}

		size_t BLOBManager::copyData(BLOBReference &source, size_t src_offset, BLOBReference &destination, size_t dst_offset, size_t length)
		{
			dtn::utils::MutexLock l1(source);
			dtn::utils::MutexLock l2(destination);

			char buffer[512];

			istream &file0 = getBLOB(source)->getInputStream();
			ostream &file1 = getBLOB(destination)->getOutputStream();

			file0.seekg( src_offset );
			file1.seekp( dst_offset );

			// copy 0 to split_position bytes of ref to ref1
			size_t i = 0;
			size_t step = length;
			size_t rsize = step;

			if (step > 512) step = 512;

			for (i = 0; i < length; i += step)
			{
				rsize = file0.readsome(buffer, step);
				file1.write(buffer, rsize);

				if (rsize < step) break;
			}

			file1.flush();

			return length;
		}

		pair<BLOBReference, BLOBReference> BLOBManager::split(BLOBReference ref, size_t split_position)
		{
			// get size of ref
			size_t size = ref.getSize();

			// create a new blob
			BLOBReference ref1 = create(BLOB_HARDDISK);

			// copy 0 to split_position bytes of ref to ref1
			copyData(ref, 0, ref1, 0, split_position);

			// create a new blob
			BLOBReference ref2 = create();

			// remaining data
			if (size > split_position)
			{
				// copy split_position to end bytes of ref to ref2
				copyData(ref, split_position, ref2, 0, size - split_position);
			}

			return make_pair(ref1, ref2);
		}
	}
}

/*
 * BLOBManager.h
 *
 *  Created on: 28.05.2009
 *      Author: morgenro
 */

#ifndef BLOBMANAGER_H_
#define BLOBMANAGER_H_

#include "ibrdtn/default.h"

namespace dtn
{
	namespace blob
	{
		class BLOBReference;

		class BLOBManager
		{
			friend class BLOBReference;

		public:
			enum BLOB_TYPE
			{
				BLOB_MEMORY = 0,
				BLOB_HARDDISK = 1,
				BLOB_FILENAME = 2
			};

		private:
			class BLOB
			{
			public:
				BLOB(string key); // creates a memory based BLOB
				BLOB(string key, string filename, bool readonly = false); // creates a file bases BLOB
				~BLOB();

				const string getKey() const;

				istream &getInputStream() const;
				ostream &getOutputStream() const;

				void increment();
				void decrement();
				bool isUnbound();

				void clear();
				size_t getSize();

			private:
				BLOB_TYPE _type;
				string _key;
				fstream *_file;
				stringstream *_sstream;
				size_t _refcount;
				dtn::utils::Mutex _reflock;
				string _filename;
			};

		public:
			virtual ~BLOBManager();

			BLOBReference create(BLOB_TYPE = BLOB_HARDDISK);
			BLOBReference create(string filename);

			static BLOBManager _instance;
			static void init(string directory);

		protected:
			BLOBManager();
			BLOBManager(string directory);

			size_t copyData(BLOBReference &source, size_t src_offset, BLOBReference &destination, size_t dst_offset, size_t length);
			pair<BLOBReference, BLOBReference> split(const BLOBReference ref, size_t split_position);

			void linkReference(BLOBReference &ref);
			void unlinkReference(BLOBReference &ref);

			//size_t pullId();

			size_t read(BLOBReference &ref, char *data, size_t offset, size_t size) const;
			size_t read(BLOBReference &ref, ostream &stream) const;

			size_t write(BLOBReference &ref, const char *data, size_t offset, size_t size);
			size_t write(BLOBReference &ref, size_t offset, istream &stream);

			size_t append(BLOBReference &destination, BLOBReference &source);
			size_t append(BLOBReference &ref, const char* data, size_t size);

			void clear(BLOBReference &ref);

			size_t getSize(const BLOBReference &ref) const;

		private:
			BLOB* getBLOB(const BLOBReference &ref) const;
			void add(BLOB *b);
			void remove(BLOB *b);

			void create(BLOBManager::BLOB_TYPE type, size_t id);
			void create(string filename, size_t id);

			map<string, BLOB*> _blobs;
			dtn::utils::Mutex _blob_lock;

			string getFreeKey();

			string _directory;

			bool _memoryonly;

			size_t _lastid;
			dtn::utils::Mutex _id_lock;
		};
	}
}

#endif /* BLOBMANAGER_H_ */

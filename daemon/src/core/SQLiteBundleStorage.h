/*
 * SQLiteBundleStorage.h
 *
 *  Created on: 09.01.2010
 *      Author: Myrtus
 */


#ifndef SQLITEBUNDLESTORAGE_H_
#define SQLITEBUNDLESTORAGE_H_

#include "BundleStorage.h"
#include "Component.h"
#include "EventReceiver.h"
#include "core/BundleStorage.h"
#include "core/EventReceiver.h"
#include <ibrdtn/data/MetaBundle.h>

#include <ibrcommon/thread/Thread.h>
#include <ibrcommon/thread/Conditional.h>
#include <ibrcommon/data/File.h>
#include <ibrcommon/thread/Queue.h>

#include <sqlite3.h>
#include <string>
#include <list>
#include <set>

//#define SQLITE_STORAGE_EXTENDED 1

class SQLiteBundleStorageTest;

namespace dtn
{
	namespace core
	{
		class SQLiteBundleStorage: public BundleStorage, public EventReceiver, public dtn::daemon::IndependentComponent, public ibrcommon::MutexInterface
		{
			enum SQL_TABLES
			{
				SQL_TABLE_BUNDLE = 0,
				SQL_TABLE_BLOCK = 1,
				SQL_TABLE_ROUTING = 2,
				SQL_TABLE_BUNDLE_ROUTING_INFO = 3,
				SQL_TABLE_NODE_ROUTING_INFO = 4,
				SQL_TABLE_END = 5
			};

			// enum of all possible statements
			enum STORAGE_STMT
			{
				BUNDLE_GET_FILTER,
				BUNDLE_GET_ID,
				FRAGMENT_GET_ID,
				BUNDLE_GET_FRAGMENT,

				EXPIRE_BUNDLES,
				EXPIRE_BUNDLE_FILENAMES,
				EXPIRE_BUNDLE_DELETE,
				EXPIRE_NEXT_TIMESTAMP,

				EMPTY_CHECK,
				COUNT_ENTRIES,

				BUNDLE_DELETE,
				FRAGMENT_DELETE,
				BUNDLE_CLEAR,
				BUNDLE_STORE,

				PROCFLAGS_SET,

				BLOCK_GET_ID,
				BLOCK_GET_ID_FRAGMENT,
				BLOCK_GET,
				BLOCK_GET_FRAGMENT,
				BLOCK_CLEAR,
				BLOCK_STORE,

#ifdef SQLITE_STORAGE_EXTENDED
				ROUTING_GET,
				ROUTING_REMOVE,
				ROUTING_CLEAR,
				ROUTING_STORE,

				NODE_GET,
				NODE_REMOVE,
				NODE_CLEAR,
				NODE_STORE,

				INFO_GET,
				INFO_REMOVE,
				INFO_STORE,
#endif

				VACUUM,
				SQL_QUERIES_END
			};

			static const std::string _tables[SQL_TABLE_END];

			// array of sql queries
			static const std::string _sql_queries[SQL_QUERIES_END];

			// array of the db structure as sql
			static const std::string _db_structure[9];

		public:
			friend class ::SQLiteBundleStorageTest;

			class SQLBundleQuery
			{
			public:
				SQLBundleQuery();
				virtual ~SQLBundleQuery() = 0;

				/**
				 * returns the user-defined sql query
				 * @return
				 */
				virtual const std::string getWhere() const = 0;

				/**
				 * bind all custom values to the statement
				 * @param st
				 * @param offset
				 * @return
				 */
				virtual size_t bind(sqlite3_stmt*, size_t offset) const
				{
					return offset;
				}

				/**
				 * contains the compiled statement of this query
				 */
				sqlite3_stmt *_statement;
			};

			class SQLiteQueryException : public ibrcommon::Exception
			{
			public:
				SQLiteQueryException(string what = "Unable to execute Querry.") throw(): Exception(what)
				{
				}
			};

			/**
			 * Constructor
			 * @param Pfad zum Ordner in denen die Datein gespeichert werden.
			 * @param Dateiname der Datenbank
			 * @param maximale Größe der Datenbank
			 */
			SQLiteBundleStorage(const ibrcommon::File &path, const size_t &size);

			/**
			 * destructor
			 */
			virtual ~SQLiteBundleStorage();

			/**
			 * open the database
			 */
			void openDatabase(const ibrcommon::File &path);

			/**
			 * Stores a bundle in the storage.
			 * @param bundle The bundle to store.
			 */
			void store(const dtn::data::Bundle &bundle);

			/**
			 * This method returns a specific bundle which is identified by
			 * its id.
			 * @param id The ID of the bundle to return.
			 * @return A bundle object.
			 */
			dtn::data::Bundle get(const dtn::data::BundleID &id);

			/**
			 * @see BundleStorage::get(BundleFilterCallback &cb)
			 */
			const std::list<dtn::data::MetaBundle> get(BundleFilterCallback &cb);

#ifdef SQLITE_STORAGE_EXTENDED
			/**
			 * store routinginformation referring to a Bundle
			 * @param ID of the Bundle to which the routinginformation is referring
			 * @param string containing the routinginformation
			 */
			void storeBundleRoutingInfo(const data::BundleID &BundleID, const int &key, const string &routingInfo);

			/**
			 * store routinginformation reffering to a Node.
			 * @param EID of the Node
			 * @param A string routinginformation
			 */
			void storeNodeRoutingInfo(const data::EID &nodel, const int &key, const std::string &routingInfo);

			/**
			 * store Routinginformation
			 * @param Key which refers to the Routinginformation
			 * @param string containing Routinginformation
			 */
			void storeRoutingInfo(const int &key, const std::string &routingInfo);

			/**
			 * Returns the routinginformation stored for a specific Bundle.
			 * @param ID to the Bundle
			 */
			std::string getBundleRoutingInfo(const data::BundleID &bundleID, const int &key);

			/**
			 * Returns the routinginformation stored for a specific Node.
			 * @param ID to the Bundle
			 * @return string containing Routinginformation for a specific Node
			 */
			std::string getNodeRoutingInfo(const data::EID &eid, const int &key);

			/**
			 * Gets Routinginformation storen refered by Key.
			 * @param the key referring to der routinginformation
			 * @return string containing Routinginformation
			 */
			std::string getRoutingInfo(const int &key);

			/**
			 * Deletes the Routinginformation of a Bundle
			 * @param BundleID
			 */
			void removeBundleRoutingInfo(const data::BundleID &bundleID, const int &key);

			/**
			 * Deletes Routinginformation of a node
			 * @param The EID of the Node
			 */
			void removeNodeRoutingInfo(const data::EID &eid, const int &key);

			/**
			 * deletes a Routinginformation specified by a key
			 * @param key specifieing the information to be deleted
			 */
			void removeRoutingInfo(const int &key);
#endif

			/**
			 * This method deletes a specific bundle in the storage.
			 * No reports will be generated here.
			 * @param id The ID of the bundle to remove.
			 */
			void remove(const dtn::data::BundleID &id);

			/**
			 * Clears all bundles and fragments in the storage. Routinginformation won't be deleted.
			 */
			void clear();

			/**
			 * Clears the hole database.
			 */
			void clearAll();

			/**
			 * @return True, if no bundles in the storage.
			 */
			bool empty();

			/**
			 * @return the count of bundles in the storage
			 */
			unsigned int count();

			/**
			 * @sa BundleStorage::releaseCustody();
			 */
			void releaseCustody(dtn::data::BundleID &bundle);

#ifdef SQLITE_STORAGE_EXTENDED
			/**
			 * Sets the priority of the Bundle.
			 * @param The Priority Value between 0 and 3.
			 * @param BundleId where the priority should be set.
			 */
			void setPriority(const int priority,const dtn::data::BundleID &id);

			/**
			 * Gets the Block by its type of a specific Bundle
			 * @param BundleID
			 * @param Blocktype
			 * @return A list containing the resulting Blocks
			 */
			list<data::Block> getBlock(const data::BundleID &bundleID,const char &blocktype);

			/**
			 * Returns the ammount of occupied storage space
			 * @return occupied storage space or -1 indicating an error
			 */
			int occupiedSpace();
#endif

			/**
			 * This method is used to receive events.
			 * @param evt
			 */
			void raiseEvent(const Event *evt);

			/**
			 * Remove all bundles which match this filter
			 * @param filter
			 */
			dtn::data::MetaBundle remove(const ibrcommon::BloomFilter&) { return dtn::data::MetaBundle(); };

			/**
			 * Try to lock everything in the database.
			 */
			virtual void trylock() throw (ibrcommon::MutexException);

			/**
			 * Locks everything in the database.
			 */
			virtual void enter() throw (ibrcommon::MutexException);

			/**
			 * Unlocks everything in the database.
			 */
			virtual void leave() throw (ibrcommon::MutexException);

		protected:
			virtual void componentRun();
			virtual void componentUp();
			virtual void componentDown();
			bool __cancellation();

		private:
			enum Position
			{
				FIRST_FRAGMENT 	= 0,
				LAST_FRAGMENT 	= 1,
				BOTH_FRAGMENTS 	= 2
			};

			class AutoResetLock
			{
			public:
				AutoResetLock(ibrcommon::Mutex &mutex, sqlite3_stmt *st);
				~AutoResetLock();

			private:
				ibrcommon::MutexLock _lock;
				sqlite3_stmt *_st;
			};

			class Task
			{
			public:
				virtual ~Task() {};
				virtual void run(SQLiteBundleStorage &storage) = 0;
			};

			class BlockingTask : public Task
			{
			public:
				BlockingTask() : _done(false), _abort(false) {};
				virtual ~BlockingTask() {};
				virtual void run(SQLiteBundleStorage &storage) = 0;

				/**
				 * wait until this job is done
				 */
				void wait()
				{
					ibrcommon::MutexLock l(_cond);
					while (!_done && !_abort) _cond.wait();

					if (_abort) throw ibrcommon::Exception("Task aborted");
				}

				void abort()
				{
					ibrcommon::MutexLock l(_cond);
					_abort = true;
					_cond.signal(true);
				}

				void done()
				{
					ibrcommon::MutexLock l(_cond);
					_done = true;
					_cond.signal(true);
				}

			private:
				ibrcommon::Conditional _cond;
				bool _done;
				bool _abort;
			};

			class TaskRemove : public Task
			{
			public:
				TaskRemove(const dtn::data::BundleID &id)
				 : _id(id) { };

				virtual ~TaskRemove() {};
				virtual void run(SQLiteBundleStorage &storage);

			private:
				const dtn::data::BundleID _id;
			};

			class TaskIdle : public Task
			{
			public:
				TaskIdle() { };

				virtual ~TaskIdle() {};
				virtual void run(SQLiteBundleStorage &storage);

				static ibrcommon::Mutex _mutex;
				static bool _idle;
			};

			class TaskExpire : public Task
			{
			public:
				TaskExpire(size_t timestamp)
				: _timestamp(timestamp) { };

				virtual ~TaskExpire() {};
				virtual void run(SQLiteBundleStorage &storage);

			private:
				size_t _timestamp;
			};

			/**
			 * Retrieve the meta data of a given bundle
			 * @param id
			 * @return
			 */
			dtn::data::MetaBundle get_meta(const dtn::data::BundleID &id);

			void get(sqlite3_stmt *st, dtn::data::MetaBundle &bundle, size_t offset = 0);
			void get(sqlite3_stmt *st, dtn::data::Bundle &bundle, size_t offset = 0);

#ifdef SQLITE_STORAGE_EXTENDED
			/**
			 * This Function stores Fragments.
			 * @param bundle The bundle to store.
			 */
			void storeFragment(const dtn::data::Bundle &bundle);
#endif

			/**
			 * Takes a string and a SQLstatement object an creates the Object
			 * @param string containing the SQLquarry
			 * @param sqlite statement which should be prepared
			 */
			sqlite3_stmt* prepare(const std::string &sqlquery);

			/**
			 *  This Funktion gets e list and a bundle. Every block of the bundle except the PrimaryBlock is saved in a File.
			 *  The filenames of the blocks are stored in the List. The order of the filenames matches the order of the blocks.
			 *  @param An empty Stringlist
			 *  @param A Bundle which should be prepared to be Stored.
			 *  @return A number bigges than zero is returned indicating an error. Zero is returned if no error occurred.
			 */
			int prepareBundle(list<std::string> &filenames, dtn::data::Bundle &bundle);

			/**
			 * Saves the Blocks of a given Bundle to Disk and stores metadata into the database. The caller of this function has to have the Lock for the database.
			 * @param Bundle which blocks should be saved
			 * @return The total number of bytes which were stored or -1 indicating an error.
			 */
			int store_blocks(const data::Bundle &Bundle);

			/**
			 * Reads the Blocks from the belonging to the ID and adds them to the bundle. The caller of this function has to have the Lock for the database.
			 * @param Bundle where the Blocks should be added
			 * @param The BundleID for which the Blocks should be read
			 */
			void get_blocks(dtn::data::Bundle &bundle, const dtn::data::BundleID &id);

			/**
			 * Checks the files on the filesystem against the filenames in the database
			 */
			void check_consistency();

			void check_fragments(set<string> &payloadFiles);

			void check_bundles(set<string> &blockFiles);

			/**
			 * updates the nextExpiredTime. The calling function has to have the databaselock.
			 */
			void update_expire_time();

			/**
			 * lower the next expire time if the ttl is lower than the current expire time
			 * @param ttl
			 */
			void new_expire_time(size_t ttl);
			void reset_expire_time();
			size_t get_expire_time();

			void set_bundleid(sqlite3_stmt *st, const dtn::data::BundleID &id, size_t offset = 0) const;
			void get_bundleid(sqlite3_stmt *st, dtn::data::BundleID &id, size_t offset = 0) const;

			/**
			 * @see Component::getName()
			 */
			virtual const std::string getName() const;

			ibrcommon::File dbPath;
			ibrcommon::File dbFile;
			ibrcommon::File _blockPath;

			int dbSize;

			// holds the database handle
			sqlite3 *_database;

			// contains all jobs to do
			ibrcommon::Queue<Task*> _tasks;

			ibrcommon::Mutex _next_expiration_lock;
			size_t _next_expiration;

			// array of statements
			sqlite3_stmt* _statements[SQL_QUERIES_END];

			// array of locks for each statement
			ibrcommon::Mutex _locks[SQL_QUERIES_END];

			void add_deletion(const dtn::data::BundleID &id);
			void remove_deletion(const dtn::data::BundleID &id);
			bool contains_deletion(const dtn::data::BundleID &id);

			// set of bundles to delete
			ibrcommon::Mutex _deletion_mutex;
			std::set<dtn::data::BundleID> _deletion_list;
		};
	}
}

#endif /* SQLITEBUNDLESTORAGE_H_ */

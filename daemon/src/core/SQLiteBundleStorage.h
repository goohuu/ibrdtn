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
#include <ibrcommon/thread/Thread.h>
#include <ibrcommon/thread/Conditional.h>
#include <ibrcommon/data/File.h>

#include <sqlite3.h>
#include <string>
#include <list>
#include <set>

class SQLiteBundleStorageTest;

namespace dtn
{
	namespace core
	{
		class SQLiteBundleStorage: public BundleStorage, public EventReceiver, public dtn::daemon::IndependentComponent
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
				BUNDLE_GET_TTL,
				BUNDLE_GET_DESTINATION,
				BUNDLE_GET_ID,
				BUNDLE_GET,
				BUNDLE_GET_FRAGMENT,
				SQL_GET_ROWID,
				PROCFLAGS_GET,
				TTL_GET_NEXT_EXPIRED,
				BUNDLE_GET_SORT_SIZE,
				BUNDLE_GET_BETWEEN_SIZE,
				BUNDLE_GET_SOURCE,
				BUNDLE_GET_SORT_TTL,
				COUNT_ENTRIES,
				BUNDLE_DELETE_TTL,
				BUNDLE_DELETE,
				BUNDLE_REMOVE,
				BUNDLE_CLEAR,
				BUNDLE_STORE,
				PROCFLAGS_SET,
				BLOCK_GET_ID,
				BLOCK_GET,
				BLOCK_CLEAR,
				BLOCK_STORE,
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
				VACUUM,
				SQL_QUERIES_END
			};

			static const std::string _tables[SQL_TABLE_END];

			// array of sql queries
			static const std::string _sql_queries[SQL_QUERIES_END];

			// array of the db structure as sql
			static const std::string _db_structure[11];

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
			 * If no bundle can be found, this exception is thrown.
			 */
			class NoBundleFoundException : public ibrcommon::Exception
			{
				public:
					NoBundleFoundException(string what = "No bundle available.") throw() : Exception(what)
					{
					};
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
			 * This method returns a specific bundle which is identified by
			 * its id.
			 * @param id The ID of the bundle to return.
			 * @return A bundle object.
			 */
			dtn::data::Bundle get(const dtn::data::BundleID &id);

			/**
			 * @see BundleStorage::get(const BundleFilterCallback &cb)
			 */
			const std::list<dtn::data::MetaBundle> get(const BundleFilterCallback &cb);

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

			/**
			 * Sets the priority of the Bundle.
			 * @param The Priority Value between 0 and 3.
			 * @param BundleId where the priority should be set.
			 */
			void setPriority(const int priority,const dtn::data::BundleID &id);

//			/**
//			 * returns the first 'limit' biggest Bundles
//			 * @param number of bundles to return
//			 * @return a list containing the resulting bundles
//			 */
//			list<data::Bundle> getBundleBySize(const int &limit);
//
//			/**
//			 *  returns the first 'limit' bundles with the lowest expiring time
//			 * @param number of bundles to return
//			 * @return a list containing the resulting bundles
//			 */
//			list<data::Bundle> getBundleByTTL (const int &limit);
//
//			/**
//			 * Gets a list of Bundles which all have the specifierd SourceEID.
//			 * @param SourceEID
//			 * @return list containing bundles with SourceEID equals to the parameter.
//			 */
//			list<data::Bundle> getBundlesBySource(const dtn::data::EID &sourceID);
//
//			/**
//			 * Gets a list of Bundles which all have the specifierd DestinationEID.
//			 * @param SourceEID
//			 * @return list containing bundles with DestinationEID equals to the parameter.
//			 */
//			list<data::Bundle> getBundlesByDestination(const dtn::data::EID &sourceID);

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

			/**
			 * This method is used to receive events.
			 * @param evt
			 */
			void raiseEvent(const Event *evt);

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

			/**
			 * Retrieve the meta data of a given bundle
			 * @param id
			 * @return
			 */
			dtn::data::MetaBundle get_meta(const dtn::data::BundleID &id);

			void get(sqlite3_stmt *st, dtn::data::MetaBundle &bundle, size_t offset = 0);
			void get(sqlite3_stmt *st, dtn::data::Bundle &bundle, size_t offset = 0);


			void executeQuery(sqlite3_stmt *statement);

			void executeQuery(sqlite3_stmt *statement, list<string> &result);

			void deleteFiles(list<std::string> &fileList);

			/**
			 * This Function stores Fragments.
			 * @param bundle The bundle to store.
			 */
			void storeFragment(const dtn::data::Bundle &bundle);

			/**
			 * Takes a string and a SQLstatement object an creates the Object
			 * @param string containing the SQLquarry
			 * @param sqlite statement which should be prepared
			 */
			sqlite3_stmt* prepareStatement(const std::string &sqlquery);

			/**
			 * Is called when a new timeevent in raised. It seach for bundles with expired lifetimes.
			 * @param actual time
			 * @return
			 */
			void deleteexpired();

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
			int storeBlocks(const data::Bundle &Bundle);

			/**
			 * Remove all bundles which match this filter
			 * @param filter
			 */
			dtn::data::MetaBundle remove(const ibrcommon::BloomFilter&) { return dtn::data::MetaBundle(); };

			/**
			 * Reads the Blocks from the belonging to the ID and adds them to the bundle. The caller of this function has to have the Lock for the database.
			 * @param Bundle where the Blocks should be added
			 * @param The BundleID for which the Blocks should be read
			 */
			void get_blocks(dtn::data::Bundle &bundle, const std::string &bundleID);

			/**
			 * Contains procedures which should be done only during idle.
			 */
			void idle_work();


			/**
			 * Checks the files on the filesystem against the filenames in the database
			 */
			void consistenceCheck();

			void checkFragments(set<string> &payloadFiles);

			void checkBundles(set<string> &blockFiles);

			/**
			 * updates the nextExpiredTime. The calling function has to have the databaselock.
			 */
			void updateexpiredTime();

			const std::string generate_bundle_key(const dtn::data::BundleID &id) const;

			/**
			 * lower the next expire time if the ttl is lower than the current expire time
			 * @param ttl
			 */
			void new_expire_time(size_t ttl);

			/**
			 * @see Component::getName()
			 */
			virtual const std::string getName() const;

			bool global_shutdown;

			ibrcommon::File dbPath;
			ibrcommon::File dbFile;
			ibrcommon::File _blockPath;

			int dbSize;
			sqlite3 *database;
			ibrcommon::Conditional dbMutex;
			ibrcommon::Conditional timeeventConditional;

			ibrcommon::Mutex time_change;// idle_mutex;

			// local Variable which saves the timestamp from incoming Timeevents
			size_t actual_time;
			size_t nextExpiredTime;
			bool idle;

			// list containing filenames (inclusive path) which could be deleted
			std::list<std::string> deleteList;

			// array of statements
			sqlite3_stmt* _statements[SQL_QUERIES_END];
		};
	}
}

#endif /* SQLITEBUNDLESTORAGE_H_ */

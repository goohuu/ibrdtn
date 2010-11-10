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


namespace dtn {
namespace testsuite{
	class SQLiteBundleStorageTestSuite;
}

namespace core {

class SQLiteBundleStorage: public BundleStorage, public EventReceiver, public ibrcommon::JoinableThread
{
	enum SQL_TABLES
	{
		SQL_TABLE_BUNDLE = 0,
		SQL_TABLE_FRAGMENT = 1,
		SQL_TABLE_BLOCK = 2,
		SQL_TABLE_ROUTING = 3,
		SQL_TABLE_BUNDLE_ROUTING_INFO = 4,
		SQL_TABLE_NODE_ROUTING_INFO = 5,
		SQL_TABLE_END = 6
	};

	static const std::string _tables[SQL_TABLE_END];

public:
	friend class testsuite::SQLiteBundleStorageTestSuite;

	class SQLiteQuerryException : public ibrcommon::Exception
	{
	public:
		SQLiteQuerryException(string what = "Unable to execute Querry.") throw(): Exception(what)
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
	SQLiteBundleStorage(const ibrcommon::File &dbPath, const string &dbFile, const int &size);

	/**
	 * destructor
	 */
	virtual ~SQLiteBundleStorage();

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
	 * Query for a bundle with a specific destination. Set exact to true, if the application
	 * part should be compared too.
	 * @param eid
	 * @param exact
	 * @return
	 */
	const dtn::data::MetaBundle getByDestination(const dtn::data::EID &eid, bool exact = false);

	/**
	 * Returns a bundle ID which is not in the bloomfilter, but in the storage
	 * @param filter
	 * @return
	 */
	const dtn::data::MetaBundle getByFilter(const ibrcommon::BloomFilter &filter);

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

	/**
	 * returns the first 'limit' biggest Bundles
	 * @param number of bundles to return
	 * @return a list containing the resulting bundles
	 */
	list<data::Bundle> getBundleBySize(const int &limit);

	/**
	 *  returns the first 'limit' bundles with the lowest expiring time
	 * @param number of bundles to return
	 * @return a list containing the resulting bundles
	 */
	list<data::Bundle> getBundleByTTL (const int &limit);

	/**
	 * Gets a list of Bundles which all have the specifierd SourceEID.
	 * @param SourceEID
	 * @return list containing bundles with SourceEID equals to the parameter.
	 */
	list<data::Bundle> getBundlesBySource(const dtn::data::EID &sourceID);

	/**
	 * Gets a list of Bundles which all have the specifierd DestinationEID.
	 * @param SourceEID
	 * @return list containing bundles with DestinationEID equals to the parameter.
	 */
	list<data::Bundle> getBundlesByDestination(const dtn::data::EID &sourceID);

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
	void run(void);
	bool __cancellation();

private:
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
	sqlite3_stmt* prepareStatement(string sqlquery);

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
	int storeBlocks(const data::Bundle &bundle);

	/**
	 * Remove all bundles which match this filter
	 * @param filter
	 */
	dtn::data::MetaBundle remove(const ibrcommon::BloomFilter&) { return dtn::data::MetaBundle(); };

	/**
	 * Reads the Blocks from the belonging to the ID and adds them to the bundle. The caller of this function has to have the Lock for the database.
	 * @param Bundle where the Blocks should be added
	 * @param The BundleID for which the Blocks should be read
	 * @return A number bigges than zero is returned indicating an error. Zero is returned if no error occurred.
	 */
	int readBlocks(data::Bundle &bundle, const string &bundleID);

	/**
	 * Contains procedures which should be done only during idle.
	 */
	void idle_work();


	/**
	 * Checks the files on the filesystem against the filenames in the database
	 */
	void consistenceCheck();

	/**
	 * updates the nextExpiredTime. The calling function has to have the databaselock.
	 */
	void updateexpiredTime();

	/**
	 * @see Component::getName()
	 */
	virtual const std::string getName() const;

	bool global_shutdown;
	string dbPath, dbFile;
	int dbSize;
	sqlite3 *database;
	ibrcommon::Conditional dbMutex, timeeventConditional;

	ibrcommon::Mutex time_change;// idle_mutex;

	//local Variable which saves the timestamp from incoming Timeevents
	size_t actual_time;
	size_t nextExpiredTime;
	bool idle;

	//list containing filenames (inclusive path) which could be deleted
	list<string> deleteList;

	sqlite3_stmt *getBundleTTL;
	sqlite3_stmt *getFragmentTTL;
	sqlite3_stmt *deleteBundleTTL;
	sqlite3_stmt *deleteFragementTTL;
	sqlite3_stmt *getBundleByDestination;
	sqlite3_stmt *getBundleByID;
	sqlite3_stmt *getFragements;
	sqlite3_stmt *store_Bundle;
	sqlite3_stmt *store_Fragment;
	sqlite3_stmt *clearBundles;
	sqlite3_stmt *clearFragments;
	sqlite3_stmt *clearBlocks;
	sqlite3_stmt *clearRouting;
	sqlite3_stmt *clearNodeRouting;
	sqlite3_stmt *countEntries;
	sqlite3_stmt *vacuum;
	sqlite3_stmt *getROWID;
	sqlite3_stmt *removeBundle;
	sqlite3_stmt *removeFragments;
	sqlite3_stmt *getBlocksByID;
	sqlite3_stmt *storeBlock;
	sqlite3_stmt *getBundlesBySize;
	sqlite3_stmt *getBundleBySource;
	sqlite3_stmt *getBundlesByTTL;
	sqlite3_stmt *getBlocks;
	sqlite3_stmt *getProcFlags;
	sqlite3_stmt *updateProcFlags;
	sqlite3_stmt *storeBundleRouting;
	sqlite3_stmt *getBundleRouting;
	sqlite3_stmt *removeBundleRouting;
	sqlite3_stmt *storeRouting;
	sqlite3_stmt *getRouting;
	sqlite3_stmt *removeRouting;
	sqlite3_stmt *storeNodeRouting;
	sqlite3_stmt *getNodeRouting;
	sqlite3_stmt *removeNodeRouting;
	sqlite3_stmt *getnextExpiredBundle;
	sqlite3_stmt *getnextExpiredFragment;

};

}
}

#endif /* SQLITEBUNDLESTORAGE_H_ */

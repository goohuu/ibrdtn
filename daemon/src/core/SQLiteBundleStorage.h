/*
 * SQLiteBundleStorage.h
 *
 *  Created on: 09.01.2010
 *      Author: Myrtus
 *
 *	Potentielle Erweiterungen:
 *		1) Zweite Tabelle für die Fragmente
 *		2)
 */

#ifndef SQLITEBUNDLESTORAGE_H_
#define SQLITEBUNDLESTORAGE_H_

#include "BundleStorage.h"
#include <sqlite3.h>
#include <string>
#include "EventReceiver.h"
#include "ibrcommon/thread/Thread.h"
#include "ibrcommon/data/File.h"
#include <set>
#include <vector>


namespace dtn {
namespace core {

class SQLiteBundleStorage: public BundleStorage, public EventReceiver, public ibrcommon::JoinableThread{
public:

	/**
	 * Constructor
	 * @param Pfad zum Ordner in denen die Datein gespeichert werden.
	 * @param Dateiname der Datenbank
	 * @param maximale Größe der Datenbank
	 */
	SQLiteBundleStorage(ibrcommon::File dbPath ,string dbFile , int size);

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
	 * This method returns a specific bundle which is identified by
	 * its id.
	 * @param id The ID of the bundle to return.
	 * @return A bundle object.
	 */
	dtn::data::Bundle get(const dtn::data::BundleID &id);

	/**
	 * This method returns a bundle which is addressed to a EID.
	 * If there is no bundle available the method will block until
	 * a global shutdown or a bundle is available.
	 * @param eid The receiver for the bundle.
	 * @return A bundle object.
	 */
	dtn::data::Bundle get(const dtn::data::EID &eid);

	/**
	 * Unblock the get()-call for a specific EID.
	 * @param eid
	 */
	void unblock(const dtn::data::EID &eid);

	/**
	 * This method deletes a specific bundle in the storage.
	 * No reports will be generated here.
	 * @param id The ID of the bundle to remove.
	 */
	void remove(const dtn::data::BundleID &id);

	/**
	 * Clears all bundles and fragments in the storage.
	 */
	void clear();

	/**
	 * @return True, if no bundles in the storage.
	 */
	bool empty();

	/**
	 * @return the count of bundles in the storage
	 */
	unsigned int count();

	void getVector(vector<dtn::data::Bundle> &bundles);

	/**
	 * @return the amount of available storage space
	 */
	//int availableSpace();

	/**
	 * This method is used to receive events.
	 * @param evt
	 */
	void raiseEvent(const Event *evt);

	/**
	 *
	 */
	void run(void);

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
	 *	Search bundles with the given EID. If no Bundle is found the return value is false (else true).
	 *	@param Destination EID
	 *	@param
	 */
	bool getBundle(const dtn::data::EID &eid, dtn::data::Bundle &bundle);

	//Tablename
	const std::string _BundleTable;
	const std::string _FragmentTable;

	bool global_shutdown;
	ibrcommon::File dbPath;
	string dbFile;
	int dbSize;
	sqlite3 *database;
	ibrcommon::Conditional dbMutex, timeeventConditional;

	/*
	 * This set contains unblock requests for threads blocked by get(EID)
	 */
	set<data::EID> unblockEID;

	sqlite3_stmt *getTTL;
	sqlite3_stmt *getBundleByDestination;
	sqlite3_stmt *getBundleByID;
	sqlite3_stmt *getFragements;
	sqlite3_stmt *store_Bundle;
	sqlite3_stmt *store_Fragment;
	sqlite3_stmt *clearStorage;
	sqlite3_stmt *countEntries;
	sqlite3_stmt *vacuum;
	sqlite3_stmt *getROWID;
	sqlite3_stmt *removeBundle;
	sqlite3_stmt *removeFragments;

};

}
}

#endif /* SQLITEBUNDLESTORAGE_H_ */

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

#include "Component.h"
#include "core/BundleStorage.h"
#include "core/EventReceiver.h"

#include <ibrcommon/thread/Thread.h>
#include <ibrcommon/thread/Conditional.h>
#include <ibrcommon/data/File.h>
#include <ibrcommon/thread/ThreadSafeQueue.h>

#include <sqlite3.h>
#include <string>
#include <set>
#include <vector>


namespace dtn
{
	namespace core
	{
		class SQLiteBundleStorage : public BundleStorage, public EventReceiver, public dtn::daemon::IndependentComponent
		{
		private:
			class Task
			{
			public:
				virtual ~Task() {};
				virtual void run() = 0;
			};

		public:
			/**
			 * Constructor
			 * @param Pfad zum Ordner in denen die Datein gespeichert werden.
			 * @param Dateiname der Datenbank
			 * @param maximale Größe der Datenbank
			 */
			SQLiteBundleStorage(ibrcommon::File dbPath ,string dbFile , size_t size);

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
			 * Returns one bundle which is not in the bloomfilter
			 * @param filter
			 * @return
			 */
			dtn::data::Bundle get(const ibrcommon::BloomFilter &filter);

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
			 * This method deletes a specific bundle in the storage.
			 * No reports will be generated here.
			 * @param id The ID of the bundle to remove.
			 */
			void remove(const dtn::data::BundleID &id);

			/**
			 * Remove all bundles which match this filter
			 * @param filter
			 */
			dtn::data::MetaBundle remove(const ibrcommon::BloomFilter &filter) { return BundleID(); };

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

			/**
			 * This method is called if another node accepts custody for a
			 * bundle of us.
			 * @param bundle
			 */
			void releaseCustody(dtn::data::BundleID &bundle);

			/**
			 * @return the amount of available storage space
			 */
			//int availableSpace();

			/**
			 * This method is used to receive events.
			 * @param evt
			 */
			void raiseEvent(const Event *evt);

		protected:
			virtual void componentUp();
			virtual void componentRun();
			virtual void componentDown();

		private:
		//	/**
		//	 * This Function stores Fragments.
		//	 * @param bundle The bundle to store.
		//	 */
		//	void storeFragment(const dtn::data::Bundle &bundle);

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

			//Tablename
			const std::string _BundleTable;
			const std::string _FragmentTable;

			ibrcommon::File dbPath;
			string dbFile;
			size_t dbSize;
			sqlite3 *database;
			ibrcommon::Conditional dbMutex;
			ibrcommon::ThreadSafeQueue<Task*> _tasks;

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

/*
 * SQLiteBundleStorage.cpp
 *
 *  Created on: 09.01.2010
 *      Author: Myrtus
 */


#include "core/SQLiteBundleStorage.h"
#include "core/TimeEvent.h"
#include "core/GlobalEvent.h"
#include "core/BundleCore.h"
#include "core/BundleExpiredEvent.h"
#include "core/SQLiteConfigure.h"
#include "core/BundleEvent.h"
#include "core/BundleExpiredEvent.h"

#include <ibrdtn/data/PayloadBlock.h>
#include <ibrdtn/data/Serializer.h>
#include <ibrdtn/data/Bundle.h>
#include <ibrdtn/data/BundleID.h>

#include <ibrcommon/thread/MutexLock.h>
#include <ibrcommon/Exceptions.h>
#include <ibrcommon/data/File.h>
#include <ibrcommon/data/BLOB.h>
#include <ibrcommon/AutoDelete.h>

#include <dirent.h>
#include <iostream>
#include <fstream>
#include <list>
#include <stdio.h>
#include <string.h>

using namespace std;


namespace dtn {
namespace core {

	const std::string SQLiteBundleStorage::_tables[SQL_TABLE_END] =
			{ "Bundles", "Block", "Routing", "BundleRoutingInfo", "NodeRoutingInfo" };

	SQLiteBundleStorage::SQLiteBundleStorage(const ibrcommon::File &path, const string &file, const int &size)
	 : global_shutdown(false), dbPath(path.getPath()), dbFile(file), dbSize(size), actual_time(0), nextExpiredTime(0)
	{
		int err = 0;

		//Configure SQLite Library
		SQLiteConfigure::configure();

		{
			//open the database
			err = sqlite3_open_v2((dbPath+"/"+dbFile).c_str(),&database,SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
			if (err){
				std::cerr << "Can't open database: " << sqlite3_errmsg(database);
				sqlite3_close(database);
				throw ibrcommon::Exception("Unable to open SQLite Database");
			}

			//create BundleTable
			sqlite3_stmt *createBundleTable = prepareStatement("create table if not exists "+ _tables[SQL_TABLE_BUNDLE] +" (Key INTEGER PRIMARY KEY ASC, BundleID text, Source text, Destination text, Reportto text, Custodian text, ProcFlags int, Timestamp int, Sequencenumber int, Lifetime int, Fragmentoffset int, Appdatalength int, TTL int, Size int, Payloadlength int, Payloadfilename text);");
			err = sqlite3_step(createBundleTable);
			if(err != SQLITE_DONE){
				std::cerr << "SQLiteBundleStorage: Constructorfailure: create BundleTable failed " << err;
			}
			sqlite3_finalize(createBundleTable);

			//create BlockTable
			sqlite3_stmt *createBlockTable = prepareStatement("create table if not exists "+ _tables[SQL_TABLE_BLOCK] +" (Key INTEGER PRIMARY KEY ASC, BundleID text, BlockType int, Filename text, Blocknumber int);");
			err = sqlite3_step(createBlockTable);
			if(err != SQLITE_DONE){
				std::cerr << "SQLiteBundleStorage: Constructorfailure: create BlockTable failed " << err;
			}
			sqlite3_finalize(createBlockTable);

			//create RoutingTable
			sqlite3_stmt *createRoutingTable = prepareStatement("create table if not exists "+ _tables[SQL_TABLE_ROUTING] +" (INTEGER PRIMARY KEY ASC, Key int, Routing text);");
			err = sqlite3_step(createRoutingTable);
			if(err != SQLITE_DONE){
				std::cerr << "SQLiteBundleStorage: Constructorfailure: create RoutingTable failed " << err;
			}
			sqlite3_finalize(createRoutingTable);

			//create BundleRoutingInfo
			sqlite3_stmt *createBundleRoutingInfo = prepareStatement("create table if not exists "+ _tables[SQL_TABLE_BUNDLE_ROUTING_INFO] +" (INTEGER PRIMARY KEY ASC, BundleID text, Key int, Routing text);");
			err = sqlite3_step(createBundleRoutingInfo);
			if(err != SQLITE_DONE){
				std::cerr << "SQLiteBundleStorage: Constructorfailure: create RoutingTable failed " << err;
			}
			sqlite3_finalize(createBundleRoutingInfo);

			//create NodeRoutingInfo
			sqlite3_stmt *createNodeRoutingInfo = prepareStatement("create table if not exists "+ _tables[SQL_TABLE_NODE_ROUTING_INFO] +" (INTEGER PRIMARY KEY ASC, EID text, Key int, Routing text);");
			err = sqlite3_step(createNodeRoutingInfo);
			if(err != SQLITE_DONE){
				std::cerr << "SQLiteBundleStorage: Constructorfailure: create RoutingTable failed " << err;
			}
			sqlite3_finalize(createNodeRoutingInfo);

			//Create Triggers
			sqlite3_stmt *createDELETETrigger = prepareStatement("CREATE TRIGGER IF NOT EXISTS Blockdelete AFTER DELETE ON "+ _tables[SQL_TABLE_BUNDLE] +" FOR EACH ROW BEGIN DELETE FROM "+ _tables[SQL_TABLE_BLOCK] +" WHERE BundleID = OLD.BundleID; DELETE FROM "+ _tables[SQL_TABLE_BUNDLE_ROUTING_INFO] +" WHERE BundleID = OLD.BundleID; END;");
			err = sqlite3_step(createDELETETrigger);
			if(err != SQLITE_DONE){
				std::cerr << "SQLiteBundleStorage: Constructorfailure: createTrigger failed " << err;
			}
			sqlite3_finalize(createDELETETrigger);

			sqlite3_stmt *createINSERTTrigger = prepareStatement("CREATE TRIGGER IF NOT EXISTS BundleRoutingdelete BEFORE INSERT ON "+ _tables[SQL_TABLE_BUNDLE_ROUTING_INFO] +" BEGIN SELECT CASE WHEN (SELECT BundleID FROM "+ _tables[SQL_TABLE_BUNDLE] +" WHERE BundleID = NEW.BundleID) IS NULL THEN RAISE(ABORT, \"Foreign Key Violation: BundleID doesn't exist in BundleTable\")END; END;");
			err = sqlite3_step(createINSERTTrigger);
			if(err != SQLITE_DONE){
				std::cerr << "SQLiteBundleStorage: Constructorfailure: createTrigger failed " << err;
			}
			sqlite3_finalize(createINSERTTrigger);

			//Chreate Indizes
			sqlite3_stmt *createBlockIDIndex = prepareStatement("CREATE INDEX IF NOT EXISTS BlockIDIndex ON "+ _tables[SQL_TABLE_BLOCK] +" (BundleID);");
			err = sqlite3_step(createBlockIDIndex);
			if(err != SQLITE_DONE){
				std::cerr << "SQLiteBundleStorage: Constructorfailure: createIndex failed " << err;
			}
			sqlite3_finalize(createBlockIDIndex);

			sqlite3_stmt *createBundleIDIndex = prepareStatement("CREATE UNIQUE INDEX IF NOT EXISTS BundleIDIndex ON "+ _tables[SQL_TABLE_BUNDLE] +" (BundleID);");
			err = sqlite3_step(createBundleIDIndex);
			if(err != SQLITE_DONE){
				std::cerr << "SQLiteBundleStorage: Constructorfailure: createIndex failed " << err;
			}
			sqlite3_finalize(createBundleIDIndex);

			sqlite3_stmt *createRoutingKeyIndex = prepareStatement("CREATE UNIQUE INDEX IF NOT EXISTS RoutingKeyIndex ON "+ _tables[SQL_TABLE_ROUTING] +" (Key);");
			err = sqlite3_step(createRoutingKeyIndex);
			if(err != SQLITE_DONE){
				std::cerr << "SQLiteBundleStorage: Constructorfailure: createIndex failed " << err;
			}
			sqlite3_finalize(createRoutingKeyIndex);

			sqlite3_stmt *createBundleRoutingIDIndex = prepareStatement("CREATE UNIQUE INDEX IF NOT EXISTS BundleRoutingIDIndex ON "+ _tables[SQL_TABLE_BUNDLE_ROUTING_INFO] +" (BundleID,Key);");
			err = sqlite3_step(createBundleRoutingIDIndex);
			if(err != SQLITE_DONE){
				std::cerr << "SQLiteBundleStorage: Constructorfailure: createIndex failed " << err;
			}
			sqlite3_finalize(createBundleRoutingIDIndex);
		}

		{//create folders
			stringstream bundlefolder;
			bundlefolder << dbPath << "/" << _tables[SQL_TABLE_BLOCK];
			ibrcommon::File folder(bundlefolder.str());
			ibrcommon::File::createDirectory(folder);
		}

		//Bundletable
		getBundleTTL 			= prepareStatement("SELECT Source,Timestamp,Sequencenumber FROM "+ _tables[SQL_TABLE_BUNDLE] +" WHERE TTL <= ? ORDER BY TTL ASC;");
		getBundleByDestination	= prepareStatement("SELECT BundleID, Source, Destination, Reportto, Custodian, ProcFlags, Timestamp, Sequencenumber, Lifetime, Fragmentoffset, Appdatalength FROM "+ _tables[SQL_TABLE_BUNDLE] +" WHERE Destination = ? ORDER BY TTL ASC;");

		getBundleByID 			= prepareStatement("SELECT Source, Destination, Reportto, Custodian, ProcFlags, Timestamp, Sequencenumber, Lifetime, Fragmentoffset, Appdatalength FROM "+ _tables[SQL_TABLE_BUNDLE] +" WHERE BundleID = ?;");
		getBundle 				= prepareStatement("SELECT BundleID, Source, Destination, Reportto, Custodian, ProcFlags, Timestamp, Sequencenumber, Lifetime, Fragmentoffset, Appdatalength FROM "+ _tables[SQL_TABLE_BUNDLE] +" ORDER BY TTL ASC;");
		getFragements			= prepareStatement("SELECT * FROM "+ _tables[SQL_TABLE_BUNDLE] +" WHERE Source = ? AND Timestamp = ? AND Sequencenumber = ? ORDER BY Fragmentoffset ASC;");

		getROWID 				= prepareStatement("SELECT ROWID from "+ _tables[SQL_TABLE_BUNDLE] +";");
		getProcFlags			= prepareStatement("SELECT ProcFlags From "+ _tables[SQL_TABLE_BUNDLE] +" WHERE BundleID = ?;");
		getnextExpiredBundle	= prepareStatement("SELECT TTL FROM "+ _tables[SQL_TABLE_BUNDLE] +" ORDER BY TTL ASC LIMIT 1;");
		getBundlesBySize		= prepareStatement("SELECT BundleID, Source, Destination, Reportto, Custodian, ProcFlags, Timestamp, Sequencenumber, Lifetime, Fragmentoffset, Appdatalength FROM "+ _tables[SQL_TABLE_BUNDLE] +" ORDER BY Size DESC LIMIT ?");
//		getBundleBetweenSize	= prepareStatement("SELECT Source, Destination, Reportto, Custodian, ProcFlags, Timestamp, Sequencenumber, Lifetime, Fragmentoffset, Appdatalength FROM "+ _tables[SQL_TABLE_BUNDLE] +" WHERE Size > ? ORDERED BY Size DESC LIMIT ?");
		getBundleBySource		= prepareStatement("SELECT BundleID, Destination, Reportto, Custodian, ProcFlags, Timestamp, Sequencenumber, Lifetime, Fragmentoffset, Appdatalength FROM "+ _tables[SQL_TABLE_BUNDLE] +" WHERE Source = ?;");
		getBundlesByTTL			= prepareStatement("SELECT BundleID, Source, Destination, Reportto, Custodian, ProcFlags, Timestamp, Sequencenumber, Lifetime, Fragmentoffset, Appdatalength FROM "+ _tables[SQL_TABLE_BUNDLE] +" ORDER BY TTL ASC LIMIT ?");
		countEntries 			= prepareStatement("SELECT Count(ROWID) From "+ _tables[SQL_TABLE_BUNDLE] +";");
		deleteBundleTTL			= prepareStatement("DELETE FROM "+ _tables[SQL_TABLE_BUNDLE] +" WHERE TTL <= ?;");
		deleteBundle			= prepareStatement("DELETE FROM "+ _tables[SQL_TABLE_BUNDLE] +" WHERE Source = ? AND Timestamp = ? AND Sequencenumber = ?;");
		removeBundle 			= prepareStatement("DELETE From "+ _tables[SQL_TABLE_BUNDLE] +" Where BundleID = ?;");
		clearBundles 			= prepareStatement("DELETE FROM "+ _tables[SQL_TABLE_BUNDLE] +";");
		store_Bundle 			= prepareStatement("INSERT INTO "+ _tables[SQL_TABLE_BUNDLE] +" (BundleID, Source, Destination, Reportto, Custodian, ProcFlags, Timestamp, Sequencenumber, Lifetime, Fragmentoffset, Appdatalength, TTL, Size, Payloadlength, Payloadfilename) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);");
		updateProcFlags			= prepareStatement("UPDATE "+ _tables[SQL_TABLE_BUNDLE] +" SET ProcFlags = ? WHERE BundleID = ?;");

		//Blocktable
		getBlocksByID			= prepareStatement("SELECT Filename FROM "+ _tables[SQL_TABLE_BLOCK] +" WHERE BundleID = ? ORDER BY Blocknumber ASC;");
		getBlocks				= prepareStatement("SELECT Filename FROM "+ _tables[SQL_TABLE_BLOCK] +" WHERE BundleID = ? AND Blocknumber = ?;");
		clearBlocks				= prepareStatement("DELETE FROM "+ _tables[SQL_TABLE_BLOCK] +";");
		storeBlock				= prepareStatement("INSERT INTO "+ _tables[SQL_TABLE_BLOCK] +" (BundleID, BlockType, Filename, Blocknumber) VALUES (?,?,?,?);");

		//Routingtable
		getRouting				= prepareStatement("SELECT Routing FROM "+ _tables[SQL_TABLE_ROUTING] +" WHERE Key = ?;");
		removeRouting			= prepareStatement("DELETE FROM "+ _tables[SQL_TABLE_ROUTING] +" WHERE Key = ?;");
		clearRouting			= prepareStatement("DELETE FROM "+ _tables[SQL_TABLE_ROUTING] +";");
		storeRouting			= prepareStatement("INSERT INTO "+ _tables[SQL_TABLE_ROUTING] +" (Key, Routing) VALUES (?,?);");

		//NodeRoutingInfotable
		getNodeRouting			= prepareStatement("SELECT Routing FROM "+ _tables[SQL_TABLE_NODE_ROUTING_INFO] +" WHERE EID = ? AND Key = ?;");
		removeNodeRouting		= prepareStatement("DELETE FROM "+ _tables[SQL_TABLE_NODE_ROUTING_INFO] +" WHERE EID = ? AND Key = ?;");
		clearNodeRouting		= prepareStatement("DELETE FROM "+ _tables[SQL_TABLE_NODE_ROUTING_INFO] +";");
		storeNodeRouting		= prepareStatement("INSERT INTO "+ _tables[SQL_TABLE_NODE_ROUTING_INFO] +" (EID, Key, Routing) VALUES (?,?,?);");

		//BundleRoutingInfotable
		getBundleRouting		= prepareStatement("SELECT Routing FROM "+ _tables[SQL_TABLE_BUNDLE_ROUTING_INFO] +" WHERE BundleID = ? AND Key = ?;");
		removeBundleRouting		= prepareStatement("DELETE FROM "+ _tables[SQL_TABLE_BUNDLE_ROUTING_INFO] +" WHERE BundleID = ? AND Key = ?;");
		storeBundleRouting		= prepareStatement("INSERT INTO "+ _tables[SQL_TABLE_BUNDLE_ROUTING_INFO] +" (BundleID, Key, Routing) VALUES (?,?,?);");

		vacuum 					= prepareStatement("vacuum;");




		//Check if the database is consistent with the filesystem
		consistenceCheck();

		// calculate next Bundleexpiredtime
		updateexpiredTime();

		//register Events
		bindEvent(TimeEvent::className);
		bindEvent(GlobalEvent::className);
	}

	SQLiteBundleStorage::~SQLiteBundleStorage(){

		ibrcommon::MutexLock lock(dbMutex);
		join();
		//finalize Statements
		sqlite3_finalize(getBundleTTL);
		sqlite3_finalize(deleteBundleTTL);
		sqlite3_finalize(getBundleByDestination);
		sqlite3_finalize(getBundleByID);
		sqlite3_finalize(getFragements);
		sqlite3_finalize(store_Bundle);
		sqlite3_finalize(clearBundles);
		sqlite3_finalize(clearBlocks);
		sqlite3_finalize(clearRouting);
		sqlite3_finalize(clearNodeRouting);
		sqlite3_finalize(countEntries);
		sqlite3_finalize(vacuum);
		sqlite3_finalize(getROWID);
		sqlite3_finalize(removeBundle);
		sqlite3_finalize(getBlocksByID);
		sqlite3_finalize(storeBlock);
		sqlite3_finalize(getBundlesBySize);
		sqlite3_finalize(getBundleBySource);
		sqlite3_finalize(getBundlesByTTL);
		sqlite3_finalize(getBlocks);
		sqlite3_finalize(getProcFlags);
		sqlite3_finalize(updateProcFlags);
		sqlite3_finalize(getBundleRouting);
		sqlite3_finalize(storeBundleRouting);
		sqlite3_finalize(removeBundleRouting);
		sqlite3_finalize(storeRouting);
		sqlite3_finalize(getRouting);
		sqlite3_finalize(removeRouting);
		sqlite3_finalize(storeNodeRouting);
		sqlite3_finalize(getNodeRouting);
		sqlite3_finalize(removeNodeRouting);
		sqlite3_finalize(getnextExpiredBundle);
		sqlite3_finalize(deleteBundle);
		sqlite3_finalize(getBundle);


		//close Databaseconnection
		int err = sqlite3_close(database);
		if (err != SQLITE_OK){
			cerr <<"unable to close Database" <<endl;
		}

		//unregister Events
		unbindEvent(TimeEvent::className);
		unbindEvent(GlobalEvent::className);
	}

	void SQLiteBundleStorage::executeQuerry(sqlite3_stmt *statement){
		int err;
		err = sqlite3_step(statement);
		sqlite3_reset(statement);
		if(err == SQLITE_CONSTRAINT)
			cerr << "warning: Bundle is already in the storage"<<endl;
		else if(err != SQLITE_DONE){
			stringstream error;
			error << "SQLiteBundleStorage: Unable to execute Querry: " << err << " errmsg: " << sqlite3_errmsg(database);
			throw SQLiteQuerryException(error.str());
		}

	}

	void SQLiteBundleStorage::deleteFiles(list<std::string> &fileList){
		list<std::string>::iterator it;
		int err;
		for(it = fileList.begin(); it != fileList.end(); it++){
			ibrcommon::File tmp(*it);
			err = tmp.remove();
			if(err != 0){
				stringstream error;
				error << "Unable to Delete File. Errorcode: "<<err;
				throw (ibrcommon::Exception(error.str() ));
			}
		}
	}

	void SQLiteBundleStorage::consistenceCheck(){
		/*
		 * check if the database is consistent with the files on the HDD. Therefore every file of the database is put in a list.
		 * When the files of the database are scanned it is checked if the actual file is in the list of files of the filesystem. if it is so the entry
		 * of the list of files of the filesystem is deleted. if not the file is put in a seperate list. After this procedure there are 2 lists containing file
		 * which are inconsistent and can be deleted.
		 */
		int err;
		size_t timestamp, sequencenumber, offset, pFlags, appdata, lifetime;
		set<string> blockFiles,payloadfiles;
		string datei;
		stringstream directory;

		list<ibrcommon::File> blist;
		list<ibrcommon::File>::iterator file_it;

		//1. Durchsuche die Dateien
		directory << dbPath << "/" << SQL_TABLE_BLOCK;

		ibrcommon::File folder(directory.str());

		folder.getFiles(blist);
		for(file_it = blist.begin(); file_it != blist.end(); file_it++){
			datei = (*file_it).getPath();
			if(datei[datei.size()-1] != '.'){
				if(((*file_it).getBasename())[0] == 'b'){
					blockFiles.insert(datei);
				}
				else
					payloadfiles.insert(datei);
			}
		}

		{
			ibrcommon::MutexLock lock(dbMutex);
			//3. Check consistency of the bundles
			checkBundles(blockFiles);

			//4. Check consistency of the fragments
			checkFragments(payloadfiles);
		}
	}

	void SQLiteBundleStorage::checkFragments(set<string> &payloadFiles){
		int err;
		size_t procFlags, timestamp, sequencenumber, appdatalength, lifetime;
		string filename, source, dest, custody, repto;
		set<string> consistenFiles, inconsistenSources;
		set<size_t> inconsistentTimestamp, inconsistentSeq_number;
		set<string>::iterator file_it, consisten_it;


		sqlite3_stmt *getPayloadfiles = prepareStatement("SELECT Source, Destination, Reportto, Custodian, ProcFlags, Timestamp, Sequencenumber, Lifetime, Fragmentoffset, Appdatalength, Payloadname FROM "+_tables[SQL_TABLE_BUNDLE]+" WHERE Payloadname != NULL;");
		for(err = sqlite3_step(getPayloadfiles); err == SQLITE_ROW; err = sqlite3_step(getPayloadfiles)){
			filename = (const char*)sqlite3_column_text(getPayloadfiles,10);
			file_it = payloadFiles.find(filename);


			if(file_it == payloadFiles.end()){
				consisten_it = consistenFiles.find(*file_it);

				//inconsistent DB entry
				if(consisten_it == consistenFiles.end()){
					//Generate Report
					source  = (const char*) sqlite3_column_text(getPayloadfiles,0);
					dest    = (const char*) sqlite3_column_text(getPayloadfiles,1);
					repto   = (const char*) sqlite3_column_text(getPayloadfiles,2);
					custody = (const char*) sqlite3_column_text(getPayloadfiles,3);
					procFlags = sqlite3_column_int64(getPayloadfiles,4);
					timestamp = sqlite3_column_int64(getPayloadfiles,5);
					sequencenumber = sqlite3_column_int64(getPayloadfiles,6);
					lifetime  = sqlite3_column_int64(getPayloadfiles,7);
					appdatalength  = sqlite3_column_int64(getPayloadfiles,9);

					dtn::data::BundleID id(dtn::data::EID(source),timestamp,sequencenumber);
					dtn::data::MetaBundle mb(id, lifetime, dtn::data::DTNTime(), dtn::data::EID(dest), dtn::data::EID(repto), dtn::data::EID(custody), appdatalength, procFlags);
					dtn::core::BundleEvent::raise(mb, BUNDLE_DELETED, dtn::data::StatusReportBlock::DEPLETED_STORAGE);
				}
			}
			//consistent DB entry
			else
				consistenFiles.insert(*file_it);
				payloadFiles.erase(file_it);
		}

		sqlite3_finalize(getPayloadfiles);
	}

	void SQLiteBundleStorage::checkBundles(set<string> &blockFiles){
		int err;
		size_t procFlags,timestamp,sequencenumber,appdatalength,lifetime;

		string source,Bundle;
		string filename, id, payloadfilename;
		string dest, custody, repto;

		set<std::string> blockIDList;
		set<string>::iterator fileList_iterator;

		sqlite3_stmt *blockConistencyCheck = prepareStatement("SELECT Filename,BundleID,Blocknumber FROM "+ _tables[SQL_TABLE_BLOCK] +";");
		for(err = sqlite3_step(blockConistencyCheck); err == SQLITE_ROW; err = sqlite3_step(blockConistencyCheck)){
			filename = (const char*)sqlite3_column_text(blockConistencyCheck,0);
			id = (const char*)sqlite3_column_text(blockConistencyCheck,1);
			fileList_iterator = blockFiles.find(filename);

			//Inconsistent:
			if(fileList_iterator == blockFiles.end()){
				blockIDList.insert(id);
			}
			//Consistent:
			else{
				blockFiles.erase(fileList_iterator);
			}
		}
		sqlite3_finalize(blockConistencyCheck);

		for(fileList_iterator = blockIDList.begin(); fileList_iterator != blockIDList.end(); fileList_iterator++) {
			//Get Blockfile of inconsistent Bundles and add them to blockFile.
			sqlite3_bind_text(getBlocksByID,1,(*fileList_iterator).c_str(),(*fileList_iterator).length(),SQLITE_TRANSIENT);
			for (err = sqlite3_step(getBlocksByID); err == SQLITE_ROW; err = sqlite3_step(getBlocksByID)){
				filename = (const char*)sqlite3_column_text(getBlocksByID,0);
				blockFiles.insert(filename);
			}
			sqlite3_reset(getBlocksByID);
			if(err != SQLITE_DONE){
				stringstream error;
				error << "SQLiteBundleStorage: Unable to execute Querry: " << err << " errmsg: " << sqlite3_errmsg(database);
				throw SQLiteQuerryException(error.str());
			}

			//Generate Report
			sqlite3_bind_text(getBundleByID,1,(*fileList_iterator).c_str(),(*fileList_iterator).length(),SQLITE_TRANSIENT);
			err = sqlite3_step(getBundleByID);
			if(err == SQLITE_ROW){
				source  = (const char*) sqlite3_column_text(getBundleByID,0);
				dest    = (const char*) sqlite3_column_text(getBundleByID,1);
				repto   = (const char*) sqlite3_column_text(getBundleByID,2);
				custody = (const char*) sqlite3_column_text(getBundleByID,3);
				procFlags = sqlite3_column_int64(getBundleByID,4);
				timestamp = sqlite3_column_int64(getBundleByID,5);
				sequencenumber = sqlite3_column_int64(getBundleByID,6);
				lifetime  = sqlite3_column_int64(getBundleByID,7);
				appdatalength  = sqlite3_column_int64(getBundleByID,9);

				dtn::data::BundleID id(dtn::data::EID(source),timestamp,sequencenumber);
				dtn::data::MetaBundle mb(id, lifetime, dtn::data::DTNTime(), dtn::data::EID(dest), dtn::data::EID(repto), dtn::data::EID(custody), appdatalength, procFlags);
				dtn::core::BundleEvent::raise(mb, BUNDLE_DELETED, dtn::data::StatusReportBlock::DEPLETED_STORAGE);
			}
			else{
				stringstream error;
				error << "Inconsistence of the database. The BundleID " << id << " was not found.";
				throw ibrcommon::Exception(error.str());
			}

			//delete BundleTable entries (include the detele of the blocks while a databasetrigger)
			sqlite3_bind_text(removeBundle,1,(*fileList_iterator).c_str(), (*fileList_iterator).length(),SQLITE_TRANSIENT);
			executeQuerry(removeBundle);
		}

		//Delete Blockfiles
		for(fileList_iterator = blockFiles.begin(); fileList_iterator != blockFiles.end(); fileList_iterator++){
			ibrcommon::File blockfile(*fileList_iterator);
			blockfile.remove();
		}
	}


	sqlite3_stmt* SQLiteBundleStorage::prepareStatement(string sqlquery){
		// prepare the statement
		sqlite3_stmt *statement;
		int err = sqlite3_prepare_v2(database, sqlquery.c_str(), sqlquery.length(), &statement, 0);
		if ( err != SQLITE_OK )
		{
			std::cerr << "SQLiteBundlestorage: failure in prepareStatement: " << err << " with Query: " << sqlquery << std::endl;
		}
		return statement;
	}

	dtn::data::Bundle SQLiteBundleStorage::get(const dtn::data::BundleID &id){
		int err;
		size_t procflag;
		data::Bundle bundle;
		//String wird in diesem Fall verwendet, da das const char arrey bei einem step, reset oder finalize verändert wird
		//und die Daten kopiert werden müssen.
		string source, destination, reportto, custodian;

		//parameterize querry
		string ID = id.toString();

		{
			ibrcommon::MutexLock lock(dbMutex);
			sqlite3_bind_text(getBundleByID, 1, ID.c_str(), ID.length(),SQLITE_TRANSIENT);

			//executing querry
			err = sqlite3_step(getBundleByID);

			//If an error occurs
			if(err != SQLITE_ROW){
				sqlite3_reset(getBundleByID);
				stringstream error;
				error << "SQLiteBundleStorage: No Bundle found with BundleID: " << id.toString();
				std::cerr << error.str();
				throw SQLiteQuerryException(error.str());
			}
			source = (const char*) sqlite3_column_text(getBundleByID, 0);
			destination = (const char*) sqlite3_column_text(getBundleByID, 1);
			reportto = (const char*) sqlite3_column_text(getBundleByID, 2);
			custodian = (const char*) sqlite3_column_text(getBundleByID, 3);
			procflag = sqlite3_column_int(getBundleByID, 4);
			bundle._timestamp = sqlite3_column_int64(getBundleByID, 5);
			bundle._sequencenumber = sqlite3_column_int64(getBundleByID, 6);
			bundle._lifetime = sqlite3_column_int64(getBundleByID, 7);
			if(procflag & data::Bundle::FRAGMENT){
				bundle._fragmentoffset = sqlite3_column_int64(getBundleByID, 8);
				bundle._appdatalength = sqlite3_column_int64(getBundleByID,9);
			}

			err = sqlite3_step(getBundleByID);

			//reset the statement
			sqlite3_reset(getBundleByID);

			if (err != SQLITE_DONE){
				throw "SQLiteBundleStorage: Database contains two or more Bundle with the same BundleID, the requested Bundle is maybe a Fragment";
			}

			//readBlocks
			readBlocks(bundle,id.toString());
		}

		bundle._source = data::EID(source);
		bundle._destination = data::EID(destination);
		bundle._reportto = data::EID(reportto);
		bundle._custodian = data::EID(custodian);
		bundle._procflags = procflag;

		return bundle;
	}

	const dtn::data::MetaBundle SQLiteBundleStorage::getByDestination(const dtn::data::EID &eid, bool exact, bool singleton)
	{
		int err;
		size_t procflag;
		dtn::data::MetaBundle bundle;
		//String wird in diesem Fall verwendet, da das const char arrey bei einem step, reset oder finalize verändert wird
		//und die Daten kopiert werden müssen.
		string source, destination, reportto, custodian;
		string ID(eid.getString()), bundleID;

		{
			ibrcommon::MutexLock lock(dbMutex);
			//parameterize querry
			sqlite3_bind_text(getBundleByDestination, 1, ID.c_str(), ID.length(),SQLITE_TRANSIENT);

			//executing querry
			err = sqlite3_step(getBundleByDestination);

			//If an error occurs
			if(err != SQLITE_ROW){
					sqlite3_reset(getBundleByDestination);
					throw NoBundleFoundException();
			}
			bundleID = (const char*) sqlite3_column_text(getBundleByDestination, 0);
			source = (const char*) sqlite3_column_text(getBundleByDestination, 1);
			destination = (const char*) sqlite3_column_text(getBundleByDestination, 2);
			reportto = (const char*) sqlite3_column_text(getBundleByDestination, 3);
			custodian = (const char*) sqlite3_column_text(getBundleByDestination, 4);
			procflag = sqlite3_column_int(getBundleByDestination, 5);
			bundle.timestamp = sqlite3_column_int64(getBundleByDestination, 6);
			bundle.sequencenumber = sqlite3_column_int64(getBundleByDestination, 7);
			bundle.lifetime = sqlite3_column_int64(getBundleByDestination, 8);
			if(procflag & data::Bundle::FRAGMENT)
			{
				bundle.fragment = true;
				bundle.offset = sqlite3_column_int64(getBundleByDestination, 9);
				bundle.appdatalength = sqlite3_column_int64(getBundleByDestination,10);
			}

			bundle.received = 0;
			bundle.source = data::EID(source);
			bundle.destination = data::EID(destination);
			bundle.reportto = data::EID(reportto);
			bundle.custodian = data::EID(custodian);
			bundle.procflags = procflag;


			//reset the statement
			sqlite3_reset(getBundleByDestination);
		}

		return bundle;
	}

	const dtn::data::MetaBundle SQLiteBundleStorage::getByFilter(const ibrcommon::BloomFilter &filter)
	{
		int err;
		size_t procflag;
		bool result(false);
		string bundleID, source, destination, reportto, custodian;
		dtn::data::MetaBundle bundle;

		{
			ibrcommon::MutexLock lock(dbMutex);
			for(err = sqlite3_step(getBundle); err == SQLITE_ROW; err = sqlite3_step(getBundle)){
				bundleID = (const char*) sqlite3_column_text(getBundle, 0);
				if(!filter.contains(bundleID)){
					source = 		(const char*) sqlite3_column_text(getBundle, 1);
					destination = 	(const char*) sqlite3_column_text(getBundle, 2);
					reportto = 		(const char*) sqlite3_column_text(getBundle, 3);
					custodian = 	(const char*) sqlite3_column_text(getBundle, 4);
					procflag = 				sqlite3_column_int64(getBundle,5);
					bundle.timestamp = 		sqlite3_column_int64(getBundle,6);
					bundle.sequencenumber = sqlite3_column_int64(getBundle,7);
					bundle.lifetime = 		sqlite3_column_int64(getBundle,8);
					if(procflag & data::Bundle::FRAGMENT)
					{
						bundle.fragment = true;
						bundle.offset = 		sqlite3_column_int64(getBundle,9);
						bundle.appdatalength = 	sqlite3_column_int64(getBundle,10);
					}
					bundle.received = 0;
					bundle.procflags = procflag;
					bundle.source = data::EID(source);
					bundle.destination = data::EID(destination);
					bundle.reportto = data::EID(reportto);
					bundle.custodian = data::EID(custodian);

					sqlite3_reset(getBundle);
					return(bundle);
				}
			}
			sqlite3_reset(getBundle);
			if(err != SQLITE_DONE){
				stringstream error;
				error << "SQLiteBundleStorage: Unable to execute Querry: " << err << " errmsg: " << sqlite3_errmsg(database);
				throw SQLiteQuerryException(error.str());
			}
			throw BundleStorage::NoBundleFoundException();
		}
	}

	list<data::Bundle> SQLiteBundleStorage::getBundleByTTL (const int &limit){
		int err;
		size_t procflags;
		string destination, reportto, custodian, bundleID, sourceID;
		list<data::Bundle> result;
		{
			ibrcommon::MutexLock lock(dbMutex);
			sqlite3_bind_int(getBundlesByTTL, 1, limit);
			err = sqlite3_step(getBundlesByTTL);

			while(err == SQLITE_ROW){
				data::Bundle bundle;
				stringstream stream_bundleid;
				bundleID 				= (const char*) sqlite3_column_text(getBundlesByTTL, 0);
				sourceID				= (const char*) sqlite3_column_text(getBundlesByTTL, 1);
				destination 			= (const char*) sqlite3_column_text(getBundlesByTTL, 2);
				reportto 				= (const char*) sqlite3_column_text(getBundlesByTTL, 3);
				custodian 				= (const char*) sqlite3_column_text(getBundlesByTTL, 4);
				procflags 				= sqlite3_column_int(getBundlesByTTL,5);
				bundle._timestamp 		= sqlite3_column_int64(getBundlesByTTL, 6);
				bundle._sequencenumber 	= sqlite3_column_int64(getBundlesByTTL, 7);
				bundle._lifetime 		= sqlite3_column_int64(getBundlesByTTL, 8);

				if(procflags & data::Bundle::FRAGMENT){
					bundle._fragmentoffset = sqlite3_column_int64(getBundlesByTTL,9);
					bundle._appdatalength  = sqlite3_column_int64(getBundlesByTTL,10);
				}

				bundle._source = data::EID(sourceID);
				bundle._destination = data::EID(destination);
				bundle._reportto = data::EID(reportto);
				bundle._custodian = data::EID(custodian);
				bundle._procflags = procflags;

				//read Blocks
				readBlocks(bundle,bundleID);

				//Add Bundlte to the result list
				result.push_back(bundle);
				err = sqlite3_step(getBundlesByTTL);
			}
			sqlite3_reset(getBundlesByTTL);
			//If an error occurs
			if(err != SQLITE_DONE){
				stringstream error;
				error << "getBundleBySource: error while executing querry: " << err;
				std::cerr << error.str();
				throw SQLiteQuerryException(error.str());
			}
		}
		return result;
	}

	list<data::Bundle>  SQLiteBundleStorage::getBundlesBySource(const dtn::data::EID &sourceID){
		int err;
		size_t procflags;
		string destination, reportto, custodian, bundleID;
		list<data::Bundle> result;
		{
			ibrcommon::MutexLock lock(dbMutex);
			sqlite3_bind_text(getBundleBySource, 1, sourceID.getString().c_str(), sourceID.getString().length(),SQLITE_TRANSIENT);
			err = sqlite3_step(getBundleBySource);

			while(err == SQLITE_ROW){
				data::Bundle bundle;
				stringstream stream_bundleid;
				bundleID 				= (const char*) sqlite3_column_text(getBundleBySource, 0);
				destination 			= (const char*) sqlite3_column_text(getBundleBySource, 1);
				reportto 				= (const char*) sqlite3_column_text(getBundleBySource, 2);
				custodian 				= (const char*) sqlite3_column_text(getBundleBySource, 3);
				procflags 				= sqlite3_column_int(getBundleBySource,4);
				bundle._timestamp 		= sqlite3_column_int64(getBundleBySource, 5);
				bundle._sequencenumber 	= sqlite3_column_int64(getBundleBySource, 6);
				bundle._lifetime 		= sqlite3_column_int64(getBundleBySource, 7);

				if(procflags & data::Bundle::FRAGMENT){
					bundle._fragmentoffset = sqlite3_column_int64(getBundleBySource, 8);
					bundle._appdatalength  = sqlite3_column_int64(getBundleBySource,9);
				}

				bundle._source = sourceID;
				bundle._destination = data::EID(destination);
				bundle._reportto = data::EID(reportto);
				bundle._custodian = data::EID(custodian);
				bundle._procflags = procflags;

				//read Blocks
				readBlocks(bundle,bundleID);

				//Add Bundlte to the result list
				result.push_back(bundle);
				err = sqlite3_step(getBundleBySource);
			}
			sqlite3_reset(getBundleBySource);
			//If an error occurs
			if(err != SQLITE_DONE){
				stringstream error;
				error << "getBundleBySource: error while executing querry: " << err;
				std::cerr << error.str();
				throw SQLiteQuerryException(error.str());
			}
		}
		return result;
	}

	list<data::Bundle> SQLiteBundleStorage::getBundlesByDestination(const dtn::data::EID &destinationID){
		int err;
		size_t procflags;
		const char *sourceID, *reportto, *custodian, *bundleID;
		list<data::Bundle> result;
		{
			ibrcommon::MutexLock lock(dbMutex);
			sqlite3_bind_text(getBundleByDestination, 1, destinationID.getString().c_str(), destinationID.getString().length(),SQLITE_TRANSIENT);
			err = sqlite3_step(getBundleByDestination);

			while(err == SQLITE_ROW){
				data::Bundle bundle;
				stringstream stream_bundleid;
				bundleID 				= (const char*) sqlite3_column_text(getBundleByDestination, 0);
				sourceID				= (const char*) sqlite3_column_text(getBundleByDestination, 1);
				reportto 				= (const char*) sqlite3_column_text(getBundleByDestination, 3);
				custodian 				= (const char*) sqlite3_column_text(getBundleByDestination, 4);
				procflags 				= sqlite3_column_int(getBundleByDestination, 5);
				bundle._timestamp 		= sqlite3_column_int64(getBundleByDestination, 6);
				bundle._sequencenumber 	= sqlite3_column_int64(getBundleByDestination, 7);
				bundle._lifetime 		= sqlite3_column_int64(getBundleByDestination, 8);

				if(procflags & data::Bundle::FRAGMENT){
					bundle._fragmentoffset = sqlite3_column_int64(getBundleByDestination,9);
					bundle._appdatalength  = sqlite3_column_int64(getBundleByDestination,10);
				}

				bundle._source = data::EID((string)sourceID);
				bundle._destination = destinationID;
				bundle._reportto = data::EID((string)reportto);
				bundle._custodian = data::EID((string)custodian);
				bundle._procflags = procflags;

				//read Blocks
				readBlocks(bundle,(string)bundleID);

				//Add Bundlte to the result list
				result.push_back(bundle);
				err = sqlite3_step(getBundleByDestination);
			}
			sqlite3_reset(getBundleByDestination);
			//If an error occurs
			if(err != SQLITE_DONE){
				stringstream error;
				error << "getBundleBySource: error while executing querry: " << err;
				std::cerr << error.str();
				throw SQLiteQuerryException(error.str());
			}
		}
		return result;
	}

	list<data::Bundle> SQLiteBundleStorage::getBundleBySize(const int &limit){
		list<data::Bundle> result;
		const char *bundleID, *sourceID, *reportto, *custodian, *destinationID;
		size_t procflags;
		int err;
		{
			ibrcommon::MutexLock lock(dbMutex);
			sqlite3_bind_int(getBundlesBySize, 1, limit);
			err = sqlite3_step(getBundlesBySize);
			while(err == SQLITE_ROW){
				data::Bundle bundle;
				stringstream stream_bundleid;
				bundleID 				= (const char*) sqlite3_column_text(getBundlesBySize, 0);
				sourceID				= (const char*) sqlite3_column_text(getBundlesBySize, 1);
				destinationID			= (const char*) sqlite3_column_text(getBundlesBySize, 2);
				reportto 				= (const char*) sqlite3_column_text(getBundlesBySize, 3);
				custodian 				= (const char*) sqlite3_column_text(getBundlesBySize, 4);
				procflags 				= sqlite3_column_int(getBundlesBySize, 5);
				bundle._timestamp 		= sqlite3_column_int64(getBundlesBySize, 6);
				bundle._sequencenumber 	= sqlite3_column_int64(getBundlesBySize, 7);
				bundle._lifetime 		= sqlite3_column_int64(getBundlesBySize, 8);

				if(procflags & data::Bundle::FRAGMENT){
					bundle._fragmentoffset = sqlite3_column_int64(getBundlesBySize,9);
					bundle._appdatalength  = sqlite3_column_int64(getBundlesBySize,10);
				}

				bundle._source = data::EID((string)sourceID);
				bundle._destination = data::EID((string)destinationID);
				bundle._reportto = data::EID((string)reportto);
				bundle._custodian = data::EID((string)custodian);
				bundle._procflags = procflags;

				//read Blocks
				readBlocks(bundle,(string)bundleID);

				//Add Bundlte to the result list
				result.push_back(bundle);
				err = sqlite3_step(getBundlesBySize);
			}
			sqlite3_reset(getBundlesBySize);
			//If an error occurs
			if(err != SQLITE_DONE){
				stringstream error;
				error << "getBundleBySource: error while executing querry: " << err;
				std::cerr << error.str();
				throw SQLiteQuerryException(error.str());
			}
		}

		return result;
	}


	std::string SQLiteBundleStorage::getBundleRoutingInfo(const data::BundleID &bundleID, const int &key){
		int err;
		string result;
		{
			ibrcommon::MutexLock lock(dbMutex);
			sqlite3_bind_text(getBundleRouting,1,bundleID.toString().c_str(),bundleID.toString().length(), SQLITE_TRANSIENT);
			sqlite3_bind_int(getBundleRouting,2,key);
			err = sqlite3_step(getBundleRouting);
			if(err == SQLITE_ROW){
				result = (const char*) sqlite3_column_text(getBundleRouting,0);
			}
			sqlite3_reset(getBundleRouting);
			if(err != SQLITE_DONE && err != SQLITE_ROW){
				stringstream error;
				error << "SQLiteBundleStorage: getBundleRoutingInfo: " << err << " errmsg: " << err;
				std::cerr << error.str();
				throw SQLiteQuerryException(error.str());
			}
		}
		return result;
	}

	std::string SQLiteBundleStorage::getNodeRoutingInfo(const data::EID &eid, const int &key){
		int err;
		string result;
		{
			ibrcommon::MutexLock lock(dbMutex);
			err = sqlite3_bind_text(getNodeRouting,1,eid.getString().c_str(),eid.getString().length(), SQLITE_TRANSIENT);
			err = sqlite3_bind_int(getNodeRouting,2,key);
			err = sqlite3_step(getNodeRouting);
			if(err == SQLITE_ROW){
				result = (const char*) sqlite3_column_text(getNodeRouting,0);
			}
			sqlite3_reset(getNodeRouting);
			if(err != SQLITE_DONE && err != SQLITE_ROW){
				stringstream error;
				error << "SQLiteBundleStorage: getNodeRoutingInfo: " << err << " errmsg: " << sqlite3_errmsg(database);
				std::cerr << error.str();
				throw SQLiteQuerryException(error.str());
			}
		}
		return result;
	}

	std::string SQLiteBundleStorage::getRoutingInfo(const int &key){
		int err;
		string result;
		{
			ibrcommon::MutexLock lock(dbMutex);
			sqlite3_bind_int(getRouting,1,key);
			err = sqlite3_step(getRouting);
			if(err == SQLITE_ROW){
				result = (const char*) sqlite3_column_text(getRouting,0);
			}
			sqlite3_reset(getRouting);
			if(err != SQLITE_DONE && err != SQLITE_ROW){
				stringstream error;
				error << "SQLiteBundleStorage: getRoutingInfo: " << err << " errmsg: " << sqlite3_errmsg(database);
				std::cerr << error.str();
				throw SQLiteQuerryException(error.str());
			}
		}
		return result;
	}

	void SQLiteBundleStorage::setPriority(const int priority,const dtn::data::BundleID &id){
		int err;
		size_t procflags;
		{
			ibrcommon::MutexLock lock(dbMutex);
			sqlite3_bind_text(getProcFlags,1,id.toString().c_str(), id.toString().length(), SQLITE_TRANSIENT);
			err = sqlite3_step(getProcFlags);
			if(err == SQLITE_ROW)
				procflags = sqlite3_column_int(getProcFlags,0);
			sqlite3_reset(getProcFlags);
			if(err != SQLITE_DONE){
				stringstream error;
				error << "SQLiteBundleStorage: error while Select Querry: " << err;
				std::cerr << error.str();
				throw SQLiteQuerryException(error.str());
			}

			//Priority auf 0 Setzen
			procflags -= ( 128*(procflags & dtn::data::Bundle::PRIORITY_BIT1) + 256*(procflags & dtn::data::Bundle::PRIORITY_BIT2));

			//Set Prioritybits
			switch(priority){
			case 1: procflags += data::Bundle::PRIORITY_BIT1; break; //Set PRIORITY_BIT1
			case 2: procflags += data::Bundle::PRIORITY_BIT2; break;
			case 3: procflags += (data::Bundle::PRIORITY_BIT1 + data::Bundle::PRIORITY_BIT2); break;
			}

			sqlite3_bind_text(updateProcFlags,1,id.toString().c_str(), id.toString().length(), SQLITE_TRANSIENT);
			sqlite3_bind_int64(updateProcFlags,2,priority);
			sqlite3_step(updateProcFlags);
			sqlite3_reset(updateProcFlags);
			if(err != SQLITE_DONE){
				stringstream error;
				error << "SQLiteBundleStorage: setPriority error while Select Querry: " << err;
				std::cerr << error.str();
				throw SQLiteQuerryException(error.str());
			}
		}
	}

	void SQLiteBundleStorage::store(const dtn::data::Bundle &bundle){
		bool local = bundle._destination.getNodeEID() == BundleCore::local.getNodeEID();
		if(bundle.get(dtn::data::Bundle::FRAGMENT) && local){
			storeFragment(bundle);
			return;
		}
		int err;
		size_t TTL;
		stringstream  stream_bundleid, completefilename;
		string bundlestring, destination;

		destination = bundle._destination.getString();
		stream_bundleid  << "[" << bundle._timestamp << "." << bundle._sequencenumber;
		if (bundle.get(dtn::data::Bundle::FRAGMENT))
		{
			stream_bundleid << "." << bundle._fragmentoffset;
		}
		stream_bundleid << "] " << bundle._source.getString();
		TTL = bundle._timestamp + bundle._lifetime;
		{
			ibrcommon::MutexLock lock(dbMutex);
			//stores the blocks to a file
			int size = storeBlocks(bundle);

			sqlite3_bind_text(store_Bundle, 1, stream_bundleid.str().c_str(), stream_bundleid.str().length(),SQLITE_TRANSIENT);
			sqlite3_bind_text(store_Bundle, 2,bundle._source.getString().c_str(), bundle._source.getString().length(),SQLITE_TRANSIENT);
			sqlite3_bind_text(store_Bundle, 3,destination.c_str(), destination.length(),SQLITE_TRANSIENT);
			sqlite3_bind_text(store_Bundle, 4,bundle._reportto.getString().c_str(), bundle._reportto.getString().length(),SQLITE_TRANSIENT);
			sqlite3_bind_text(store_Bundle, 5,bundle._custodian.getString().c_str(), bundle._custodian.getString().length(),SQLITE_TRANSIENT);
			sqlite3_bind_int(store_Bundle,6,bundle._procflags);
			sqlite3_bind_int64(store_Bundle,7,bundle._timestamp);
			sqlite3_bind_int(store_Bundle,8,bundle._sequencenumber);
			sqlite3_bind_int64(store_Bundle,9,bundle._lifetime);
			sqlite3_bind_int64(store_Bundle,10,NULL);
			sqlite3_bind_int64(store_Bundle,11,NULL);
			sqlite3_bind_int64(store_Bundle,12,TTL);
			sqlite3_bind_int(store_Bundle,13,size);
			sqlite3_bind_int(store_Bundle,14,NULL);
			sqlite3_bind_text(store_Bundle,15,NULL,0,SQLITE_TRANSIENT);
			err = sqlite3_step(store_Bundle);
			if(err == SQLITE_CONSTRAINT)
				cerr << "warning: Bundle is already in the storage"<<endl;
			else if(err != SQLITE_DONE){
				stringstream error;
				error << "SQLiteBundleStorage: store() failure: "<< err << " " <<  sqlite3_errmsg(database);
				std::cerr << error.str();
				throw SQLiteQuerryException(error.str());
			}
			sqlite3_reset(store_Bundle);
		}
		{
			ibrcommon::MutexLock lock = ibrcommon::MutexLock(time_change);
			if(nextExpiredTime == 0 || TTL < nextExpiredTime){
				nextExpiredTime = TTL;
			}
		}
	}

	void SQLiteBundleStorage::storeBundleRoutingInfo(const data::BundleID &bundleID, const int &key, const string &routingInfo){
		int err;
		{
			ibrcommon::MutexLock lock(dbMutex);
			sqlite3_bind_text(storeBundleRouting,1,bundleID.toString().c_str(),bundleID.toString().length(), SQLITE_TRANSIENT);
			sqlite3_bind_int(storeBundleRouting,2,key);
			sqlite3_bind_text(storeBundleRouting,3,routingInfo.c_str(),routingInfo.length(), SQLITE_TRANSIENT);
			err = sqlite3_step(storeBundleRouting);
			sqlite3_reset(storeBundleRouting);
			if(err == SQLITE_CONSTRAINT)
				cerr << "warning: BundleRoutingInformations for "<<bundleID.toString() <<" are either already in the storage or there is no Bundle with this BundleID."<<endl;
			else if(err != SQLITE_DONE){
				stringstream error;
				error << "SQLiteBundleStorage: storeBundleRoutingInfo: " << err << " errmsg: " << err;
				std::cerr << error.str();
				throw SQLiteQuerryException(error.str());
			}
		}
	}

	void SQLiteBundleStorage::storeNodeRoutingInfo(const data::EID &node, const int &key, const std::string &routingInfo){
		int err;
		{
			ibrcommon::MutexLock lock(dbMutex);
			sqlite3_bind_text(storeNodeRouting,1,node.getString().c_str(),node.getString().length(), SQLITE_TRANSIENT);
			sqlite3_bind_int(storeNodeRouting,2,key);
			sqlite3_bind_text(storeNodeRouting,3,routingInfo.c_str(),routingInfo.length(), SQLITE_TRANSIENT);
			err = sqlite3_step(storeNodeRouting);
			sqlite3_reset(storeNodeRouting);
			if(err == SQLITE_CONSTRAINT)
				cerr << "warning: NodeRoutingInfo for "<<node.getString() <<" are already in the storage."<<endl;
			else if(err != SQLITE_DONE){
				stringstream error;
				error << "SQLiteBundleStorage: storeNodeRoutingInfo: " << err << " errmsg: " << err;
				std::cerr << error.str();
				throw SQLiteQuerryException(error.str());
			}
		}
	}

	void SQLiteBundleStorage::storeRoutingInfo(const int &key, const std::string &routingInfo){
		int err;
		{
			ibrcommon::MutexLock lock(dbMutex);
			sqlite3_bind_int(storeRouting,1,key);
			sqlite3_bind_text(storeRouting,2,routingInfo.c_str(),routingInfo.length(), SQLITE_TRANSIENT);
			err = sqlite3_step(storeRouting);
			sqlite3_reset(storeRouting);
			if(err == SQLITE_CONSTRAINT)
				cerr << "warning: There are already RoutingInformation refereed by this Key"<<endl;
			else if(err != SQLITE_DONE){
				stringstream error;
				error << "SQLiteBundleStorage: storeRoutingInfo: " << err << " errmsg: " << err;
				std::cerr << error.str();
				throw SQLiteQuerryException(error.str());
			}
		}
	}

	void SQLiteBundleStorage::remove(const dtn::data::BundleID &id){
		int err;
		string filename;
		list<const char*> blocklist;
		list<const char*>::iterator it;
		//enter Lock
		{
			ibrcommon::MutexLock lock(dbMutex);
			sqlite3_bind_text(getBlocksByID, 1, id.toString().c_str(), id.toString().length(),SQLITE_TRANSIENT);
			err = sqlite3_step(getBlocksByID);
			if(err == SQLITE_DONE)
				return;
			while (err == SQLITE_ROW){
				filename = (const char*)sqlite3_column_text(getBlocksByID,0);
				deleteList.push_front(filename);
				err = sqlite3_step(getBlocksByID);
			}
			sqlite3_reset(getBlocksByID);
			if(err != SQLITE_DONE){
				stringstream error;
				error << "SQLiteBundleStorage: failure while getting filename: " << id.toString() << " errmsg: " << err;
				std::cerr << error.str();
				throw SQLiteQuerryException(error.str());
			}

			sqlite3_bind_text(removeBundle, 1, id.toString().c_str(), id.toString().length(),SQLITE_TRANSIENT);
			err = sqlite3_step(removeBundle);
			sqlite3_reset(removeBundle);
			if(err != SQLITE_DONE){
				stringstream error;
				error << "SQLiteBundleStorage: failure while removing: " << id.toString() << " errmsg: " << err;
				std::cerr << error.str();
				throw SQLiteQuerryException(error.str());
			}

			//update deprecated timer
			updateexpiredTime();
		}
	}

	void SQLiteBundleStorage::removeBundleRoutingInfo(const data::BundleID &bundleID, const int &key){
		int err;
		ibrcommon::MutexLock lock(dbMutex);
		sqlite3_bind_text(removeBundleRouting,1,bundleID.toString().c_str(),bundleID.toString().length(), SQLITE_TRANSIENT);
		sqlite3_bind_int(removeBundleRouting,2,key);
		err = sqlite3_step(removeBundleRouting);
		sqlite3_reset(removeBundleRouting);
		if(err != SQLITE_DONE){
			stringstream error;
			error << "SQLiteBundleStorage: removeBundleRoutingInfo: " << err << " errmsg: " << err;
			std::cerr << error.str();
			throw SQLiteQuerryException(error.str());
		}
	}

	void SQLiteBundleStorage::removeNodeRoutingInfo(const data::EID &eid, const int &key){
		int err;
		ibrcommon::MutexLock lock(dbMutex);
		sqlite3_bind_text(removeNodeRouting,1,eid.getString().c_str(),eid.getString().length(), SQLITE_TRANSIENT);
		sqlite3_bind_int(removeNodeRouting,2,key);
		err = sqlite3_step(removeNodeRouting);
		sqlite3_reset(removeNodeRouting);
		if(err != SQLITE_DONE){
			stringstream error;
			error << "SQLiteBundleStorage: removeNodeRoutingInfo: " << err << " errmsg: " << err;
			std::cerr << error.str();
			throw SQLiteQuerryException(error.str());
		}
	}

	void SQLiteBundleStorage::removeRoutingInfo(const int &key){
		int err;
		ibrcommon::MutexLock lock(dbMutex);
		sqlite3_bind_int(removeRouting,1,key);
		err = sqlite3_step(removeRouting);
		sqlite3_reset(removeRouting);
		if(err != SQLITE_DONE){
			stringstream error;
			error << "SQLiteBundleStorage: removeRoutingInfo: " << err << " errmsg: " << err;
			std::cerr << error.str();
			throw SQLiteQuerryException(error.str());
		}
	}

	void SQLiteBundleStorage::clearAll(){
		ibrcommon::MutexLock lock(dbMutex);
		sqlite3_step(clearBundles);
		sqlite3_reset(clearBundles);
		sqlite3_step(clearRouting);
		sqlite3_reset(clearRouting);
		sqlite3_step(clearNodeRouting);
		sqlite3_reset(clearNodeRouting);
		sqlite3_step(clearBlocks);
		sqlite3_reset(clearBlocks);
		if(SQLITE_DONE != sqlite3_step(vacuum)){
			sqlite3_reset(vacuum);
			throw SQLiteQuerryException("SQLiteBundleStore: clear(): vacuum failed.");
		}
		sqlite3_reset(vacuum);
		//Delete Folder SQL_TABLE_BLOCK containing Blocks
		{
			stringstream rm,mkdir;
			rm << "rm -r " << dbPath << "/" << SQL_TABLE_BLOCK;
			mkdir << "mkdir "<< dbPath << "/" << SQL_TABLE_BLOCK;
			system(rm.str().c_str());
			system(mkdir.str().c_str());
		}
		nextExpiredTime = 0;
	}

	void SQLiteBundleStorage::clear(){
		ibrcommon::MutexLock lock(dbMutex);
		sqlite3_step(clearBundles);
		sqlite3_reset(clearBundles);
		sqlite3_step(clearBlocks);
		sqlite3_reset(clearBlocks);
		if(SQLITE_DONE != sqlite3_step(vacuum)){
			sqlite3_reset(vacuum);
			throw SQLiteQuerryException("SQLiteBundleStore: clear(): vacuum failed.");
		}
		sqlite3_reset(vacuum);
		//Delete Folder SQL_TABLE_BLOCK containing Blocks
		{
			stringstream rm,mkdir;
			rm << "rm -r " << dbPath << "/" << SQL_TABLE_BLOCK;
			mkdir << "mkdir "<< dbPath << "/" << SQL_TABLE_BLOCK;
			system(rm.str().c_str());
			system(mkdir.str().c_str());
		}
		nextExpiredTime = 0;
	}



	bool SQLiteBundleStorage::empty(){
		ibrcommon::MutexLock lock(dbMutex);
		if (SQLITE_DONE == sqlite3_step(getROWID)){
			sqlite3_reset(getROWID);
			return true;
		}
		else{
			sqlite3_reset(getROWID);
			return false;
		}
	}

	unsigned int SQLiteBundleStorage::count(){
		ibrcommon::MutexLock lock(dbMutex);
		int rows = 0,err;
		err = sqlite3_step(countEntries);
		if(err == SQLITE_ROW){
			rows = sqlite3_column_int(countEntries,0);
		}
		sqlite3_reset(countEntries);
		if (err != SQLITE_ROW){
			stringstream error;
			error << "SQLiteBundleStorage: count: failure " << err << " " << sqlite3_errmsg(database);
			std::cerr << error.str();
			throw SQLiteQuerryException(error.str());
		}
		return rows;
	}

	int SQLiteBundleStorage::occupiedSpace(){
		int size(0);
		DIR *dir;
		struct dirent *directory;
		struct stat aktFile;
		dir = opendir((dbPath+"/"+_tables[SQL_TABLE_BLOCK]).c_str());
		if(dir == NULL){
			cerr << "occupiedSpace: unable to open Directory " << dbPath+"/"+ _tables[SQL_TABLE_BLOCK];
			return -1;
		}
		directory = readdir(dir);
		if(dir == NULL){
			cerr << "occupiedSpace: unable to read Directory " << dbPath+"/"+ _tables[SQL_TABLE_BLOCK];
			return -1;
		}
		while (directory != 0){
			stringstream filename;
			if(strcmp(directory->d_name,".") && strcmp(directory->d_name, "..")){
				filename << dbPath << "/"<<SQL_TABLE_BLOCK<<"/"<< directory->d_name;
				stat(filename.str().c_str(),&aktFile);
				size += aktFile.st_size;
			}
			directory = readdir(dir);
		}
		closedir(dir);

		stat((dbPath+'/'+dbFile).c_str(),&aktFile);
		size += aktFile.st_size;

		closedir(dir);
		return size;
	}

 	void SQLiteBundleStorage::storeFragment(const dtn::data::Bundle &bundle){
		int err, filedescriptor(-1);
		size_t bitcounter = 0;
		bool allFragmentsReceived(true);
		size_t payloadsize, TTL, fragmentoffset;
		fstream datei;
		list<const dtn::data::Block*> blocklist;
		list<const dtn::data::Block*>:: const_iterator it;
		stringstream path;
		string sourceEID, destination, payloadfilename, filename;
		char *tmp = strdup(path.str().c_str());
		std::string bundleID = dtn::data::BundleID(bundle).toString();

		destination = bundle._destination.getString();
		sourceEID = bundle._source.getString();
		TTL = bundle._timestamp + bundle._lifetime;
		path << dbPath << "/" << SQL_TABLE_BLOCK << "/blockXXXXXX";

		const dtn::data::PayloadBlock &block = bundle.getBlock<dtn::data::PayloadBlock>();
		ibrcommon::BLOB::Reference payloadBlob = block.getBLOB();
		ibrcommon::BLOB::iostream io = payloadBlob.iostream();
		{
//			ibrcommon::MutexLock reflock(payloadBlob);
			//ToDo: Ist ein MutexLock erforderlich?
			payloadsize = io.size();
		}
		{
			ibrcommon::MutexLock lock(dbMutex);
			//Saves the blocks from the first and the last Fragment, they may contain Extentionblock
			bool last = bundle._fragmentoffset + payloadsize == bundle._appdatalength;
			bool first = bundle._fragmentoffset == 0;
			if(first || last){
				int i,blocknumber;
				blocklist = bundle.getBlocks();

				for(it = blocklist.begin(),  i = 0; it != blocklist.end(); it++,i++){
					char *temp = strdup(path.str().c_str());
					filedescriptor = mkstemp(temp);
					if (filedescriptor == -1){
						free(temp);
						throw ibrcommon::IOException("Could not create file.");
					}
					close(filedescriptor);

					datei.open(temp,ios::out|ios::binary);
					dtn::data::SeparateSerializer serializer(datei);
					serializer << (*(*it));
					datei.close();
					filename=temp;
					free(temp);

					/* Schreibt Blöcke in die DB.
					 * Die Blocknummern werden nach folgendem Schema in die DB geschrieben:
					 * 		Die Blöcke des ersten Fragments fangen bei -x an und gehen bis -1
					 * 		Die Blöcke des letzten Fragments beginnen bei 1 und gehen bis x
					 * 		Der Payloadblock wird an Stelle 0 eingefügt.
					 */
					sqlite3_bind_text(storeBlock,1,bundleID.c_str(), bundleID.length(),SQLITE_TRANSIENT);
					sqlite3_bind_int(storeBlock,2,(int)((*it)->getType()));
					sqlite3_bind_text(storeBlock,3,filename.c_str(), filename.length(),SQLITE_TRANSIENT);

					blocknumber = blocklist.size() - i;
					if(first)	{blocknumber = (blocklist.size() - i)*(-1);}

					sqlite3_bind_int(storeBlock,4,blocknumber);
					executeQuerry(storeBlock);
				}
			}

			//At first the database is checked for already received bundles and some
			//informations are read, which effect the following program flow.
			sqlite3_bind_text(getFragements,1,sourceEID.c_str(),sourceEID.length(),SQLITE_TRANSIENT);
			sqlite3_bind_int64(getFragements,2,bundle._timestamp);
			sqlite3_bind_int(getFragements,3,bundle._sequencenumber);
			err = sqlite3_step(getFragements);

			//Es ist noch kein Eintrag vorhanden: generiere Payloadfilename
			if(err == SQLITE_DONE){
				filedescriptor = mkstemp(tmp);
				if (filedescriptor == -1){
					free(tmp);
					throw ibrcommon::IOException("Could not create file.");
				}
				close(filedescriptor);
				payloadfilename = tmp;
			}
			//Es ist bereits ein eintrag vorhanden: speicher den payloadfilename
			else if (err == SQLITE_ROW){
				payloadfilename = (const char*)sqlite3_column_text(getFragements,14);				//14 = Payloadfilename
			}
			free(tmp);

			while (err == SQLITE_ROW){
				//The following codepice summs the payloadfragmentbytes, to check if all fragments were received
				//and the bundle is complete.
				fragmentoffset = sqlite3_column_int(getFragements,9);								// 9 = fragmentoffset

				if(bitcounter >= fragmentoffset){
					bitcounter += fragmentoffset - bitcounter + sqlite3_column_int(getFragements,13);	//13 = payloadlength
				}
				else if(bitcounter >= bundle._fragmentoffset){
					bitcounter += bundle._fragmentoffset - bitcounter + payloadsize;
				}
				else{
					allFragmentsReceived = false;
				}
				err = sqlite3_step(getFragements);
			}
			if(bitcounter +1 >= bundle._fragmentoffset && allFragmentsReceived) //Wenn das aktuelle fragment das größte ist, ist die Liste durchlaufen und das Frag nicht dazugezählt. Dies wird hier nachgeholt.
				bitcounter += bundle._fragmentoffset - bitcounter + payloadsize;

			sqlite3_reset(getFragements);
			if(err != SQLITE_DONE){
				stringstream error;
				error << "SQLiteBundleStorage: storeFragment: " << err << " errmsg: " << sqlite3_errmsg(database);
				std::cerr << error.str();
				throw SQLiteQuerryException(error.str());
			}

			//Save the Payload in a separate file
			datei.open(payloadfilename.c_str(),ios::in|ios::out|ios::binary);
			datei.seekp(bundle._fragmentoffset);
			{
				ibrcommon::MutexLock reflock(payloadBlob);

				size_t ret = 0;
				istream &s = (*io);
				s.seekg(0);

				while (true)
				{
					char buf;
					s.get(buf);
					if(!s.eof()){
						datei.put(buf);
						ret++;
					}
					else
						break;
				}
				datei.close();
			}

			//All fragments received
			if(bundle._appdatalength == bitcounter){
				/*
				 * 1. Payloadblock zusammensetzen
				 * 2. PayloadBlock in der BlockTable sichern
				 * 3. Primary BlockInfos in die BundleTable eintragen.
				 * 4. Einträge der Fragmenttabelle löschen
				 */

				// 1. Get the payloadblockobject
				ibrcommon::File file(payloadfilename);
				{
					ibrcommon::BLOB::Reference ref(ibrcommon::FileBLOB::create(file));
					dtn::data::PayloadBlock pb(ref);
					//Copy EID list
					if(pb.get(dtn::data::PayloadBlock::BLOCK_CONTAINS_EIDS)){
						std::list<dtn::data::EID> eidlist;
						std::list<dtn::data::EID>::iterator eidIt;
						eidlist = block.getEIDList();
						for(eidIt = eidlist.begin(); eidIt != eidlist.end(); eidIt++){
							pb.addEID(*eidIt);
						}
					}
					//Copy ProcFlags
					pb.set(dtn::data::PayloadBlock::REPLICATE_IN_EVERY_FRAGMENT, block.get(dtn::data::PayloadBlock::REPLICATE_IN_EVERY_FRAGMENT));
					pb.set(dtn::data::PayloadBlock::TRANSMIT_STATUSREPORT_IF_NOT_PROCESSED, block.get(dtn::data::PayloadBlock::TRANSMIT_STATUSREPORT_IF_NOT_PROCESSED));
					pb.set(dtn::data::PayloadBlock::DELETE_BUNDLE_IF_NOT_PROCESSED, block.get(dtn::data::PayloadBlock::DELETE_BUNDLE_IF_NOT_PROCESSED));
					pb.set(dtn::data::PayloadBlock::LAST_BLOCK, block.get(dtn::data::PayloadBlock::LAST_BLOCK));
					pb.set(dtn::data::PayloadBlock::DISCARD_IF_NOT_PROCESSED, block.get(dtn::data::PayloadBlock::DISCARD_IF_NOT_PROCESSED));
					pb.set(dtn::data::PayloadBlock::FORWARDED_WITHOUT_PROCESSED, block.get(dtn::data::PayloadBlock::FORWARDED_WITHOUT_PROCESSED));
					pb.set(dtn::data::PayloadBlock::BLOCK_CONTAINS_EIDS, block.get(dtn::data::PayloadBlock::BLOCK_CONTAINS_EIDS));

					//2. Save the PayloadBlock to a file and store the metainformation in the BlockTable
					char *temp = strdup(path.str().c_str());
					filedescriptor = mkstemp(temp);
					if (filedescriptor == -1){
						free(temp);
						throw ibrcommon::IOException("Could not create file.");
					}
					close(filedescriptor);

					datei.open(temp,ios::out|ios::binary);
					dtn::data::SeparateSerializer serializer(datei);
					serializer << (pb);
					datei.close();
					filename=temp;
					free(temp);
				}

				sqlite3_bind_text(storeBlock,1,bundleID.c_str(), bundleID.length(),SQLITE_TRANSIENT);
				sqlite3_bind_int(storeBlock,2,(int)((*it)->getType()));
				sqlite3_bind_text(storeBlock,3,filename.c_str(), filename.length(),SQLITE_TRANSIENT);
				sqlite3_bind_int(storeBlock,4,0);
				executeQuerry(storeBlock);

				//3. Delete reassembled Palyloaddata
				file.remove();

				//4. Remove Fragmententries from BundleTable
				sqlite3_bind_text(deleteBundle,1,sourceEID.c_str(), sourceEID.length(),SQLITE_TRANSIENT);
				sqlite3_bind_int64(deleteBundle,2,bundle._sequencenumber);
				sqlite3_bind_int64(deleteBundle,3,bundle._timestamp);
				executeQuerry(deleteBundle);

				//5. Write the Primary Block to the BundleTable
				size_t procFlags = bundle._procflags;
				procFlags &= ~(dtn::data::PrimaryBlock::FRAGMENT);

				sqlite3_bind_text(store_Bundle, 1, bundleID.c_str(), bundleID.length(),SQLITE_TRANSIENT);
				sqlite3_bind_text(store_Bundle, 2,sourceEID.c_str(), sourceEID.length(),SQLITE_TRANSIENT);
				sqlite3_bind_text(store_Bundle, 3,destination.c_str(), destination.length(),SQLITE_TRANSIENT);
				sqlite3_bind_text(store_Bundle, 4,bundle._reportto.getString().c_str(), bundle._reportto.getString().length(),SQLITE_TRANSIENT);
				sqlite3_bind_text(store_Bundle, 5,bundle._custodian.getString().c_str(), bundle._custodian.getString().length(),SQLITE_TRANSIENT);
				sqlite3_bind_int(store_Bundle,6,procFlags);
				sqlite3_bind_int64(store_Bundle,7,bundle._timestamp);
				sqlite3_bind_int(store_Bundle,8,bundle._sequencenumber);
				sqlite3_bind_int64(store_Bundle,9,bundle._lifetime);
				sqlite3_bind_int64(store_Bundle,10,bundle._fragmentoffset);
				sqlite3_bind_int64(store_Bundle,11,bundle._appdatalength);
				sqlite3_bind_int64(store_Bundle,12,TTL);
				sqlite3_bind_int(store_Bundle,13,NULL);
				sqlite3_bind_int(store_Bundle,14,NULL);
				sqlite3_bind_text(store_Bundle,15,NULL,0,SQLITE_TRANSIENT);
				executeQuerry(store_Bundle);
			}

			//There are some fragments missing
			else{
				sqlite3_bind_text(store_Bundle, 1, bundleID.c_str(), bundleID.length(),SQLITE_TRANSIENT);
				sqlite3_bind_text(store_Bundle, 2,sourceEID.c_str(), sourceEID.length(),SQLITE_TRANSIENT);
				sqlite3_bind_text(store_Bundle, 3,destination.c_str(), destination.length(),SQLITE_TRANSIENT);
				sqlite3_bind_text(store_Bundle, 4,bundle._reportto.getString().c_str(), bundle._reportto.getString().length(),SQLITE_TRANSIENT);
				sqlite3_bind_text(store_Bundle, 5,bundle._custodian.getString().c_str(), bundle._custodian.getString().length(),SQLITE_TRANSIENT);
				sqlite3_bind_int(store_Bundle,6,bundle._procflags);
				sqlite3_bind_int64(store_Bundle,7,bundle._timestamp);
				sqlite3_bind_int(store_Bundle,8,bundle._sequencenumber);
				sqlite3_bind_int64(store_Bundle,9,bundle._lifetime);
				sqlite3_bind_int64(store_Bundle,10,bundle._fragmentoffset);
				sqlite3_bind_int64(store_Bundle,11,bundle._appdatalength);
				sqlite3_bind_int64(store_Bundle,12,TTL);
				sqlite3_bind_int(store_Bundle,13,NULL);
				sqlite3_bind_int(store_Bundle,14,payloadsize);
				sqlite3_bind_text(store_Bundle,15,payloadfilename.c_str(),payloadfilename.length(),SQLITE_TRANSIENT);
				executeQuerry(store_Bundle);
			}
		}//dbMutex left
	}

	void SQLiteBundleStorage::raiseEvent(const Event *evt)
	{
		try {
			const TimeEvent &time = dynamic_cast<const TimeEvent&>(*evt);
			
			ibrcommon::MutexLock lock = ibrcommon::MutexLock(time_change);
			if (time.getAction() == dtn::core::TIME_SECOND_TICK)
			{
				actual_time = time.getTimestamp();
				/*
				 * Only if the actual time is bigger or equal than the time when the next bundle expires, deleteexpired is called.
				 */
				if(actual_time >= nextExpiredTime){
					deleteexpired();
				}
			}
		} catch (const std::bad_cast&) { }
		
		try {
			const GlobalEvent &global = dynamic_cast<const GlobalEvent&>(*evt);
			
			if (global.getAction() == dtn::core::GlobalEvent::GLOBAL_SHUTDOWN)
			{
				{
					ibrcommon::MutexLock lock(dbMutex);
					global_shutdown = true;
				}
				timeeventConditional.signal();
			}
			else if(global.getAction() == GlobalEvent::GLOBAL_IDLE){
				idle = true;
				timeeventConditional.signal();
			}
			else if(global.getAction() == GlobalEvent::GLOBAL_BUSY) {
				idle = false;
			}
		} catch (const std::bad_cast&) { }
	}

	void SQLiteBundleStorage::componentRun()
	{
		ibrcommon::MutexLock l(timeeventConditional);

		while (!global_shutdown)
		{
			idle_work();
			timeeventConditional.wait();
		}
	}

	void SQLiteBundleStorage::componentUp() {};
	void SQLiteBundleStorage::componentDown() {};

	bool SQLiteBundleStorage::__cancellation()
	{
		ibrcommon::MutexLock l(timeeventConditional);
		global_shutdown = true;
		timeeventConditional.signal(true);

		// return true, to signal that no further cancel (the hardway) is needed
		return true;
	}

	void SQLiteBundleStorage::deleteexpired(){
		/*
		 * Performanceverbesserung: Damit die Abfragen nicht jede Sekunde ausgeführt werden müssen, speichert man den Zeitpunkt an dem
		 * das nächste Bündel gelöscht werden soll in eine Variable und fürhrt deleteexpired erst dann aus wenn ein Bündel abgelaufen ist.
		 * Nach dem Löschen wird die DB durchsucht und der nächste Ablaufzeitpunkt wird in die Variable gesetzt.
		 */
		int err;
		string file_Name, source;
		size_t sequence, offset, timestamp;
		data::BundleID ID;
		data::EID sourceID;
		list<data::BundleID> eventlist;
		list<data::BundleID>::iterator it;
		{
			//find expired Bundle
			ibrcommon::MutexLock lock(dbMutex);
			sqlite3_bind_int64(getBundleTTL,1, actual_time);
			err = sqlite3_step(getBundleTTL);
			while (err == SQLITE_ROW){
				source	  	 = (const char*)sqlite3_column_text(getBundleTTL,0);
				timestamp 	 = sqlite3_column_int64(getBundleTTL,1);
				sequence  	 = sqlite3_column_int64(getBundleTTL,2);
				sourceID = data::EID(source);
				ID = data::BundleID(sourceID, timestamp, sequence);
				eventlist.push_back(ID);
				err = sqlite3_step(getBundleTTL);
			}
			sqlite3_reset(getBundleTTL);
			if(err != SQLITE_DONE){
				stringstream error;
				error << "SQLiteBundleStorage: deleteexpired() failure: "<< err << " " << sqlite3_errmsg(database);
				std::cerr << error.str();
				throw SQLiteQuerryException(error.str());
			}

			//Put Blockfilnames of expired Bundles into the deleteList
			for(it = eventlist.begin(); it != eventlist.end(); it++){
				sqlite3_bind_text(getBlocksByID,1,(*it).toString().c_str(),(*it).toString().length(),SQLITE_TRANSIENT);
				err = sqlite3_step(getBlocksByID);
				//put all Blocks from the actual Bundle to the deletelist
				while(err == SQLITE_ROW){
					file_Name = (const char*)sqlite3_column_text(getBlocksByID,0);
					deleteList.push_back(file_Name);
					err = sqlite3_step(getBlocksByID);
				}
				sqlite3_reset(getBlocksByID);

				//if everything went normal err should be SQLITE_DONE
				if(err != SQLITE_DONE){
					stringstream error;
					error << "SQLiteBundleStorage deleteexpired() failure: while getting Blocks " << err << " "<< sqlite3_errmsg(database);
					std::cerr << error.str();
					throw SQLiteQuerryException(error.str());
				}
			}

			//remove Bundles from the database
			sqlite3_bind_int64(deleteBundleTTL,1, actual_time);
			executeQuerry(deleteBundleTTL);
		}

		//update deprecated timer
		updateexpiredTime();

		//raising events
		for(it = eventlist.begin(); it != eventlist.end(); it++){
			BundleExpiredEvent::raise((*it));
		}
	}

	void SQLiteBundleStorage::updateexpiredTime(){
		int err;
		size_t time;
		err = sqlite3_step(getnextExpiredBundle);
		if (err == SQLITE_ROW)
			time = sqlite3_column_int64(getnextExpiredBundle,0);
		sqlite3_reset(getnextExpiredBundle);

		nextExpiredTime = time;
	}

	int SQLiteBundleStorage::storeBlocks(const data::Bundle &bundle){
		const list<const data::Block*> blocklist(bundle.getBlocks());
		list<const data::Block*>::const_iterator it;
		int filedescriptor(-1), blocktyp, err, blocknumber(1), storedBytes(0);
		fstream filestream;
		stringstream path;
		path << dbPath << "/" << SQL_TABLE_BLOCK << "/blockXXXXXX";
		data::BundleID ID(bundle);

		for(it = blocklist.begin() ;it != blocklist.end(); it++){
			filedescriptor = -1;
			blocktyp = (int)(*it)->getType();
			char *blockfilename;

			//create file
			blockfilename = strdup(path.str().c_str());
			filedescriptor = mkstemp(blockfilename);
			if (filedescriptor == -1)
				throw ibrcommon::IOException("Could not create file.");
			close(filedescriptor);

			filestream.open(blockfilename,ios_base::out|ios::binary);
			dtn::data::SeparateSerializer serializer(filestream);
			serializer << (*(*it));
			storedBytes += serializer.getLength(*(*it));
			filestream.close();

			//put metadata into the storage
			sqlite3_bind_text(storeBlock,1,ID.toString().c_str(), ID.toString().length(),SQLITE_TRANSIENT);
			sqlite3_bind_int(storeBlock,2,blocktyp);
			sqlite3_bind_text(storeBlock,3,blockfilename, strlen(blockfilename),SQLITE_TRANSIENT);
			sqlite3_bind_int(storeBlock,4,blocknumber);
			executeQuerry(storeBlock);

			//increment blocknumber
			blocknumber++;
			free(blockfilename);
		}

		return storedBytes;
	}

	int SQLiteBundleStorage::readBlocks(data::Bundle &bundle, const string &bundleID){
		int err = 0;
		string file;
		list<string> filenames;

		fstream filestream;

		//Get the containing blocks and their filenames
		err = sqlite3_bind_text(getBlocksByID,1,bundleID.c_str(),bundleID.length(),SQLITE_TRANSIENT);
		if(err != SQLITE_OK)
			cerr << "bind error"<<endl;
		err = sqlite3_step(getBlocksByID);
		if(err == SQLITE_DONE){
			cerr << "readBlocks: no Blocks found for: " << bundleID <<endl;
		}
		while(err == SQLITE_ROW){
			file = (const char*)sqlite3_column_text(getBlocksByID,0);
			filenames.push_back(file);
			err = sqlite3_step(getBlocksByID);
		}
		if(err != SQLITE_DONE){
			stringstream error;
			error << "SQLiteBundleStorage: readBlocks() failure: "<< err << " " << sqlite3_errmsg(database);
			std::cerr << error.str();
			throw SQLiteQuerryException(error.str());
		}
		sqlite3_reset(getBlocksByID);

		//De-serialize the Blocks
		for(std::list<string>::const_iterator it = filenames.begin(); it != filenames.end(); it++){
			std::ifstream is((*it).c_str(), ios::binary);
			dtn::data::SeparateDeserializer deserializer(is, bundle);
			deserializer.readBlock();
			is.close();
		}
		return 0;
	}

	void SQLiteBundleStorage::idle_work(){	//ToDo Auf DeleteList wird gleichzeitig zugegriffen.
		int err;
		list<string>::iterator it;
		it = deleteList.begin();
		while(it != deleteList.end() || idle){
			::remove((*it).c_str());
		}
		/*
		 * When an object (table, index, trigger, or view) is dropped from the database, it leaves behind empty space.
		 * This empty space will be reused the next time new information is added to the database. But in the meantime,
		 * the database file might be larger than strictly necessary. Also, frequent inserts, updates, and deletes can
		 * cause the information in the database to become fragmented - scrattered out all across the database file rather
		 * than clustered together in one place.
		 * The VACUUM command cleans the main database. This eliminates free pages, aligns table data to be contiguous,
		 * and otherwise cleans up the database file structure.
		 */
		if(idle){
			err = sqlite3_step(vacuum);
			sqlite3_reset(vacuum);
		}
	}

	const std::string SQLiteBundleStorage::getName() const
	{
		return "SQLiteBundleStorage";
	}

	void SQLiteBundleStorage::releaseCustody(dtn::data::BundleID&)
	{
		// custody is successful transferred to another node.
		// it is safe to delete this bundle now. (depending on the routing algorithm.)
	}
}
}

/*
 * SQLiteBundleStorage.cpp
 *
 *  Created on: 09.01.2010
 *      Author: Myrtus
 */

#include "core/SQLiteBundleStorage.h"
#include "core/EventSwitch.h"
#include "core/TimeEvent.h"
#include "core/GlobalEvent.h"
#include "ibrcommon/thread/MutexLock.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/data/BundleID.h"
#include <iostream>
#include "core/BundleCore.h"
#include <list>
#include <stdio.h>

#include <vector>

namespace dtn {
namespace core {

	SQLiteBundleStorage::SQLiteBundleStorage(string dbPath ,string dbFile , int size):
			dbPath(dbPath), dbFile(dbFile), dbSize(size), _BundleTable("Bundles"), global_shutdown(false), _FragmentTable("Fragments"){
		int err,filename , err2;
		list<int> inconsistentList;
		list<int>::iterator it;

		//open the database
		err = sqlite3_open_v2((dbPath+"/"+dbFile).c_str(),&database,SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
		if (err){
			std::cerr << "Can't open database: " << sqlite3_errmsg(database);
			sqlite3_close(database);
			throw "Unable to open SQLite Database";
		}

		//create BundleTable
		sqlite3_stmt *createBundleTable = prepareStatement("create table if not exists "+_BundleTable+" (Filename INTEGER PRIMARY KEY ASC,BundleID text, Destination text, TTL int, Priority int);");
		err = sqlite3_step(createBundleTable);
		if(err != SQLITE_DONE){
			std::cerr << "SQLiteBundleStorage: Constructorfailure: createTable failed " << err;
		}
		sqlite3_finalize(createBundleTable);

		//create FragemntTable
		sqlite3_stmt *createFragmentTable = prepareStatement("create table if not exists "+_FragmentTable+" (Filename INTEGER PRIMARY KEY ASC, Source text, Timestamp int, Sequencenumber int, Destination text, TTL int, Priority int, FragementationOffset int, PayloadLength int, file int);");
		err = sqlite3_step(createFragmentTable);
		if(err != SQLITE_DONE){
			std::cerr << "SQLiteBundleStorage: Constructorfailure: createTable failed " << err;
		}
		sqlite3_finalize(createFragmentTable);

		//check if the database is consistent with the files on the HDD.
		sqlite3_stmt *conistencycheck = prepareStatement("SELECT ROWID FROM "+_BundleTable+";");
		stringstream name;
		for(int i = sqlite3_step(conistencycheck); i == SQLITE_ROW; i=sqlite3_step(conistencycheck)){
			filename = sqlite3_column_int(conistencycheck,0);
			stringstream name;
			name << dbPath << "/"<<filename;
			FILE *fpointer = fopen(name.str().c_str(),"r");
			if (fpointer == NULL){
				//ToDo: Daten Protokollieren, damit sie nach der Suche gelöscht werden können.
				inconsistentList.push_front(filename);
				cerr << "datenbank inkonsistent" << endl;
			}
			else{
				fclose(fpointer);
			}
		}
		sqlite3_finalize(conistencycheck);

		//ToDo: Konsistenzcheck der Fragmente

		//delete inconsistent database entries
		sqlite3_stmt *deleteBundle = prepareStatement("DELETE FROM "+_BundleTable+" WHERE ROWID = ?;");
		for(it = inconsistentList.begin(); it != inconsistentList.end(); it++){
			//ToDo: Infrmiere das Routing, das dass Bundle XY Gelöscht wurde.
			sqlite3_bind_int(deleteBundle,1,(*it));
			err = sqlite3_step(deleteBundle);
			if ( err != SQLITE_DONE )
			{
				std::cerr << "SQLiteBundlestorage: failure in prepareStatement: " << err << std::endl;
			}
			sqlite3_reset(deleteBundle);
		}
		sqlite3_finalize(deleteBundle);

		//prepare the sql statements
		getTTL 					= prepareStatement("SELECT TTL FROM "+_BundleTable+" ASC;");
		//TODO  get bundle by destination anpassen
		getBundleByDestination	= prepareStatement("SELECT ROWID FROM "+_BundleTable+" WHERE Destination = ? ORDER BY TTL ASC;");
		getBundleByID 			= prepareStatement("SELECT ROWID FROM "+_BundleTable+" WHERE BundleID = ?;");
		getFragements			= prepareStatement("SELECT * FROM "+_FragmentTable+" WHERE Source = ? AND Timestamp = ? ORDER BY FragementationOffset ASC;");
		store_Bundle 			= prepareStatement("INSERT INTO "+_BundleTable+" (BundleID, Destination, TTL, Priority) VALUES (?,?,?,?);");
		store_Fragment			= prepareStatement("INSERT INTO "+_FragmentTable+" (Source, Timestamp, Sequencenumber, Destination, TTL, Priority, FragementationOffset ,PayloadLength,file) VALUES (?,?,?,?,?,?,?,?,?);");
		clearStorage 			= prepareStatement("DELETE FROM "+_BundleTable+";");
		countEntries 			= prepareStatement("SELECT Count(ROWID) From "+_BundleTable+";");
		vacuum 					= prepareStatement("vacuum;");
		getROWID 				= prepareStatement("select ROWID from "+_BundleTable+";");
		removeBundle 			= prepareStatement("Delete From "+_BundleTable+" Where BundleID = ?;");
		removeFragments			= prepareStatement("DELETE FROM "+_FragmentTable+" WHERE Source = ? AND Timestamp = ?;");

		//register Events
		EventSwitch::registerEventReceiver(TimeEvent::className, this);
		EventSwitch::registerEventReceiver(GlobalEvent::className, this);
	}

	SQLiteBundleStorage::~SQLiteBundleStorage(){

		ibrcommon::MutexLock lock = ibrcommon::MutexLock(dbMutex);

		//finalize Statements
		sqlite3_finalize(getTTL);
		sqlite3_finalize(getBundleByDestination);
		sqlite3_finalize(getBundleByID);
		sqlite3_finalize(store_Bundle);
		sqlite3_finalize(clearStorage);
		sqlite3_finalize(countEntries);
		sqlite3_finalize(vacuum);
		sqlite3_finalize(getROWID);
		sqlite3_finalize(removeBundle);

		//close Databaseconnection
		sqlite3_close(database);

		//unregister Events
		EventSwitch::unregisterEventReceiver(TimeEvent::className, this);
		EventSwitch::unregisterEventReceiver(GlobalEvent::className, this);
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
		int err, filename;
		data::Bundle bundle;
		stringstream completefilename;
		fstream datei;

		//parameterize querry
		string ID = id.toString();

		{
			ibrcommon::MutexLock lock = ibrcommon::MutexLock(dbMutex);
			sqlite3_bind_text(getBundleByID, 1, ID.c_str(), ID.length(),SQLITE_TRANSIENT);

			//executing querry
			err = sqlite3_step(getBundleByID);

			//If an error occurs
			if(err != SQLITE_ROW){
				std::cerr << "SQLiteBundleStorage: No Bundle found with BundleID: " << id.toString() <<endl;
			}
			filename = sqlite3_column_int(getBundleByID, 0);

			err = sqlite3_step(getBundleByID);
			if (err != SQLITE_DONE){
				sqlite3_reset(getBundleByID);
				throw "SQLiteBundleStorage: Database contains two or more Bundle with the same BundleID, the requested Bundle is maybe a Fragment";
			}

			completefilename << dbPath << "/" << filename;
			datei.open((completefilename.str()).c_str(), ios::in|ios::binary);
			datei >> bundle;
			datei.close();

			//reset the statement
			sqlite3_reset(getBundleByID);
		}

		return bundle;
	}

	dtn::data::Bundle SQLiteBundleStorage::get(const dtn::data::EID &eid){
		dtn::data::Bundle bundel;
		set<dtn::data::EID>::iterator iter;
		while(true){
			ibrcommon::MutexLock lock = ibrcommon::MutexLock(dbMutex);
			//checks if global shutdown is activated
			if(!global_shutdown){
				//checks if a unblock wish is occured
				iter = unblockEID.find(eid);
				if(iter == unblockEID.end()){
					if(getBundle(eid,bundel)){
						//ToDo: Nachricht an das Routing senden, dass das Bundle erfolgreich zugestellt wurde.
						return bundel;
					}
					else{
						//lock.~MutexLock();
						dbMutex.wait();
					}
				}
				else{
					 unblockEID.erase(eid);
					 break;
				}
			}
			else {
				break;
			}
		}
		throw dtn::exceptions::NoBundleFoundException();
	}

	bool SQLiteBundleStorage::getBundle(const dtn::data::EID &eid, dtn::data::Bundle &bundle){
		int err,filename;
		stringstream completefilename;
		string ID = eid.getString();
		//ToDo: Wenn das gewisse Flags gesetzt sind wird muss ein Custody delivered report gesendet werden.
		//		Hierfür muss eine Nachricht an das Routing geschickt werden.

		//parameterize querry
		err = sqlite3_bind_text(getBundleByDestination, 1, ID.c_str(), ID.length(),SQLITE_TRANSIENT);

		//executing querry
		err = sqlite3_step(getBundleByDestination);

		//If an error occurs
		if(err != SQLITE_ROW){
				sqlite3_reset(getBundleByDestination);
				return false;
		}
		filename = sqlite3_column_int(getBundleByDestination, 0);

		fstream datei;
		completefilename << dbPath << "/" << filename;
		datei.open((completefilename.str()).c_str(), ios::in|ios::binary);
		datei >> bundle;
		datei.close();

		//reset the statement
		sqlite3_reset(getBundleByID);

		return true;
	}

	void SQLiteBundleStorage::store(const dtn::data::Bundle &bundle){
//		bool local = bundle._destination.getNodeEID() == BundleCore::local.getNodeEID();
//		if(bundle._procflags & dtn::data::Bundle::FRAGMENT && local){
//			// ToDo: evt eine abfrage ob genügend platz für das Gesamte Bundle vorhanden ist. if(bundle._appdatalength <= )
//			storeFragment(bundle);
//			return;
//		}
		int err, TTL, priority;
		stringstream  stream_bundleid, completefilename;
		string bundlestring, destination;

		//calculation of priority
		priority = 2 * (bundle._procflags & dtn::data::Bundle::PRIORITY_BIT2) + (bundle._procflags & dtn::data::Bundle::PRIORITY_BIT1);

		destination = bundle._destination.getString();
		stream_bundleid  << "[" << bundle._timestamp << "." << bundle._sequencenumber << "] " << bundle._source.getString();
		TTL = bundle._timestamp + bundle._lifetime;

		{
			ibrcommon::MutexLock lock = ibrcommon::MutexLock(dbMutex);
			sqlite3_bind_text(store_Bundle, 1, stream_bundleid.str().c_str(), stream_bundleid.str().length(),SQLITE_TRANSIENT);
			sqlite3_bind_text(store_Bundle, 2,destination.c_str(), destination.length(),SQLITE_TRANSIENT);
			sqlite3_bind_int(store_Bundle,3,TTL);
			sqlite3_bind_int(store_Bundle,4,priority);
			err = sqlite3_step(store_Bundle);
			if(err != SQLITE_DONE){
				std::cerr << "SQLiteBundleStorage: store() failure: "<< err << " " <<  sqlite3_errmsg(database) <<endl;
			}
			sqlite3_reset(store_Bundle);
			//stores the bundle to a file
			int filename = sqlite3_last_insert_rowid(database);
			fstream datei;
			completefilename << dbPath << "/" << filename;
			datei.open((completefilename.str()).c_str(), ios::out|ios::binary);
			datei << bundle;
			datei.close();
		}
		//wake up thread who are waiting to get a bundle with a specific destinationEID
		dbMutex.signal(true);
	}

	void SQLiteBundleStorage::remove(const dtn::data::BundleID &id){
		int err, filename;
		stringstream file;
		//enter Lock
		{
			ibrcommon::MutexLock lock = ibrcommon::MutexLock(dbMutex);
			sqlite3_bind_text(getBundleByID, 1, id.toString().c_str(), id.toString().length(),SQLITE_TRANSIENT);
			err = sqlite3_step(getBundleByID);
			if(err != SQLITE_OK){
				std::cerr << "SQLiteBundleStorage: failure while getting filename: " << id.toString() << " errmsg: " << err <<endl;
			}
			filename = sqlite3_column_int(getBundleByID,0);
			file << dbPath << "/" << filename;
			sqlite3_reset(removeBundle);
			err = ::remove(file.str().c_str());
			if(err != SQLITE_OK){
				std::cerr << "SQLiteBundleStorage: remove():Datei konnte nicht gelöscht werden " << " errmsg: " << err <<endl;
			}

			sqlite3_bind_text(removeBundle, 1, id.toString().c_str(), id.toString().length(),SQLITE_TRANSIENT);
			err = sqlite3_step(removeBundle);
			if(err != SQLITE_OK){
				std::cerr << "SQLiteBundleStorage: failure while removing: " << id.toString() << " errmsg: " << err <<endl;
			}
			sqlite3_reset(removeBundle);

			/*
			 * When an object (table, index, trigger, or view) is dropped from the database, it leaves behind empty space.
			 * This empty space will be reused the next time new information is added to the database. But in the meantime,
			 * the database file might be larger than strictly necessary. Also, frequent inserts, updates, and deletes can
			 * cause the information in the database to become fragmented - scrattered out all across the database file rather
			 * than clustered together in one place.
			 * The VACUUM command cleans the main database. This eliminates free pages, aligns table data to be contiguous,
			 * and otherwise cleans up the database file structure.
			 */
			err = sqlite3_step(vacuum);
			sqlite3_reset(vacuum);
		}
	}

	void SQLiteBundleStorage::unblock(const dtn::data::EID &eid){
		ibrcommon::MutexLock lock = ibrcommon::MutexLock(dbMutex);
		unblockEID.insert(eid);
		dbMutex.signal(true);
	}

	void SQLiteBundleStorage::clear(){
		char *err;
		{
			ibrcommon::MutexLock lock = ibrcommon::MutexLock(dbMutex);
			sqlite3_step(clearStorage);
			sqlite3_reset(clearStorage);
			if(SQLITE_OK != sqlite3_step(vacuum)){
				std::cerr << "SQLiteBundleStorage: failure while processing vacuum.";
			}
			sqlite3_reset(vacuum);
		}
	}

	bool SQLiteBundleStorage::empty(){
		ibrcommon::MutexLock lock = ibrcommon::MutexLock(dbMutex);
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
		ibrcommon::MutexLock lock = ibrcommon::MutexLock(dbMutex);
		int rows = 0,err;
		err = sqlite3_step(countEntries);
		rows = sqlite3_column_int(countEntries,0);
		sqlite3_reset(countEntries);
		return rows;
	}

//	int SQLiteBundleStorage::availableSpace(){
//
//	}

	void SQLiteBundleStorage::storeFragment(const dtn::data::Bundle &bundle){
		int payloadsize, TTL, priority, err, filename;
		stringstream completefilename;
		string destination, sourceEID;
		list<dtn::data::Block*> blocklist;
		list<dtn::data::Block*>::iterator it;

		destination = bundle._destination.getString();
		sourceEID = bundle._source.getString();
		TTL = bundle._timestamp + bundle._lifetime;

	//calculation of priority
		priority = 2 * (bundle._procflags & dtn::data::Bundle::PRIORITY_BIT2) + (bundle._procflags & dtn::data::Bundle::PRIORITY_BIT1);

	//calculation of the int header: count bytes till first payloadbyte
//		int header;
//		blocklist = bundle.getBlocks();
//		it = blocklist.begin();
//		while ((*it)->getType() != PayloadBlock::BLOCK_TYPE){
//			header += (*it)->getSize();  //gibt getSize() die größe in Bit oder Byte wieder?
//			it++;
//		}

	//the iterator now points to the payloadblock. Here we get the length of the payloaddata for later use.
		ibrcommon::BLOB::Reference payloadBlob = (*it)->getBLOB();
		payloadsize = payloadBlob.getSize();


		ibrcommon::MutexLock lock = ibrcommon::MutexLock(dbMutex);
	//guck ob schon andere Fragmente vorhanden sind
		sqlite3_bind_text(getFragements,1, sourceEID.c_str(), sourceEID.length(),SQLITE_TRANSIENT);
		sqlite3_bind_int(getFragements,2,bundle._timestamp);
		err = sqlite3_step(getFragements);
		if(err == SQLITE_ERROR){
			std::cerr << "SQLiteBundleStorage: storeFragment() failure: "<< err << " " << sqlite3_errmsg(database) << endl;
		}
		if(err != SQLITE_ROW){
			//man selbst ist die ROWID welche den Dateinamen kennzeichnet
			//1) Metadaten speicher
			//2) Dateinmanen rausfindn
			//3) checken ob man offset 0 ist
			//		ja: bundle in datei speichern
			//		nein: header weglassen beim dateispeichern
			sqlite3_reset(getFragements);

			//1) Metadateien speichern
			sqlite3_bind_text(store_Fragment, 1, sourceEID.c_str(), sourceEID.length(),SQLITE_TRANSIENT);
			sqlite3_bind_int(store_Fragment,2,bundle._timestamp);
			sqlite3_bind_int(store_Fragment,3,bundle._sequencenumber);
			sqlite3_bind_text(store_Fragment, 4,destination.c_str(), destination.length(),SQLITE_TRANSIENT);
			sqlite3_bind_int(store_Fragment,5,TTL);
			sqlite3_bind_int(store_Fragment,6,priority);
			sqlite3_bind_int(store_Fragment,7,bundle._fragmentoffset);
			sqlite3_bind_int(store_Fragment,8,payloadsize);
			//bundle._fragmentoffset  == 0 ? 	sqlite3_bind_int(store_Fragment,9,1) : sqlite3_bind_int(store_Fragment,9,header);
			err = sqlite3_step(store_Fragment);
			if(err != SQLITE_OK){
				std::cerr << "SQLiteBundleStorage: storeFragment() failure: "<< err << " " << sqlite3_errmsg(database) << endl;
			}
			sqlite3_reset(store_Fragment);

			//2) Dateinamen rausfinden
			int filename = sqlite3_last_insert_rowid(database);

			if(bundle._fragmentoffset == 0){
				fstream datei;
				completefilename << dbPath << "/Fragment/" << filename;
				datei.open((completefilename.str()).c_str(), ios::binary);
				datei << bundle;
				datei.close();
			}
			else{

			}
		}

		else{
			//1)dateinamen bestimmen / check ob das Fragment vollständig ist
			//2)check ob man offset 0 ist
			//3)Bundle in datei speichern
			//		offset = 0: bundle in datei speichern
			//		offset != 0: header weglassen beim dateispeichern
			//4)Metadaten speicher

		//1)dateinamen bestimmen / check ob da Fragment vollständig ist
			//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			// Die Querry ist bereits ausgeführt und der Zeiger steht auf pos 0
			//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			int fragmentoffset , bitcounter = 0;
			while (err != SQLITE_DONE || err == SQLITE_ERROR){
				fragmentoffset = sqlite3_column_int(getFragements,7);
				//if file is set to 1, filename equals ROWID of this database entry
				if(sqlite3_column_int(getFragements,9) == 1){
					filename = sqlite3_column_int(getFragements,0);
				}
				//Now its getting boring. the following codepice summs the payloadfragmentbytes, to check if all fragments were received
				//and the bundle is complete.
				if(bitcounter >= fragmentoffset){
					bitcounter = fragmentoffset - bitcounter + sqlite3_column_int(getFragements,8);
				}
				err = sqlite3_step(getFragements);
			}
			//bitcounter noch das aktuell verarbeitete Bündel erhöhen
			fragmentoffset = bundle._fragmentoffset;
			if(bitcounter >= fragmentoffset){
				bitcounter = fragmentoffset - bitcounter + payloadsize;
			}
			sqlite3_reset(getFragements);

			//2) Offset = 0
			if(bundle._fragmentoffset == 0){
			//3) Bundle in datei speichern
					fstream datei;
					completefilename << dbPath << "/Fragment/" << filename;
					datei.open((completefilename.str()).c_str(), ios::binary);
					datei << bundle;
					datei.close();
				}
			//2) Offset != 0
			else{
			//3) Bundle in Datei speichern
				fstream datei, tmpdatei;
				completefilename << dbPath << "/Fragment/" << filename;
				tmpdatei.open((completefilename.str()).c_str(), ios::binary);
				tmpdatei << bundle;
				//copy the tmpfile without the headers to the file which should contain the complete bundle
			}

		//4) Metadaten speichern
			//if all fragments were received
			if(bitcounter == bundle._appdatalength){
//				//ToDo: Bits im Header ändern und fragmentfelder löschen
//				sqlite3_bind_text(store_Bundle, 1, stream_bundleid.str().c_str(), stream_bundleid.str().length(),SQLITE_TRANSIENT);
//				sqlite3_bind_text(store_Bundle, 2,destination.c_str(), destination.length(),SQLITE_TRANSIENT);
//				sqlite3_bind_int(store_Bundle,3,TTL);
//				sqlite3_bind_int(store_Bundle,4,priority);
//				err = sqlite3_step(store_Bundle);
//				if(err != SQLITE_OK){
//					std::cerr << "SQLiteBundleStorage: store() failure: "<< err << " " << sqlite3_errmsg(database) <<endl;
//				}
//				sqlite3_reset(store_Bundle);
//				int filename = sqlite3_last_insert_rowid(database);
//				//ToDo Kopieren der Datei
//				sqlite3_bind_text(removeFragments,1, sourceEID.c_str(), sourceEID.length(),SQLITE_TRANSIENT);
//				sqlite3_bind_int(removeFragments,bundle._timestamp);
//				err = sqlite3_step(removeFragments);
//				if(err != SQLITE_OK){
//					std::cerr << "SQLiteBundleStorage: store() failure: "<< err << " " << sqlite3_errmsg(database) <<endl;
//				}
//				sqlite3_reset(removeFragments);
			}
			else{
				sqlite3_bind_text(store_Fragment, 1, sourceEID.c_str(), sourceEID.length(),SQLITE_TRANSIENT);
				sqlite3_bind_int(store_Fragment,2,bundle._timestamp);
				sqlite3_bind_int(store_Fragment,3,bundle._sequencenumber);
				sqlite3_bind_text(store_Fragment, 4,destination.c_str(), destination.length(),SQLITE_TRANSIENT);
				sqlite3_bind_int(store_Fragment,5,TTL);
				sqlite3_bind_int(store_Fragment,6,priority);
				sqlite3_bind_int(store_Fragment,7,bundle._fragmentoffset);
				sqlite3_bind_int(store_Fragment,8,payloadsize);
				sqlite3_bind_int(store_Fragment,9,0);
				err = sqlite3_step(store_Fragment);
				if(err != SQLITE_OK){
					std::cerr << "SQLiteBundleStorage: storeFragment() failure: "<< err << " " << sqlite3_errmsg(database) << endl;
				}
				sqlite3_reset(store_Fragment);
			}
		}
	}

	void SQLiteBundleStorage::raiseEvent(const Event *evt){
		const TimeEvent *time = dynamic_cast<const TimeEvent*>(evt);
		const GlobalEvent *global = dynamic_cast<const GlobalEvent*>(evt);

		if (global != NULL)
		{
			if (global->getAction() == dtn::core::GlobalEvent::GLOBAL_SHUTDOWN)
			{
				{
				ibrcommon::MutexLock lock = ibrcommon::MutexLock(dbMutex);
				global_shutdown = true;
				}
				timeeventConditional.signal();
				dbMutex.signal(true);
			}
		}

		if (time != NULL)
		{
			//TODO Reaktion auf Timeevents
			if (time->getAction() == dtn::core::TIME_SECOND_TICK)
			{
//				_timecond.go();
			}
		}
	}

	void SQLiteBundleStorage::run(void){
		while(!global_shutdown){
			deleteexpired();
			timeeventConditional.wait();
		}
	}

	void SQLiteBundleStorage::deleteexpired(){
		/*
		 * 1) Suche abgelaufene Bündel
		 * 2) Wenn daten vorhanden kopiere die Tabelle
		 * 3) Lösche die gefundenen Daten
		 */
	}
}
}

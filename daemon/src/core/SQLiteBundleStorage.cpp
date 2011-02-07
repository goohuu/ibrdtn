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
#include "core/SQLiteConfigure.h"
#include "core/BundleEvent.h"
#include "core/BundleExpiredEvent.h"

#include <ibrdtn/data/PayloadBlock.h>
#include <ibrdtn/data/Serializer.h>
#include <ibrdtn/data/Bundle.h>
#include <ibrdtn/data/BundleID.h>

#include <ibrcommon/thread/MutexLock.h>
#include <ibrcommon/data/BLOB.h>
#include <ibrcommon/AutoDelete.h>
#include <ibrcommon/Logger.h>

namespace dtn
{
	namespace core
	{
		const std::string SQLiteBundleStorage::_tables[SQL_TABLE_END] =
				{ "Bundles", "Block", "Routing", "BundleRoutingInfo", "NodeRoutingInfo" };

		const std::string SQLiteBundleStorage::_sql_queries[SQL_QUERIES_END] =
		{
			"SELECT BundleID, Source, Destination, Reportto, Custodian, ProcFlags, Timestamp, Sequencenumber, Lifetime, Fragmentoffset, Appdatalength FROM " + _tables[SQL_TABLE_BUNDLE] + " LIMIT ?,?;",
			/* "SELECT BundleID, Source, Destination, Reportto, Custodian, ProcFlags, Timestamp, Sequencenumber, Lifetime, Fragmentoffset, Appdatalength FROM "+ _tables[SQL_TABLE_BUNDLE] +" WHERE Destination = ? ORDER BY TTL ASC;", */
			"SELECT Source, Destination, Reportto, Custodian, ProcFlags, Timestamp, Sequencenumber, Lifetime, Fragmentoffset, Appdatalength FROM "+ _tables[SQL_TABLE_BUNDLE] +" WHERE BundleID = ?;",
			/* "SELECT BundleID, Source, Destination, Reportto, Custodian, ProcFlags, Timestamp, Sequencenumber, Lifetime, Fragmentoffset, Appdatalength FROM "+ _tables[SQL_TABLE_BUNDLE] +" ORDER BY TTL ASC;", */
			"SELECT * FROM "+ _tables[SQL_TABLE_BUNDLE] +" WHERE Source = ? AND Timestamp = ? AND Sequencenumber = ? ORDER BY Fragmentoffset ASC;",

			"SELECT Source, Timestamp, Sequencenumber, Fragmentoffset, ProcFlags FROM Bundles WHERE TTL <= ?;",
			"SELECT Filename FROM "+ _tables[SQL_TABLE_BUNDLE] +" as a, "+ _tables[SQL_TABLE_BLOCK] +" as b WHERE a.BundleID = b.BundleID AND TTL <= ?;",
			"DELETE FROM "+ _tables[SQL_TABLE_BUNDLE] +" WHERE TTL <= ?;",
			"SELECT TTL FROM "+ _tables[SQL_TABLE_BUNDLE] +" ORDER BY TTL ASC LIMIT 1;",

			/* "SELECT ProcFlags FROM "+ _tables[SQL_TABLE_BUNDLE] +" WHERE BundleID = ?;", */

			"SELECT BundleID, Source, Destination, Reportto, Custodian, ProcFlags, Timestamp, Sequencenumber, Lifetime, Fragmentoffset, Appdatalength FROM "+ _tables[SQL_TABLE_BUNDLE] +" ORDER BY Size DESC LIMIT ?",
			"SELECT Source, Destination, Reportto, Custodian, ProcFlags, Timestamp, Sequencenumber, Lifetime, Fragmentoffset, Appdatalength FROM "+ _tables[SQL_TABLE_BUNDLE] +" WHERE Size > ? ORDER BY Size DESC LIMIT ?",
			"SELECT BundleID, Destination, Reportto, Custodian, ProcFlags, Timestamp, Sequencenumber, Lifetime, Fragmentoffset, Appdatalength FROM "+ _tables[SQL_TABLE_BUNDLE] +" WHERE Source = ?;",
			"SELECT BundleID, Source, Destination, Reportto, Custodian, ProcFlags, Timestamp, Sequencenumber, Lifetime, Fragmentoffset, Appdatalength FROM "+ _tables[SQL_TABLE_BUNDLE] +" ORDER BY TTL ASC LIMIT ?",

			"SELECT ROWID FROM "+ _tables[SQL_TABLE_BUNDLE] +" LIMIT 1;",
			"SELECT Count(ROWID) FROM "+ _tables[SQL_TABLE_BUNDLE] +";",

			"DELETE FROM "+ _tables[SQL_TABLE_BUNDLE] +" WHERE Source = ? AND Timestamp = ? AND Sequencenumber = ?;",
			"DELETE From "+ _tables[SQL_TABLE_BUNDLE] +" Where BundleID = ?;",
			"DELETE FROM "+ _tables[SQL_TABLE_BUNDLE] +";",
			"INSERT INTO "+ _tables[SQL_TABLE_BUNDLE] +" (BundleID, Source, Destination, Reportto, Custodian, ProcFlags, Timestamp, Sequencenumber, Lifetime, Fragmentoffset, Appdatalength, TTL, Size, Payloadlength, Payloadfilename) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);",

			"UPDATE "+ _tables[SQL_TABLE_BUNDLE] +" SET ProcFlags = ? WHERE BundleID = ?;",

			"SELECT Filename FROM "+ _tables[SQL_TABLE_BLOCK] +" WHERE BundleID = ? ORDER BY Blocknumber ASC;",
			"SELECT Filename FROM "+ _tables[SQL_TABLE_BLOCK] +" WHERE BundleID = ? AND Blocknumber = ?;",
			"DELETE FROM "+ _tables[SQL_TABLE_BLOCK] +";",
			"INSERT INTO "+ _tables[SQL_TABLE_BLOCK] +" (BundleID, BlockType, Filename, Blocknumber) VALUES (?,?,?,?);",

			"SELECT Routing FROM "+ _tables[SQL_TABLE_ROUTING] +" WHERE Key = ?;",
			"DELETE FROM "+ _tables[SQL_TABLE_ROUTING] +" WHERE Key = ?;",
			"DELETE FROM "+ _tables[SQL_TABLE_ROUTING] +";",
			"INSERT INTO "+ _tables[SQL_TABLE_ROUTING] +" (Key, Routing) VALUES (?,?);",

			"SELECT Routing FROM "+ _tables[SQL_TABLE_NODE_ROUTING_INFO] +" WHERE EID = ? AND Key = ?;",
			"DELETE FROM "+ _tables[SQL_TABLE_NODE_ROUTING_INFO] +" WHERE EID = ? AND Key = ?;",
			"DELETE FROM "+ _tables[SQL_TABLE_NODE_ROUTING_INFO] +";",
			"INSERT INTO "+ _tables[SQL_TABLE_NODE_ROUTING_INFO] +" (EID, Key, Routing) VALUES (?,?,?);",

			"SELECT Routing FROM "+ _tables[SQL_TABLE_BUNDLE_ROUTING_INFO] +" WHERE BundleID = ? AND Key = ?;",
			"DELETE FROM "+ _tables[SQL_TABLE_BUNDLE_ROUTING_INFO] +" WHERE BundleID = ? AND Key = ?;",
			"INSERT INTO "+ _tables[SQL_TABLE_BUNDLE_ROUTING_INFO] +" (BundleID, Key, Routing) VALUES (?,?,?);",

			"vacuum;"
		};

		const std::string SQLiteBundleStorage::_db_structure[11] =
		{
			"create table if not exists "+ _tables[SQL_TABLE_BUNDLE] +" (Key INTEGER PRIMARY KEY ASC, BundleID text, Source text, Destination text, Reportto text, Custodian text, ProcFlags int, Timestamp int, Sequencenumber int, Lifetime int, Fragmentoffset int, Appdatalength int, TTL int, Size int, Payloadlength int, Payloadfilename text);",
			"create table if not exists "+ _tables[SQL_TABLE_BLOCK] +" (Key INTEGER PRIMARY KEY ASC, BundleID text, BlockType int, Filename text, Blocknumber int);",
			"create table if not exists "+ _tables[SQL_TABLE_ROUTING] +" (INTEGER PRIMARY KEY ASC, Key int, Routing text);",
			"create table if not exists "+ _tables[SQL_TABLE_BUNDLE_ROUTING_INFO] +" (INTEGER PRIMARY KEY ASC, BundleID text, Key int, Routing text);",
			"create table if not exists "+ _tables[SQL_TABLE_NODE_ROUTING_INFO] +" (INTEGER PRIMARY KEY ASC, EID text, Key int, Routing text);",
			"CREATE TRIGGER IF NOT EXISTS Blockdelete AFTER DELETE ON "+ _tables[SQL_TABLE_BUNDLE] +" FOR EACH ROW BEGIN DELETE FROM "+ _tables[SQL_TABLE_BLOCK] +" WHERE BundleID = OLD.BundleID; DELETE FROM "+ _tables[SQL_TABLE_BUNDLE_ROUTING_INFO] +" WHERE BundleID = OLD.BundleID; END;",
			"CREATE TRIGGER IF NOT EXISTS BundleRoutingdelete BEFORE INSERT ON "+ _tables[SQL_TABLE_BUNDLE_ROUTING_INFO] +" BEGIN SELECT CASE WHEN (SELECT BundleID FROM "+ _tables[SQL_TABLE_BUNDLE] +" WHERE BundleID = NEW.BundleID) IS NULL THEN RAISE(ABORT, \"Foreign Key Violation: BundleID doesn't exist in BundleTable\")END; END;",
			"CREATE INDEX IF NOT EXISTS BlockIDIndex ON "+ _tables[SQL_TABLE_BLOCK] +" (BundleID);",
			"CREATE UNIQUE INDEX IF NOT EXISTS BundleIDIndex ON "+ _tables[SQL_TABLE_BUNDLE] +" (BundleID);",
			"CREATE UNIQUE INDEX IF NOT EXISTS RoutingKeyIndex ON "+ _tables[SQL_TABLE_ROUTING] +" (Key);",
			"CREATE UNIQUE INDEX IF NOT EXISTS BundleRoutingIDIndex ON "+ _tables[SQL_TABLE_BUNDLE_ROUTING_INFO] +" (BundleID,Key);",
		};

		ibrcommon::Mutex SQLiteBundleStorage::TaskIdle::_mutex;
		bool SQLiteBundleStorage::TaskIdle::_idle = false;

		SQLiteBundleStorage::SQLBundleQuery::SQLBundleQuery()
		 : _statement(NULL)
		{ }

		SQLiteBundleStorage::SQLBundleQuery::~SQLBundleQuery()
		{ }

		SQLiteBundleStorage::SQLiteBundleStorage(const ibrcommon::File &path, const size_t &size)
		 : dbPath(path), dbFile(path.get("sqlite.db")), dbSize(size) //, actual_time(0), nextExpiredTime(0)
		{
			//Configure SQLite Library
			SQLiteConfigure::configure();
		}

		SQLiteBundleStorage::~SQLiteBundleStorage()
		{
			stop();
			join();

			// shutdown sqlite library
			SQLiteConfigure::shutdown();
		}

		void SQLiteBundleStorage::openDatabase(const ibrcommon::File &path)
		{
			ibrcommon::MutexLock l(_db_mutex);

			// set the block path
			_blockPath = dbPath.get(_tables[SQL_TABLE_BLOCK]);

			//open the database
			if (sqlite3_open_v2(path.getPath().c_str(), &_database, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL))
			{
				IBRCOMMON_LOGGER(error) << "Can't open database: " << sqlite3_errmsg(_database) << IBRCOMMON_LOGGER_ENDL;
				sqlite3_close(_database);
				throw ibrcommon::Exception("Unable to open SQLite Database");
			}

			int err = 0;

			// create all tables
			for (size_t i = 0; i < 11; i++)
			{
				sqlite3_stmt *st = prepare(_db_structure[i]);
				err = sqlite3_step(st);
				if(err != SQLITE_DONE){
					IBRCOMMON_LOGGER(error) << "SQLiteBundleStorage: Constructorfailure: create BundleTable failed " << err << IBRCOMMON_LOGGER_ENDL;
				}
				sqlite3_finalize(st);
			}

			// create the bundle folder
			ibrcommon::File::createDirectory( _blockPath );

			// prepare all statements
			for (int i = 0; i < SQL_QUERIES_END; i++)
			{
				// prepare the statement
				int err = sqlite3_prepare_v2(_database, _sql_queries[i].c_str(), _sql_queries[i].length(), &_statements[i], 0);
				if ( err != SQLITE_OK )
				{
					IBRCOMMON_LOGGER(error) << "SQLiteBundlestorage: failure in prepareStatement: " << err << " with Query: " << _sql_queries[i] << IBRCOMMON_LOGGER_ENDL;
				}
			}
		}

		void SQLiteBundleStorage::componentRun()
		{
			// loop until aborted
			try {
				while (true)
				{
					Task *t = _tasks.getnpop(true);

					try {
						BlockingTask &btask = dynamic_cast<BlockingTask&>(*t);
						try {
							btask.run(*this);
						} catch (const std::exception&) {
							btask.abort();
							continue;
						};
						btask.done();
						continue;
					} catch (const std::bad_cast&) { };

					try {
						ibrcommon::AutoDelete<Task> killer(t);
						t->run(*this);
					} catch (const std::exception&) { };
				}
			} catch (const ibrcommon::QueueUnblockedException &ex) {
				// we are aborted, abort all blocking tasks
			}
		}

		void SQLiteBundleStorage::componentUp()
		{
			//register Events
			bindEvent(TimeEvent::className);
			bindEvent(GlobalEvent::className);

			// open the database and create all folders and files if needed
			openDatabase(dbFile);

			//Check if the database is consistent with the filesystem
			check_consistency();

			// calculate next Bundleexpiredtime
			update_expire_time();
		};

		void SQLiteBundleStorage::componentDown()
		{
			//unregister Events
			unbindEvent(TimeEvent::className);
			unbindEvent(GlobalEvent::className);

			ibrcommon::MutexLock l(_db_mutex);

			// free all statements
			for (int i = 0; i < SQL_QUERIES_END; i++)
			{
				// prepare the statement
				sqlite3_finalize(_statements[i]);
			}

			//close Databaseconnection
			if (sqlite3_close(_database) != SQLITE_OK)
			{
				IBRCOMMON_LOGGER(error) <<"unable to close Database" << IBRCOMMON_LOGGER_ENDL;
			}
		};

		bool SQLiteBundleStorage::__cancellation()
		{
			_tasks.abort();
			return true;
		}

		void SQLiteBundleStorage::execute(sqlite3_stmt *statement)
		{
			int err;
			err = sqlite3_step(statement);
			sqlite3_reset(statement);
			if(err == SQLITE_CONSTRAINT)
				cerr << "warning: Bundle is already in the storage"<<endl;
			else if(err != SQLITE_DONE){
				stringstream error;
				error << "SQLiteBundleStorage: Unable to execute Querry: " << err << " errmsg: " << sqlite3_errmsg(_database);
				throw SQLiteQueryException(error.str());
			}
		}

		void SQLiteBundleStorage::check_consistency()
		{
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

			list<ibrcommon::File> blist;
			list<ibrcommon::File>::iterator file_it;

			//1. Durchsuche die Dateien
			_blockPath.getFiles(blist);
			for(file_it = blist.begin(); file_it != blist.end(); file_it++)
			{
				datei = (*file_it).getPath();
				if(datei[datei.size()-1] != '.')
				{
					if(((*file_it).getBasename())[0] == 'b')
					{
						blockFiles.insert(datei);
					}
					else
						payloadfiles.insert(datei);
				}
			}

			//3. Check consistency of the bundles
			check_bundles(blockFiles);

			//4. Check consistency of the fragments
			check_fragments(payloadfiles);
		}

		void SQLiteBundleStorage::check_fragments(std::set<std::string> &payloadFiles)
		{
			int err;
			size_t procFlags, timestamp, sequencenumber, appdatalength, lifetime;
			string filename, source, dest, custody, repto;
			set<string> consistenFiles, inconsistenSources;
			set<size_t> inconsistentTimestamp, inconsistentSeq_number;
			set<string>::iterator file_it, consisten_it;


			sqlite3_stmt *getPayloadfiles = prepare("SELECT Source, Destination, Reportto, Custodian, ProcFlags, Timestamp, Sequencenumber, Lifetime, Fragmentoffset, Appdatalength, Payloadfilename FROM "+_tables[SQL_TABLE_BUNDLE]+" WHERE Payloadfilename != NULL;");
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

		void SQLiteBundleStorage::check_bundles(std::set<std::string> &blockFiles)
		{
			int err;
			size_t procFlags,timestamp,sequencenumber,appdatalength,lifetime;

			string source,Bundle;
			string filename, id, payloadfilename;
			string dest, custody, repto;

			set<std::string> blockIDList;
			set<string>::iterator fileList_iterator;

			sqlite3_stmt *blockConistencyCheck = prepare("SELECT Filename,BundleID,Blocknumber FROM "+ _tables[SQL_TABLE_BLOCK] +";");
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
				sqlite3_bind_text(_statements[BLOCK_GET_ID], 1, (*fileList_iterator).c_str(), (*fileList_iterator).length(), SQLITE_TRANSIENT);
				for (err = sqlite3_step(_statements[BLOCK_GET_ID]); err == SQLITE_ROW; err = sqlite3_step(_statements[BLOCK_GET_ID]))
				{
					filename = (const char*)sqlite3_column_text(_statements[BLOCK_GET_ID],0);
					blockFiles.insert(filename);
				}
				sqlite3_reset(_statements[BLOCK_GET_ID]);
				if(err != SQLITE_DONE){
					stringstream error;
					error << "SQLiteBundleStorage: Unable to execute Querry: " << err << " errmsg: " << sqlite3_errmsg(_database);
					throw SQLiteQueryException(error.str());
				}

				//Generate Report
				sqlite3_bind_text(_statements[BUNDLE_GET_ID], 1, (*fileList_iterator).c_str(), (*fileList_iterator).length(), SQLITE_TRANSIENT);
				err = sqlite3_step(_statements[BUNDLE_GET_ID]);
				if(err == SQLITE_ROW){
					source  = (const char*) sqlite3_column_text(_statements[BUNDLE_GET_ID], 0);
					dest    = (const char*) sqlite3_column_text(_statements[BUNDLE_GET_ID], 1);
					repto   = (const char*) sqlite3_column_text(_statements[BUNDLE_GET_ID], 2);
					custody = (const char*) sqlite3_column_text(_statements[BUNDLE_GET_ID], 3);
					procFlags = sqlite3_column_int64(_statements[BUNDLE_GET_ID], 4);
					timestamp = sqlite3_column_int64(_statements[BUNDLE_GET_ID], 5);
					sequencenumber = sqlite3_column_int64(_statements[BUNDLE_GET_ID], 6);
					lifetime  = sqlite3_column_int64(_statements[BUNDLE_GET_ID], 7);
					appdatalength  = sqlite3_column_int64(_statements[BUNDLE_GET_ID], 9);

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
				sqlite3_bind_text(_statements[BUNDLE_REMOVE], 1, (*fileList_iterator).c_str(), (*fileList_iterator).length(), SQLITE_TRANSIENT);
				execute(_statements[BUNDLE_REMOVE]);
			}

			//Delete Blockfiles
			for(fileList_iterator = blockFiles.begin(); fileList_iterator != blockFiles.end(); fileList_iterator++){
				ibrcommon::File blockfile(*fileList_iterator);
				blockfile.remove();
			}
		}


		sqlite3_stmt* SQLiteBundleStorage::prepare(const std::string &sqlquery)
		{
			// prepare the statement
			sqlite3_stmt *statement;
			int err = sqlite3_prepare_v2(_database, sqlquery.c_str(), sqlquery.length(), &statement, 0);
			if ( err != SQLITE_OK )
			{
				IBRCOMMON_LOGGER(error) << "SQLiteBundlestorage: failure in prepareStatement: " << err << " with Query: " << sqlquery << IBRCOMMON_LOGGER_ENDL;
			}
			return statement;
		}

		dtn::data::MetaBundle SQLiteBundleStorage::get_meta(const dtn::data::BundleID &id)
		{
			dtn::data::MetaBundle bundle;

			// lock the database
			ibrcommon::MutexLock l(_db_mutex);

			// reset bindings
//			sqlite3_clear_bindings(_statements[BUNDLE_GET_ID]);

			// do this while db is locked
			sqlite3_bind_text(_statements[BUNDLE_GET_ID], 1, id.toString().c_str(), id.toString().length(), SQLITE_TRANSIENT);

			// execute the query and check for error
			if (sqlite3_step(_statements[BUNDLE_GET_ID]) != SQLITE_ROW)
			{
				sqlite3_reset(_statements[BUNDLE_GET_ID]);
				stringstream error;
				error << "SQLiteBundleStorage: No Bundle found with BundleID: " << id.toString();
				IBRCOMMON_LOGGER(error) << error.str() << IBRCOMMON_LOGGER_ENDL;
				throw SQLiteQueryException(error.str());
			}

			sqlite3_reset(_statements[BUNDLE_GET_ID]);

			// query bundle data
			get(_statements[BUNDLE_GET_ID], bundle);

			return bundle;
		}

		void SQLiteBundleStorage::get(sqlite3_stmt *st, dtn::data::MetaBundle &bundle, size_t offset)
		{
			bundle.source = std::string( (const char*) sqlite3_column_text(st, offset + 0) );
			bundle.destination = std::string( (const char*) sqlite3_column_text(st, offset + 1) );
			bundle.reportto = std::string( (const char*) sqlite3_column_text(st, offset + 2) );
			bundle.custodian = std::string( (const char*) sqlite3_column_text(st, offset + 3) );
			bundle.procflags = sqlite3_column_int(st, offset + 4);
			bundle.timestamp = sqlite3_column_int64(st, offset + 5);
			bundle.sequencenumber = sqlite3_column_int64(st, offset + 6);
			bundle.lifetime = sqlite3_column_int64(st, offset + 7);

			if (bundle.procflags & data::Bundle::FRAGMENT)
			{
				bundle.offset = sqlite3_column_int64(st, offset + 8);
				bundle.appdatalength = sqlite3_column_int64(st, offset + 9);
			}
		}

		void SQLiteBundleStorage::get(sqlite3_stmt *st, dtn::data::Bundle &bundle, size_t offset)
		{
			bundle._source = std::string( (const char*) sqlite3_column_text(st, offset + 0) );
			bundle._destination = std::string( (const char*) sqlite3_column_text(st, offset + 1) );
			bundle._reportto = std::string( (const char*) sqlite3_column_text(st, offset + 2) );
			bundle._custodian = std::string( (const char*) sqlite3_column_text(st, offset + 3) );
			bundle._procflags = sqlite3_column_int(st, offset + 4);
			bundle._timestamp = sqlite3_column_int64(st, offset + 5);
			bundle._sequencenumber = sqlite3_column_int64(st, offset + 6);
			bundle._lifetime = sqlite3_column_int64(st, offset + 7);

			if (bundle._procflags & data::Bundle::FRAGMENT)
			{
				bundle._fragmentoffset = sqlite3_column_int64(st, offset + 8);
				bundle._appdatalength = sqlite3_column_int64(st, offset + 9);
			}
		}

		const std::list<dtn::data::MetaBundle> SQLiteBundleStorage::get(BundleFilterCallback &cb)
		{
			std::list<dtn::data::MetaBundle> ret;

			const std::string base_query =
					"SELECT BundleID, Source, Destination, Reportto, Custodian, ProcFlags, Timestamp, Sequencenumber, Lifetime, Fragmentoffset, Appdatalength FROM " + _tables[SQL_TABLE_BUNDLE];

			sqlite3_stmt *st = NULL;

			try {
				SQLBundleQuery &query = dynamic_cast<SQLBundleQuery&>(cb);

				if (query._statement == NULL)
				{
					std::string sql = base_query + " WHERE " + query.getWhere() + " LIMIT ?,?;";
					query._statement = prepare(sql);
				}

				st = query._statement;
			} catch (const std::bad_cast&) {
				// this query is not optimized for sql, use the generic way (slow)
				st = _statements[BUNDLE_GET_FILTER];
			};

			// lock the database
			ibrcommon::MutexLock l(_db_mutex);

			int offset = 0;
			while (ret.size() != cb.limit())
			{
//				sqlite3_clear_bindings(st);

				sqlite3_bind_int64(st, 1, offset);
				sqlite3_bind_int64(st, 2, cb.limit());

				if (sqlite3_step(st) == SQLITE_DONE)
				{
					// returned no result
					sqlite3_reset(st);
					break;
				}

				while (ret.size() != cb.limit())
				{
					dtn::data::MetaBundle m;

					// extract the primary values and set them in the bundle object
					get(st, m, 1);

					// ask the filter if this bundle should be added to the return list
					if (cb.shouldAdd(m))
					{
						// add the bundle to the list
						ret.push_back(m);
					}

					if (sqlite3_step(st) != SQLITE_ROW)
					{
						break;
					}
				}

				sqlite3_reset(st);

				// increment the offset, because we might not have enough
				offset += 50;
			}

			return ret;
		}

		dtn::data::Bundle SQLiteBundleStorage::get(const dtn::data::BundleID &id)
		{
			dtn::data::Bundle bundle;

			IBRCOMMON_LOGGER_DEBUG(25) << "get bundle from sqlite storage " << id.toString() << IBRCOMMON_LOGGER_ENDL;

			// do this while db is locked
			ibrcommon::MutexLock l(_db_mutex);

//			sqlite3_clear_bindings(_statements[BUNDLE_GET_ID]);
			sqlite3_bind_text(_statements[BUNDLE_GET_ID], 1, id.toString().c_str(), id.toString().length(), SQLITE_TRANSIENT);

			int err = sqlite3_step(_statements[BUNDLE_GET_ID]);

			// execute the query and check for error
			if (err != SQLITE_ROW)
			{
				sqlite3_reset(_statements[BUNDLE_GET_ID]);
				IBRCOMMON_LOGGER(error) << "sql error: " << err << "; No Bundle found with id: " << id.toString() << IBRCOMMON_LOGGER_ENDL;
				throw dtn::core::BundleStorage::NoBundleFoundException();
			}

			// query bundle data
			get(_statements[BUNDLE_GET_ID], bundle);

			// reset used variables
			sqlite3_reset(_statements[BUNDLE_GET_ID]);

			// read all blocks
			get_blocks(bundle, id);

			return bundle;
		}

//		void SQLiteBundleStorage::setPriority(const int priority, const dtn::data::BundleID &id)
//		{
//			int err;
//			size_t procflags;
//			{
//				sqlite3_bind_text(_statements[PROCFLAGS_GET],1,id.toString().c_str(), id.toString().length(), SQLITE_TRANSIENT);
//				err = sqlite3_step(_statements[PROCFLAGS_GET]);
//				if(err == SQLITE_ROW)
//					procflags = sqlite3_column_int(_statements[PROCFLAGS_GET],0);
//				sqlite3_reset(_statements[PROCFLAGS_GET]);
//				if(err != SQLITE_DONE){
//					stringstream error;
//					error << "SQLiteBundleStorage: error while Select Querry: " << err;
//					IBRCOMMON_LOGGER(error) << error.str() << IBRCOMMON_LOGGER_ENDL;
//					throw SQLiteQueryException(error.str());
//				}
//
//				//Priority auf 0 Setzen
//				procflags -= ( 128*(procflags & dtn::data::Bundle::PRIORITY_BIT1) + 256*(procflags & dtn::data::Bundle::PRIORITY_BIT2));
//
//				//Set Prioritybits
//				switch(priority){
//				case 1: procflags += data::Bundle::PRIORITY_BIT1; break; //Set PRIORITY_BIT1
//				case 2: procflags += data::Bundle::PRIORITY_BIT2; break;
//				case 3: procflags += (data::Bundle::PRIORITY_BIT1 + data::Bundle::PRIORITY_BIT2); break;
//				}
//
//				sqlite3_bind_text(_statements[PROCFLAGS_SET],1,id.toString().c_str(), id.toString().length(), SQLITE_TRANSIENT);
//				sqlite3_bind_int64(_statements[PROCFLAGS_SET],2,priority);
//				sqlite3_step(_statements[PROCFLAGS_SET]);
//				sqlite3_reset(_statements[PROCFLAGS_SET]);
//
//				if(err != SQLITE_DONE)
//				{
//					stringstream error;
//					error << "SQLiteBundleStorage: setPriority error while Select Querry: " << err;
//					IBRCOMMON_LOGGER(error) << error.str() << IBRCOMMON_LOGGER_ENDL;
//					throw SQLiteQueryException(error.str());
//				}
//			}
//		}

		void SQLiteBundleStorage::store(const dtn::data::Bundle &bundle)
		{
			IBRCOMMON_LOGGER_DEBUG(25) << "store bundle " << bundle.toString() << IBRCOMMON_LOGGER_ENDL;

			// determine if the bundle is for local delivery
			bool local = (bundle._destination.sameHost(BundleCore::local)
					&& (bundle._procflags & dtn::data::PrimaryBlock::DESTINATION_IS_SINGLETON));

			// if the bundle is a fragment store it as one
			if (bundle.get(dtn::data::Bundle::FRAGMENT) && local)
			{
//				storeFragment(bundle);
				return;
			}

			int err;

			const dtn::data::BundleID id(bundle);
			size_t TTL = bundle._timestamp + bundle._lifetime;

			// lock the database
			ibrcommon::MutexLock l(_db_mutex);

			// stores the blocks to a file
			int size = store_blocks(bundle);

			// reset all bindings
//			sqlite3_clear_bindings(_statements[BUNDLE_STORE]);

			sqlite3_bind_text(_statements[BUNDLE_STORE], 1, id.toString().c_str(), id.toString().length(), SQLITE_TRANSIENT);
			sqlite3_bind_text(_statements[BUNDLE_STORE], 2, bundle._source.getString().c_str(), bundle._source.getString().length(), SQLITE_TRANSIENT);
			sqlite3_bind_text(_statements[BUNDLE_STORE], 3, bundle._destination.getString().c_str(), bundle._destination.getString().length(), SQLITE_TRANSIENT);
			sqlite3_bind_text(_statements[BUNDLE_STORE], 4, bundle._reportto.getString().c_str(), bundle._reportto.getString().length(), SQLITE_TRANSIENT);
			sqlite3_bind_text(_statements[BUNDLE_STORE], 5, bundle._custodian.getString().c_str(), bundle._custodian.getString().length(), SQLITE_TRANSIENT);
			sqlite3_bind_int(_statements[BUNDLE_STORE], 6, bundle._procflags);
			sqlite3_bind_int64(_statements[BUNDLE_STORE], 7, bundle._timestamp);
			sqlite3_bind_int(_statements[BUNDLE_STORE], 8, bundle._sequencenumber);
			sqlite3_bind_int64(_statements[BUNDLE_STORE], 9, bundle._lifetime);

			if (bundle.get(dtn::data::Bundle::FRAGMENT))
			{
				sqlite3_bind_int64(_statements[BUNDLE_STORE], 10, bundle._fragmentoffset);
				sqlite3_bind_int64(_statements[BUNDLE_STORE], 11, bundle._appdatalength);
			}
			else
			{
				sqlite3_bind_int64(_statements[BUNDLE_STORE], 10, NULL);
				sqlite3_bind_int64(_statements[BUNDLE_STORE], 11, NULL);
			}

			sqlite3_bind_int64(_statements[BUNDLE_STORE], 12, TTL);
			sqlite3_bind_int(_statements[BUNDLE_STORE], 13, size);
			sqlite3_bind_int(_statements[BUNDLE_STORE], 14, NULL);
			sqlite3_bind_text(_statements[BUNDLE_STORE],15, NULL, 0, SQLITE_TRANSIENT);

			// store the bundle data in the database
			err = sqlite3_step(_statements[BUNDLE_STORE]);

			if (err == SQLITE_CONSTRAINT)
			{
				IBRCOMMON_LOGGER(error) << "warning: Bundle is already in the storage" << IBRCOMMON_LOGGER_ENDL;

				// reset the statement
				sqlite3_reset(_statements[BUNDLE_STORE]);
			}
			else if (err != SQLITE_DONE)
			{
				// reset the statement
				sqlite3_reset(_statements[BUNDLE_STORE]);

				stringstream error;
				error << "SQLiteBundleStorage: store() failure: " << err << " " <<  sqlite3_errmsg(_database);
				IBRCOMMON_LOGGER(error) << error.str() << IBRCOMMON_LOGGER_ENDL;
				throw SQLiteQueryException(error.str());
			}
			else
			{
				// reset the statement
				sqlite3_reset(_statements[BUNDLE_STORE]);

				IBRCOMMON_LOGGER_DEBUG(10) << "bundle " << bundle.toString() << " stored" << IBRCOMMON_LOGGER_ENDL;
			}

			// set new expire time
			new_expire_time(TTL);
		}

		void SQLiteBundleStorage::remove(const dtn::data::BundleID &id)
		{
			_tasks.push(new TaskRemove(id));
		}

		void SQLiteBundleStorage::TaskRemove::run(SQLiteBundleStorage &storage)
		{
			// lock the database
			ibrcommon::MutexLock l(storage._db_mutex);

			// reset bindings
			sqlite3_clear_bindings(storage._statements[BLOCK_GET_ID]);

			// first remove all block files of this bundle
			sqlite3_bind_text(storage._statements[BLOCK_GET_ID], 1, _id.toString().c_str(), _id.toString().length(), SQLITE_TRANSIENT);

			while (sqlite3_step(storage._statements[BLOCK_GET_ID]) == SQLITE_ROW)
			{
				ibrcommon::File blockfile( (const char*)sqlite3_column_text(storage._statements[BLOCK_GET_ID], 0) );
				blockfile.remove();
			}

			sqlite3_reset(storage._statements[BLOCK_GET_ID]);

			// then remove the bundle data
			sqlite3_bind_text(storage._statements[BUNDLE_REMOVE], 1, _id.toString().c_str(), _id.toString().length(), SQLITE_TRANSIENT);
			storage.execute(storage._statements[BUNDLE_REMOVE]);

			IBRCOMMON_LOGGER_DEBUG(10) << "bundle " << _id.toString() << " deleted" << IBRCOMMON_LOGGER_ENDL;

			//update deprecated timer
			storage.update_expire_time();
		}

		std::string SQLiteBundleStorage::getBundleRoutingInfo(const data::BundleID &bundleID, const int &key)
		{
			int err;
			string result;

			sqlite3_bind_text(_statements[ROUTING_GET],1,bundleID.toString().c_str(),bundleID.toString().length(), SQLITE_TRANSIENT);
			sqlite3_bind_int(_statements[ROUTING_GET],2,key);
			err = sqlite3_step(_statements[ROUTING_GET]);
			if(err == SQLITE_ROW){
				result = (const char*) sqlite3_column_text(_statements[ROUTING_GET],0);
			}
			sqlite3_reset(_statements[ROUTING_GET]);
			if(err != SQLITE_DONE && err != SQLITE_ROW){
				stringstream error;
				error << "SQLiteBundleStorage: getBundleRoutingInfo: " << err << " errmsg: " << err;
				IBRCOMMON_LOGGER(error) << error.str() << IBRCOMMON_LOGGER_ENDL;
				throw SQLiteQueryException(error.str());
			}

			return result;
		}

		std::string SQLiteBundleStorage::getNodeRoutingInfo(const data::EID &eid, const int &key)
		{
			int err;
			string result;

			err = sqlite3_bind_text(_statements[NODE_GET],1,eid.getString().c_str(),eid.getString().length(), SQLITE_TRANSIENT);
			err = sqlite3_bind_int(_statements[NODE_GET],2,key);
			err = sqlite3_step(_statements[NODE_GET]);
			if(err == SQLITE_ROW){
				result = (const char*) sqlite3_column_text(_statements[NODE_GET],0);
			}
			sqlite3_reset(_statements[NODE_GET]);
			if(err != SQLITE_DONE && err != SQLITE_ROW){
				stringstream error;
				error << "SQLiteBundleStorage: getNodeRoutingInfo: " << err << " errmsg: " << sqlite3_errmsg(_database);
				IBRCOMMON_LOGGER(error) << error.str() << IBRCOMMON_LOGGER_ENDL;
				throw SQLiteQueryException(error.str());
			}

			return result;
		}

		std::string SQLiteBundleStorage::getRoutingInfo(const int &key)
		{
			int err;
			string result;

			sqlite3_bind_int(_statements[INFO_GET],1,key);
			err = sqlite3_step(_statements[INFO_GET]);
			if(err == SQLITE_ROW){
				result = (const char*) sqlite3_column_text(_statements[INFO_GET],0);
			}
			sqlite3_reset(_statements[INFO_GET]);
			if(err != SQLITE_DONE && err != SQLITE_ROW){
				stringstream error;
				error << "SQLiteBundleStorage: getRoutingInfo: " << err << " errmsg: " << sqlite3_errmsg(_database);
				IBRCOMMON_LOGGER(error) << error.str() << IBRCOMMON_LOGGER_ENDL;
				throw SQLiteQueryException(error.str());
			}

			return result;
		}

		void SQLiteBundleStorage::storeBundleRoutingInfo(const data::BundleID &bundleID, const int &key, const string &routingInfo)
		{
			int err;

			sqlite3_bind_text(_statements[ROUTING_STORE],1,bundleID.toString().c_str(),bundleID.toString().length(), SQLITE_TRANSIENT);
			sqlite3_bind_int(_statements[ROUTING_STORE],2,key);
			sqlite3_bind_text(_statements[ROUTING_STORE],3,routingInfo.c_str(),routingInfo.length(), SQLITE_TRANSIENT);
			err = sqlite3_step(_statements[ROUTING_STORE]);
			sqlite3_reset(_statements[ROUTING_STORE]);
			if(err == SQLITE_CONSTRAINT)
				cerr << "warning: BundleRoutingInformations for "<<bundleID.toString() <<" are either already in the storage or there is no Bundle with this BundleID."<<endl;
			else if(err != SQLITE_DONE){
				stringstream error;
				error << "SQLiteBundleStorage: storeBundleRoutingInfo: " << err << " errmsg: " << err;
				IBRCOMMON_LOGGER(error) << error.str() << IBRCOMMON_LOGGER_ENDL;
				throw SQLiteQueryException(error.str());
			}
		}

		void SQLiteBundleStorage::storeNodeRoutingInfo(const data::EID &node, const int &key, const std::string &routingInfo)
		{
			int err;

			sqlite3_bind_text(_statements[NODE_STORE],1,node.getString().c_str(),node.getString().length(), SQLITE_TRANSIENT);
			sqlite3_bind_int(_statements[NODE_STORE],2,key);
			sqlite3_bind_text(_statements[NODE_STORE],3,routingInfo.c_str(),routingInfo.length(), SQLITE_TRANSIENT);
			err = sqlite3_step(_statements[NODE_STORE]);
			sqlite3_reset(_statements[NODE_STORE]);
			if(err == SQLITE_CONSTRAINT)
				cerr << "warning: NodeRoutingInfo for "<<node.getString() <<" are already in the storage."<<endl;
			else if(err != SQLITE_DONE){
				stringstream error;
				error << "SQLiteBundleStorage: storeNodeRoutingInfo: " << err << " errmsg: " << err;
				IBRCOMMON_LOGGER(error) << error.str() << IBRCOMMON_LOGGER_ENDL;
				throw SQLiteQueryException(error.str());
			}
		}

		void SQLiteBundleStorage::storeRoutingInfo(const int &key, const std::string &routingInfo)
		{
			int err;

			sqlite3_bind_int(_statements[INFO_STORE],1,key);
			sqlite3_bind_text(_statements[INFO_STORE],2,routingInfo.c_str(),routingInfo.length(), SQLITE_TRANSIENT);
			err = sqlite3_step(_statements[INFO_STORE]);
			sqlite3_reset(_statements[INFO_STORE]);
			if(err == SQLITE_CONSTRAINT)
				cerr << "warning: There are already RoutingInformation refereed by this Key"<<endl;
			else if(err != SQLITE_DONE){
				stringstream error;
				error << "SQLiteBundleStorage: storeRoutingInfo: " << err << " errmsg: " << err;
				IBRCOMMON_LOGGER(error) << error.str() << IBRCOMMON_LOGGER_ENDL;
				throw SQLiteQueryException(error.str());
			}
		}

		void SQLiteBundleStorage::removeBundleRoutingInfo(const data::BundleID &bundleID, const int &key)
		{
			int err;

			sqlite3_bind_text(_statements[ROUTING_REMOVE],1,bundleID.toString().c_str(),bundleID.toString().length(), SQLITE_TRANSIENT);
			sqlite3_bind_int(_statements[ROUTING_REMOVE],2,key);
			err = sqlite3_step(_statements[ROUTING_REMOVE]);
			sqlite3_reset(_statements[ROUTING_REMOVE]);
			if(err != SQLITE_DONE){
				stringstream error;
				error << "SQLiteBundleStorage: removeBundleRoutingInfo: " << err << " errmsg: " << err;
				IBRCOMMON_LOGGER(error) << error.str() << IBRCOMMON_LOGGER_ENDL;
				throw SQLiteQueryException(error.str());
			}
		}

		void SQLiteBundleStorage::removeNodeRoutingInfo(const data::EID &eid, const int &key)
		{
			int err;

			sqlite3_bind_text(_statements[NODE_REMOVE],1,eid.getString().c_str(),eid.getString().length(), SQLITE_TRANSIENT);
			sqlite3_bind_int(_statements[NODE_REMOVE],2,key);
			err = sqlite3_step(_statements[NODE_REMOVE]);
			sqlite3_reset(_statements[NODE_REMOVE]);
			if(err != SQLITE_DONE){
				stringstream error;
				error << "SQLiteBundleStorage: removeNodeRoutingInfo: " << err << " errmsg: " << err;
				IBRCOMMON_LOGGER(error) << error.str() << IBRCOMMON_LOGGER_ENDL;
				throw SQLiteQueryException(error.str());
			}
		}

		void SQLiteBundleStorage::removeRoutingInfo(const int &key)
		{
			int err;

			sqlite3_bind_int(_statements[INFO_REMOVE],1,key);
			err = sqlite3_step(_statements[INFO_REMOVE]);
			sqlite3_reset(_statements[INFO_REMOVE]);
			if(err != SQLITE_DONE){
				stringstream error;
				error << "SQLiteBundleStorage: removeRoutingInfo: " << err << " errmsg: " << err;
				IBRCOMMON_LOGGER(error) << error.str() << IBRCOMMON_LOGGER_ENDL;
				throw SQLiteQueryException(error.str());
			}
		}

		void SQLiteBundleStorage::clearAll()
		{
			sqlite3_step(_statements[BUNDLE_CLEAR]);
			sqlite3_reset(_statements[BUNDLE_CLEAR]);
			sqlite3_step(_statements[ROUTING_CLEAR]);
			sqlite3_reset(_statements[ROUTING_CLEAR]);
			sqlite3_step(_statements[NODE_CLEAR]);
			sqlite3_reset(_statements[NODE_CLEAR]);
			sqlite3_step(_statements[BLOCK_CLEAR]);
			sqlite3_reset(_statements[BLOCK_CLEAR]);
			if(SQLITE_DONE != sqlite3_step(_statements[VACUUM])){
				sqlite3_reset(_statements[VACUUM]);
				throw SQLiteQueryException("SQLiteBundleStore: clear(): vacuum failed.");
			}
			sqlite3_reset(_statements[VACUUM]);

			//Delete Folder SQL_TABLE_BLOCK containing Blocks
			{
				_blockPath.remove(true);
				ibrcommon::File::createDirectory(_blockPath);
			}

			next_expiration = 0;
		}

		void SQLiteBundleStorage::clear()
		{
			sqlite3_step(_statements[BUNDLE_CLEAR]);
			sqlite3_reset(_statements[BUNDLE_CLEAR]);
			sqlite3_step(_statements[BLOCK_CLEAR]);
			sqlite3_reset(_statements[BLOCK_CLEAR]);
			if(SQLITE_DONE != sqlite3_step(_statements[VACUUM])){
				sqlite3_reset(_statements[VACUUM]);
				throw SQLiteQueryException("SQLiteBundleStore: clear(): vacuum failed.");
			}
			sqlite3_reset(_statements[VACUUM]);

			//Delete Folder SQL_TABLE_BLOCK containing Blocks
			{
				_blockPath.remove(true);
				ibrcommon::File::createDirectory(_blockPath);
			}

			next_expiration = 0;
		}



		bool SQLiteBundleStorage::empty()
		{
			if (SQLITE_DONE == sqlite3_step(_statements[EMPTY_CHECK]))
			{
				sqlite3_reset(_statements[EMPTY_CHECK]);
				return true;
			}
			else
			{
				sqlite3_reset(_statements[EMPTY_CHECK]);
				return false;
			}
		}

		unsigned int SQLiteBundleStorage::count()
		{
			int rows = 0,err;
			err = sqlite3_step(_statements[COUNT_ENTRIES]);
			if(err == SQLITE_ROW)
			{
				rows = sqlite3_column_int(_statements[COUNT_ENTRIES],0);
			}
			sqlite3_reset(_statements[COUNT_ENTRIES]);

			if (err != SQLITE_ROW)
			{
				stringstream error;
				error << "SQLiteBundleStorage: count: failure " << err << " " << sqlite3_errmsg(_database);
				IBRCOMMON_LOGGER(error) << error.str() << IBRCOMMON_LOGGER_ENDL;
				throw SQLiteQueryException(error.str());
			}
			return rows;
		}

//		int SQLiteBundleStorage::occupiedSpace()
//		{
//			int size = 0;
//			std::list<ibrcommon::File> files;
//
//			if (_blockPath.getFiles(files))
//			{
//				IBRCOMMON_LOGGER(error) << "occupiedSpace: unable to open Directory " << _blockPath.getPath() << IBRCOMMON_LOGGER_ENDL;
//				return -1;
//			}
//
//			for (std::list<ibrcommon::File>::const_iterator iter = files.begin(); iter != files.end(); iter++)
//			{
//				const ibrcommon::File &f = (*iter);
//
//				if (!f.isSystem())
//				{
//					size += f.size();
//				}
//			}
//
//			// add db file size
//			size += dbFile.size();
//
//			return size;
//		}

//		void SQLiteBundleStorage::storeFragment(const dtn::data::Bundle &bundle)
//		{
//			int err = 0;
//			size_t bitcounter = 0;
//			bool allFragmentsReceived(true);
//			size_t payloadsize, TTL, fragmentoffset;
//
//			ibrcommon::File payloadfile, filename;
//
//			// generate a bundle id string
//			std::string bundleID = dtn::data::BundleID(bundle).toString();
//
//			// get the destination id string
//			std::string destination = bundle._destination.getString();
//
//			// get the source id string
//			std::string sourceEID = bundle._source.getString();
//
//			// calculate the expiration timestamp
//			TTL = bundle._timestamp + bundle._lifetime;
//
//			const dtn::data::PayloadBlock &block = bundle.getBlock<dtn::data::PayloadBlock>();
//			ibrcommon::BLOB::Reference payloadBlob = block.getBLOB();
//
//			// get the payload size
//			{
//				ibrcommon::BLOB::iostream io = payloadBlob.iostream();
//				payloadsize = io.size();
//			}
//
//			//Saves the blocks from the first and the last Fragment, they may contain Extentionblock
//			bool last = bundle._fragmentoffset + payloadsize == bundle._appdatalength;
//			bool first = bundle._fragmentoffset == 0;
//
//			if(first || last)
//			{
//				int i = 0, blocknumber = 0;
//
//				// get the list of blocks
//				std::list<const dtn::data::Block*> blocklist = bundle.getBlocks();
//
//				// iterate through all blocks
//				for (std::list<const dtn::data::Block* >::const_iterator it = blocklist.begin(); it != blocklist.end(); it++, i++)
//				{
//					// generate a temporary block file
//					ibrcommon::TemporaryFile tmpfile(_blockPath, "block");
//
//					ofstream datei(tmpfile.getPath().c_str(), std::ios::out | std::ios::binary);
//
//					dtn::data::SeparateSerializer serializer(datei);
//					serializer << (*(*it));
//					datei.close();
//
//					/* Schreibt Blöcke in die DB.
//					 * Die Blocknummern werden nach folgendem Schema in die DB geschrieben:
//					 * 		Die Blöcke des ersten Fragments fangen bei -x an und gehen bis -1
//					 * 		Die Blöcke des letzten Fragments beginnen bei 1 und gehen bis x
//					 * 		Der Payloadblock wird an Stelle 0 eingefügt.
//					 */
//					sqlite3_bind_text(_statements[BLOCK_STORE], 1, bundleID.c_str(), bundleID.length(), SQLITE_TRANSIENT);
//					sqlite3_bind_int(_statements[BLOCK_STORE], 2, (int)((*it)->getType()));
//					sqlite3_bind_text(_statements[BLOCK_STORE], 3, tmpfile.getPath().c_str(), tmpfile.getPath().size(), SQLITE_TRANSIENT);
//
//					blocknumber = blocklist.size() - i;
//
//					if (first)
//					{
//						blocknumber = (blocklist.size() - i)*(-1);
//					}
//
//					sqlite3_bind_int(_statements[BLOCK_STORE],4,blocknumber);
//					executeQuery(_statements[BLOCK_STORE]);
//				}
//			}
//
//			//At first the database is checked for already received bundles and some
//			//informations are read, which effect the following program flow.
//			sqlite3_bind_text(_statements[BUNDLE_GET_FRAGMENT],1,sourceEID.c_str(),sourceEID.length(),SQLITE_TRANSIENT);
//			sqlite3_bind_int64(_statements[BUNDLE_GET_FRAGMENT],2,bundle._timestamp);
//			sqlite3_bind_int(_statements[BUNDLE_GET_FRAGMENT],3,bundle._sequencenumber);
//			err = sqlite3_step(_statements[BUNDLE_GET_FRAGMENT]);
//
//			//Es ist noch kein Eintrag vorhanden: generiere Payloadfilename
//			if(err == SQLITE_DONE)
//			{
//				// generate a temporary block file
//				ibrcommon::TemporaryFile tmpfile(_blockPath, "block");
//				payloadfile = tmpfile;
//			}
//			//Es ist bereits ein eintrag vorhanden: speicher den payloadfilename
//			else if (err == SQLITE_ROW)
//			{
//				payloadfile = ibrcommon::File( (const char*)sqlite3_column_text(_statements[BUNDLE_GET_FRAGMENT], 14) );				//14 = Payloadfilename
//			}
//
//			while (err == SQLITE_ROW){
//				//The following codepice summs the payloadfragmentbytes, to check if all fragments were received
//				//and the bundle is complete.
//				fragmentoffset = sqlite3_column_int(_statements[BUNDLE_GET_FRAGMENT],9);								// 9 = fragmentoffset
//
//				if(bitcounter >= fragmentoffset){
//					bitcounter += fragmentoffset - bitcounter + sqlite3_column_int(_statements[BUNDLE_GET_FRAGMENT],13);	//13 = payloadlength
//				}
//				else if(bitcounter >= bundle._fragmentoffset){
//					bitcounter += bundle._fragmentoffset - bitcounter + payloadsize;
//				}
//				else{
//					allFragmentsReceived = false;
//				}
//				err = sqlite3_step(_statements[BUNDLE_GET_FRAGMENT]);
//			}
//
//			if(bitcounter +1 >= bundle._fragmentoffset && allFragmentsReceived) //Wenn das aktuelle fragment das größte ist, ist die Liste durchlaufen und das Frag nicht dazugezählt. Dies wird hier nachgeholt.
//			{
//				bitcounter += bundle._fragmentoffset - bitcounter + payloadsize;
//			}
//
//			sqlite3_reset(_statements[BUNDLE_GET_FRAGMENT]);
//			if(err != SQLITE_DONE){
//				stringstream error;
//				error << "SQLiteBundleStorage: storeFragment: " << err << " errmsg: " << sqlite3_errmsg(_database);
//				IBRCOMMON_LOGGER(error) << error.str() << IBRCOMMON_LOGGER_ENDL;
//				throw SQLiteQueryException(error.str());
//			}
//
//			//Save the Payload in a separate file
//			std::fstream datei(payloadfile.getPath().c_str(), std::ios::in | std::ios::out | std::ios::binary);
//			datei.seekp(bundle._fragmentoffset);
//			{
//				ibrcommon::BLOB::iostream io = payloadBlob.iostream();
//
//				size_t ret = 0;
//				istream &s = (*io);
//				s.seekg(0);
//
//				while (true)
//				{
//					char buf;
//					s.get(buf);
//					if(!s.eof()){
//						datei.put(buf);
//						ret++;
//					}
//					else
//						break;
//				}
//				datei.close();
//			}
//
//			//All fragments received
//			if(bundle._appdatalength == bitcounter)
//			{
//				/*
//				 * 1. Payloadblock zusammensetzen
//				 * 2. PayloadBlock in der BlockTable sichern
//				 * 3. Primary BlockInfos in die BundleTable eintragen.
//				 * 4. Einträge der Fragmenttabelle löschen
//				 */
//
//				// 1. Get the payloadblockobject
//				{
//					ibrcommon::BLOB::Reference ref(ibrcommon::FileBLOB::create(payloadfile));
//					dtn::data::PayloadBlock pb(ref);
//
//					//Copy EID list
//					if(pb.get(dtn::data::PayloadBlock::BLOCK_CONTAINS_EIDS))
//					{
//						std::list<dtn::data::EID> eidlist;
//						std::list<dtn::data::EID>::iterator eidIt;
//						eidlist = block.getEIDList();
//
//						for(eidIt = eidlist.begin(); eidIt != eidlist.end(); eidIt++)
//						{
//							pb.addEID(*eidIt);
//						}
//					}
//
//					//Copy ProcFlags
//					pb.set(dtn::data::PayloadBlock::REPLICATE_IN_EVERY_FRAGMENT, block.get(dtn::data::PayloadBlock::REPLICATE_IN_EVERY_FRAGMENT));
//					pb.set(dtn::data::PayloadBlock::TRANSMIT_STATUSREPORT_IF_NOT_PROCESSED, block.get(dtn::data::PayloadBlock::TRANSMIT_STATUSREPORT_IF_NOT_PROCESSED));
//					pb.set(dtn::data::PayloadBlock::DELETE_BUNDLE_IF_NOT_PROCESSED, block.get(dtn::data::PayloadBlock::DELETE_BUNDLE_IF_NOT_PROCESSED));
//					pb.set(dtn::data::PayloadBlock::LAST_BLOCK, block.get(dtn::data::PayloadBlock::LAST_BLOCK));
//					pb.set(dtn::data::PayloadBlock::DISCARD_IF_NOT_PROCESSED, block.get(dtn::data::PayloadBlock::DISCARD_IF_NOT_PROCESSED));
//					pb.set(dtn::data::PayloadBlock::FORWARDED_WITHOUT_PROCESSED, block.get(dtn::data::PayloadBlock::FORWARDED_WITHOUT_PROCESSED));
//					pb.set(dtn::data::PayloadBlock::BLOCK_CONTAINS_EIDS, block.get(dtn::data::PayloadBlock::BLOCK_CONTAINS_EIDS));
//
//					//2. Save the PayloadBlock to a file and store the metainformation in the BlockTable
//					ibrcommon::TemporaryFile tmpfile(_blockPath, "block");
//
//					std::ofstream datei(tmpfile.getPath().c_str(), std::ios::out | std::ios::binary);
//					dtn::data::SeparateSerializer serializer(datei);
//					serializer << (pb);
//					datei.close();
//
//					filename = tmpfile;
//				}
//
//				sqlite3_bind_text(_statements[BLOCK_STORE], 1,bundleID.c_str(), bundleID.length(), SQLITE_TRANSIENT);
////				sqlite3_bind_int(_statements[BLOCK_STORE], 2, (int)((*it)->getType()));
//				sqlite3_bind_text(_statements[BLOCK_STORE], 3, filename.getPath().c_str(), filename.getPath().length(), SQLITE_TRANSIENT);
//				sqlite3_bind_int(_statements[BLOCK_STORE], 4, 0);
//				executeQuery(_statements[BLOCK_STORE]);
//
//				//3. Delete reassembled Payload data
//				payloadfile.remove();
//
//				//4. Remove Fragment entries from BundleTable
//				sqlite3_bind_text(_statements[BUNDLE_DELETE],1,sourceEID.c_str(), sourceEID.length(),SQLITE_TRANSIENT);
//				sqlite3_bind_int64(_statements[BUNDLE_DELETE],2,bundle._sequencenumber);
//				sqlite3_bind_int64(_statements[BUNDLE_DELETE],3,bundle._timestamp);
//				executeQuery(_statements[BUNDLE_DELETE]);
//
//				//5. Write the Primary Block to the BundleTable
//				size_t procFlags = bundle._procflags;
//				procFlags &= ~(dtn::data::PrimaryBlock::FRAGMENT);
//
//				sqlite3_bind_text(_statements[BUNDLE_STORE], 1, bundleID.c_str(), bundleID.length(),SQLITE_TRANSIENT);
//				sqlite3_bind_text(_statements[BUNDLE_STORE], 2,sourceEID.c_str(), sourceEID.length(),SQLITE_TRANSIENT);
//				sqlite3_bind_text(_statements[BUNDLE_STORE], 3,destination.c_str(), destination.length(),SQLITE_TRANSIENT);
//				sqlite3_bind_text(_statements[BUNDLE_STORE], 4,bundle._reportto.getString().c_str(), bundle._reportto.getString().length(),SQLITE_TRANSIENT);
//				sqlite3_bind_text(_statements[BUNDLE_STORE], 5,bundle._custodian.getString().c_str(), bundle._custodian.getString().length(),SQLITE_TRANSIENT);
//				sqlite3_bind_int(_statements[BUNDLE_STORE],6,procFlags);
//				sqlite3_bind_int64(_statements[BUNDLE_STORE],7,bundle._timestamp);
//				sqlite3_bind_int(_statements[BUNDLE_STORE],8,bundle._sequencenumber);
//				sqlite3_bind_int64(_statements[BUNDLE_STORE],9,bundle._lifetime);
//				sqlite3_bind_int64(_statements[BUNDLE_STORE],10,bundle._fragmentoffset);
//				sqlite3_bind_int64(_statements[BUNDLE_STORE],11,bundle._appdatalength);
//				sqlite3_bind_int64(_statements[BUNDLE_STORE],12,TTL);
//				sqlite3_bind_int(_statements[BUNDLE_STORE],13,NULL);
//				sqlite3_bind_int(_statements[BUNDLE_STORE],14,NULL);
//				sqlite3_bind_text(_statements[BUNDLE_STORE],15,NULL,0,SQLITE_TRANSIENT);
//				executeQuery(_statements[BUNDLE_STORE]);
//			}
//
//			//There are some fragments missing
//			else{
//				sqlite3_bind_text(_statements[BUNDLE_STORE], 1, bundleID.c_str(), bundleID.length(),SQLITE_TRANSIENT);
//				sqlite3_bind_text(_statements[BUNDLE_STORE], 2,sourceEID.c_str(), sourceEID.length(),SQLITE_TRANSIENT);
//				sqlite3_bind_text(_statements[BUNDLE_STORE], 3,destination.c_str(), destination.length(),SQLITE_TRANSIENT);
//				sqlite3_bind_text(_statements[BUNDLE_STORE], 4,bundle._reportto.getString().c_str(), bundle._reportto.getString().length(),SQLITE_TRANSIENT);
//				sqlite3_bind_text(_statements[BUNDLE_STORE], 5,bundle._custodian.getString().c_str(), bundle._custodian.getString().length(),SQLITE_TRANSIENT);
//				sqlite3_bind_int(_statements[BUNDLE_STORE],6,bundle._procflags);
//				sqlite3_bind_int64(_statements[BUNDLE_STORE],7,bundle._timestamp);
//				sqlite3_bind_int(_statements[BUNDLE_STORE],8,bundle._sequencenumber);
//				sqlite3_bind_int64(_statements[BUNDLE_STORE],9,bundle._lifetime);
//				sqlite3_bind_int64(_statements[BUNDLE_STORE],10,bundle._fragmentoffset);
//				sqlite3_bind_int64(_statements[BUNDLE_STORE],11,bundle._appdatalength);
//				sqlite3_bind_int64(_statements[BUNDLE_STORE],12,TTL);
//				sqlite3_bind_int(_statements[BUNDLE_STORE],13,NULL);
//				sqlite3_bind_int(_statements[BUNDLE_STORE],14,payloadsize);
//				sqlite3_bind_text(_statements[BUNDLE_STORE],15,payloadfile.getPath().c_str(),payloadfile.getPath().length(),SQLITE_TRANSIENT);
//				executeQuery(_statements[BUNDLE_STORE]);
//			}
//		}

		void SQLiteBundleStorage::raiseEvent(const Event *evt)
		{
			try {
				const TimeEvent &time = dynamic_cast<const TimeEvent&>(*evt);

				if (time.getAction() == dtn::core::TIME_SECOND_TICK)
				{
					_tasks.push(new TaskExpire(time.getTimestamp()));
				}
			} catch (const std::bad_cast&) { }
			
			try {
				const GlobalEvent &global = dynamic_cast<const GlobalEvent&>(*evt);

				if(global.getAction() == GlobalEvent::GLOBAL_IDLE)
				{
					// switch to idle mode
					ibrcommon::MutexLock l(TaskIdle::_mutex);
					TaskIdle::_idle = true;

					// generate an idle task
					_tasks.push(new TaskIdle());
				}
				else if(global.getAction() == GlobalEvent::GLOBAL_BUSY)
				{
					// switch back to non-idle mode
					ibrcommon::MutexLock l(TaskIdle::_mutex);
					TaskIdle::_idle = false;
				}
			} catch (const std::bad_cast&) { }
		}

		void SQLiteBundleStorage::TaskExpire::run(SQLiteBundleStorage &storage)
		{
			/*
			 * Only if the actual time is bigger or equal than the time when the next bundle expires, deleteexpired is called.
			 */
			if (_timestamp < storage.next_expiration) return;

			/*
			 * Performanceverbesserung: Damit die Abfragen nicht jede Sekunde ausgeführt werden müssen, speichert man den Zeitpunkt an dem
			 * das nächste Bündel gelöscht werden soll in eine Variable und fürhrt deleteexpired erst dann aus wenn ein Bündel abgelaufen ist.
			 * Nach dem Löschen wird die DB durchsucht und der nächste Ablaufzeitpunkt wird in die Variable gesetzt.
			 */

			// query for blocks of expired bundles
			sqlite3_bind_int64(storage._statements[EXPIRE_BUNDLE_FILENAMES], 1, _timestamp);
			while (sqlite3_step(storage._statements[EXPIRE_BUNDLE_FILENAMES]) == SQLITE_ROW)
			{
				ibrcommon::File block((const char*)sqlite3_column_text(storage._statements[EXPIRE_BUNDLE_FILENAMES],0));
				block.remove();
			}
			sqlite3_reset(storage._statements[EXPIRE_BUNDLE_FILENAMES]);

			// query expired bundles
			sqlite3_bind_int64(storage._statements[EXPIRE_BUNDLES], 1, _timestamp);
			while (sqlite3_step(storage._statements[EXPIRE_BUNDLES]) == SQLITE_ROW)
			{
				dtn::data::EID source( (const char*)sqlite3_column_text(storage._statements[EXPIRE_BUNDLES], 0) );
				size_t timestamp = sqlite3_column_int64(storage._statements[EXPIRE_BUNDLES], 1);
				size_t sequence = sqlite3_column_int64(storage._statements[EXPIRE_BUNDLES], 2);
				size_t procflags = sqlite3_column_int64(storage._statements[EXPIRE_BUNDLES], 3);
				size_t offset = sqlite3_column_int64(storage._statements[EXPIRE_BUNDLES], 4);

				dtn::data::BundleID id(source, timestamp, sequence, (procflags & dtn::data::PrimaryBlock::FRAGMENT), offset);
				dtn::core::BundleExpiredEvent::raise(id);
			}
			sqlite3_reset(storage._statements[EXPIRE_BUNDLES]);

			// delete all expired db entries (bundles and blocks)
			sqlite3_bind_int64(storage._statements[EXPIRE_BUNDLE_DELETE], 1, _timestamp);
			sqlite3_step(storage._statements[EXPIRE_BUNDLE_DELETE]);
			sqlite3_reset(storage._statements[EXPIRE_BUNDLE_DELETE]);

			//update deprecated timer
			storage.update_expire_time();
		}

		void SQLiteBundleStorage::update_expire_time()
		{
			int err = sqlite3_step(_statements[EXPIRE_NEXT_TIMESTAMP]);
			if (err == SQLITE_ROW)
				next_expiration = sqlite3_column_int64(_statements[EXPIRE_NEXT_TIMESTAMP], 0);
			sqlite3_reset(_statements[EXPIRE_NEXT_TIMESTAMP]);
		}

		int SQLiteBundleStorage::store_blocks(const data::Bundle &bundle)
		{
			int blocktyp, blocknumber(1), storedBytes(0);

			// get all blocks of the bundle
			const list<const data::Block*> blocklist = bundle.getBlocks();

			// generate a bundle id
			data::BundleID ID(bundle);

			for(std::list<const data::Block*>::const_iterator it = blocklist.begin() ;it != blocklist.end(); it++)
			{
				blocktyp = (int)(*it)->getType();

				ibrcommon::TemporaryFile tmpfile(_blockPath, "block");
				std::ofstream filestream(tmpfile.getPath().c_str(), std::ios_base::out | std::ios::binary);

				dtn::data::SeparateSerializer serializer(filestream);
				serializer << (*(*it));
				storedBytes += serializer.getLength(*(*it));

				filestream.close();

				//put metadata into the storage
				sqlite3_bind_text(_statements[BLOCK_STORE], 1, ID.toString().c_str(), ID.toString().length(), SQLITE_TRANSIENT);
				sqlite3_bind_int(_statements[BLOCK_STORE], 2, blocktyp);
				sqlite3_bind_text(_statements[BLOCK_STORE], 3, tmpfile.getPath().c_str(), tmpfile.getPath().size(), SQLITE_TRANSIENT);
				sqlite3_bind_int(_statements[BLOCK_STORE], 4, blocknumber);
				execute(_statements[BLOCK_STORE]);

				//increment blocknumber
				blocknumber++;
			}

			return storedBytes;
		}

		void SQLiteBundleStorage::get_blocks(dtn::data::Bundle &bundle, const dtn::data::BundleID &id)
		{
			int err = 0;
			string file;
			std::list<ibrcommon::File> files;

			// Get the containing blocks and their filenames
			if (sqlite3_bind_text(_statements[BLOCK_GET_ID], 1, id.toString().c_str(), id.toString().length(), SQLITE_TRANSIENT) != SQLITE_OK)
			{
				IBRCOMMON_LOGGER(error) << "bind error" << IBRCOMMON_LOGGER_ENDL;
			}

			while ((err = sqlite3_step(_statements[BLOCK_GET_ID])) == SQLITE_ROW)
			{
				files.push_back( ibrcommon::File( (const char*) sqlite3_column_text(_statements[BLOCK_GET_ID], 0) ) );
			}

			if (err == SQLITE_DONE)
			{
				if (files.size() == 0)
				{
					IBRCOMMON_LOGGER(error) << "get_blocks: no Blocks found for: " << id.toString() << IBRCOMMON_LOGGER_ENDL;
					throw SQLiteQueryException("no blocks found");
				}
			}
			else
			{
				IBRCOMMON_LOGGER(error) << "get_blocks() failure: "<< err << " " << sqlite3_errmsg(_database) << IBRCOMMON_LOGGER_ENDL;
				throw SQLiteQueryException("can not query for blocks");
			}

			sqlite3_reset(_statements[BLOCK_GET_ID]);

			//De-serialize the Blocks
			for (std::list<ibrcommon::File>::const_iterator it = files.begin(); it != files.end(); it++)
			{
				const ibrcommon::File &f = (*it);

				// open the file
				std::ifstream is(f.getPath().c_str(), std::ios::binary | std::ios::in);

				// read the block
				dtn::data::SeparateDeserializer(is, bundle).readBlock();

				// close the file
				is.close();
			}
		}

		void SQLiteBundleStorage::TaskIdle::run(SQLiteBundleStorage &storage)
		{
			// until IDLE is false
			while (true)
			{
				/*
				 * When an object (table, index, trigger, or view) is dropped from the database, it leaves behind empty space.
				 * This empty space will be reused the next time new information is added to the database. But in the meantime,
				 * the database file might be larger than strictly necessary. Also, frequent inserts, updates, and deletes can
				 * cause the information in the database to become fragmented - scrattered out all across the database file rather
				 * than clustered together in one place.
				 * The VACUUM command cleans the main database. This eliminates free pages, aligns table data to be contiguous,
				 * and otherwise cleans up the database file structure.
				 */
				{
					ibrcommon::MutexLock l(storage._db_mutex);
					sqlite3_step(storage._statements[VACUUM]);
					sqlite3_reset(storage._statements[VACUUM]);
				}

				// here we can do some IDLE stuff...
				::sleep(1);

				ibrcommon::MutexLock l(TaskIdle::_mutex);
				if (!TaskIdle::_idle) return;
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

		void SQLiteBundleStorage::new_expire_time(size_t ttl)
		{
			if (next_expiration == 0 || ttl < next_expiration)
			{
				next_expiration = ttl;
			}
		}
	}
}

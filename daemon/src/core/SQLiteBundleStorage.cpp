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
#include <ibrdtn/data/AgeBlock.h>
#include <ibrdtn/data/Serializer.h>
#include <ibrdtn/data/Bundle.h>
#include <ibrdtn/data/BundleID.h>
#include <ibrdtn/utils/Clock.h>

#include <ibrcommon/thread/MutexLock.h>
#include <ibrcommon/data/BLOB.h>
#include <ibrcommon/AutoDelete.h>
#include <ibrcommon/Logger.h>

namespace dtn
{
	namespace core
	{
		void sql_tracer(void*, const char * pQuery)
		{
			IBRCOMMON_LOGGER_DEBUG(50) << "sqlite trace: " << pQuery << IBRCOMMON_LOGGER_ENDL;
		}

		const std::string SQLiteBundleStorage::_tables[SQL_TABLE_END] =
				{ "bundles", "blocks", "routing", "routing_bundles", "routing_nodes" };

		const std::string SQLiteBundleStorage::_sql_queries[SQL_QUERIES_END] =
		{
			"SELECT source, destination, reportto, custodian, procflags, timestamp, sequencenumber, lifetime, fragmentoffset, appdatalength FROM " + _tables[SQL_TABLE_BUNDLE] + " ORDER BY priority DESC LIMIT ?,?;",
			"SELECT source, destination, reportto, custodian, procflags, timestamp, sequencenumber, lifetime, fragmentoffset, appdatalength FROM "+ _tables[SQL_TABLE_BUNDLE] +" WHERE source_id = ? AND timestamp = ? AND sequencenumber = ? AND fragmentoffset IS NULL LIMIT 1;",
			"SELECT source, destination, reportto, custodian, procflags, timestamp, sequencenumber, lifetime, fragmentoffset, appdatalength FROM "+ _tables[SQL_TABLE_BUNDLE] +" WHERE source_id = ? AND timestamp = ? AND sequencenumber = ? AND fragmentoffset = ? LIMIT 1;",
			"SELECT * FROM " + _tables[SQL_TABLE_BUNDLE] + " WHERE source_id = ? AND timestamp = ? AND sequencenumber = ? AND fragmentoffset != NULL ORDER BY fragmentoffset ASC;",

			"SELECT source, timestamp, sequencenumber, fragmentoffset, procflags FROM "+ _tables[SQL_TABLE_BUNDLE] +" WHERE expiretime <= ?;",
			"SELECT filename FROM "+ _tables[SQL_TABLE_BUNDLE] +" as a, "+ _tables[SQL_TABLE_BLOCK] +" as b WHERE a.source_id = b.source_id AND a.timestamp = b.timestamp AND a.sequencenumber = b.sequencenumber AND a.fragmentoffset = b.fragmentoffset AND a.expiretime <= ?;",
			"DELETE FROM "+ _tables[SQL_TABLE_BUNDLE] +" WHERE expiretime <= ?;",
			"SELECT expiretime FROM "+ _tables[SQL_TABLE_BUNDLE] +" ORDER BY expiretime ASC LIMIT 1;",

			"SELECT ROWID FROM "+ _tables[SQL_TABLE_BUNDLE] +" LIMIT 1;",
			"SELECT COUNT(ROWID) FROM "+ _tables[SQL_TABLE_BUNDLE] +";",

			"DELETE FROM "+ _tables[SQL_TABLE_BUNDLE] +" WHERE source_id = ? AND timestamp = ? AND sequencenumber = ? AND fragmentoffset IS NULL;",
			"DELETE FROM "+ _tables[SQL_TABLE_BUNDLE] +" WHERE source_id = ? AND timestamp = ? AND sequencenumber = ? AND fragmentoffset = ?;",
			"DELETE FROM "+ _tables[SQL_TABLE_BUNDLE] +";",
			"INSERT INTO "+ _tables[SQL_TABLE_BUNDLE] +" (source_id, timestamp, sequencenumber, fragmentoffset, source, destination, reportto, custodian, procflags, lifetime, appdatalength, expiretime, priority) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?);",
			"UPDATE "+ _tables[SQL_TABLE_BUNDLE] +" SET custodian = ? WHERE source_id = ? AND timestamp = ? AND sequencenumber = ? AND fragmentoffset IS NULL;",
			"UPDATE "+ _tables[SQL_TABLE_BUNDLE] +" SET custodian = ? WHERE source_id = ? AND timestamp = ? AND sequencenumber = ? AND fragmentoffset = ?;",

			"UPDATE "+ _tables[SQL_TABLE_BUNDLE] +" SET procflags = ? WHERE source_id = ? AND timestamp = ? AND sequencenumber = ? AND fragmentoffset = ?;",

			"SELECT filename, blocktype FROM "+ _tables[SQL_TABLE_BLOCK] +" WHERE source_id = ? AND timestamp = ? AND sequencenumber = ? AND fragmentoffset IS NULL ORDER BY ordernumber ASC;",
			"SELECT filename, blocktype FROM "+ _tables[SQL_TABLE_BLOCK] +" WHERE source_id = ? AND timestamp = ? AND sequencenumber = ? AND fragmentoffset = ? ORDER BY ordernumber ASC;",
			"SELECT filename, blocktype FROM "+ _tables[SQL_TABLE_BLOCK] +" WHERE source_id = ? AND timestamp = ? AND sequencenumber = ? AND fragmentoffset IS NULL AND ordernumber = ?;",
			"SELECT filename, blocktype FROM "+ _tables[SQL_TABLE_BLOCK] +" WHERE source_id = ? AND timestamp = ? AND sequencenumber = ? AND fragmentoffset = ? AND ordernumber = ?;",
			"DELETE FROM "+ _tables[SQL_TABLE_BLOCK] +";",
			"INSERT INTO "+ _tables[SQL_TABLE_BLOCK] +" (source_id, timestamp, sequencenumber, fragmentoffset, blocktype, filename, ordernumber) VALUES (?,?,?,?,?,?,?);",

#ifdef SQLITE_STORAGE_EXTENDED
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
#endif

			"VACUUM;"
		};

		const std::string SQLiteBundleStorage::_db_structure[10] =
		{
			"CREATE TABLE IF NOT EXISTS `" + _tables[SQL_TABLE_BLOCK] + "` ( `key` INTEGER PRIMARY KEY ASC, `source_id` TEXT NOT NULL, `timestamp` INTEGER NOT NULL, `sequencenumber` INTEGER NOT NULL, `fragmentoffset` INTEGER DEFAULT NULL, `blocktype` INTEGER NOT NULL, `filename` TEXT NOT NULL, `ordernumber` INTEGER NOT NULL);",
			"CREATE TABLE IF NOT EXISTS `" + _tables[SQL_TABLE_BUNDLE] + "` ( `key` INTEGER PRIMARY KEY ASC, `source_id` TEXT NOT NULL, `source` TEXT NOT NULL, `destination` TEXT NOT NULL, `reportto` TEXT NOT NULL, `custodian` TEXT NOT NULL, `procflags` INTEGER NOT NULL, `timestamp` INTEGER NOT NULL, `sequencenumber` INTEGER NOT NULL, `lifetime` INTEGER NOT NULL, `fragmentoffset` INTEGER DEFAULT NULL, `appdatalength` INTEGER DEFAULT NULL, `expiretime` INTEGER NOT NULL, `priority` INTEGER NOT NULL);",
			"create table if not exists "+ _tables[SQL_TABLE_ROUTING] +" (INTEGER PRIMARY KEY ASC, Key int, Routing text);",
			"create table if not exists "+ _tables[SQL_TABLE_BUNDLE_ROUTING_INFO] +" (INTEGER PRIMARY KEY ASC, BundleID text, Key int, Routing text);",
			"create table if not exists "+ _tables[SQL_TABLE_NODE_ROUTING_INFO] +" (INTEGER PRIMARY KEY ASC, EID text, Key int, Routing text);",
			"CREATE TRIGGER IF NOT EXISTS blocks_autodelete AFTER DELETE ON " + _tables[SQL_TABLE_BUNDLE] + " FOR EACH ROW BEGIN DELETE FROM " + _tables[SQL_TABLE_BLOCK] + " WHERE " + _tables[SQL_TABLE_BLOCK] + ".source_id = OLD.source_id AND " + _tables[SQL_TABLE_BLOCK] + ".timestamp = OLD.timestamp AND " + _tables[SQL_TABLE_BLOCK] + ".sequencenumber = OLD.sequencenumber AND ((" + _tables[SQL_TABLE_BLOCK] + ".fragmentoffset IS NULL AND old.fragmentoffset IS NULL) OR (" + _tables[SQL_TABLE_BLOCK] + ".fragmentoffset = old.fragmentoffset)); END;",
			"CREATE UNIQUE INDEX IF NOT EXISTS blocks_bid ON " + _tables[SQL_TABLE_BLOCK] + " (source_id, timestamp, sequencenumber, fragmentoffset);",
			"CREATE INDEX IF NOT EXISTS bundles_destination ON " + _tables[SQL_TABLE_BUNDLE] + " (destination);",
			"CREATE INDEX IF NOT EXISTS bundles_destination_priority ON " + _tables[SQL_TABLE_BUNDLE] + " (destination, priority);",
			"CREATE UNIQUE INDEX IF NOT EXISTS bundles_id ON " + _tables[SQL_TABLE_BUNDLE] + " (source_id, timestamp, sequencenumber, fragmentoffset);"
			"CREATE INDEX IF NOT EXISTS bundles_expire ON " + _tables[SQL_TABLE_BUNDLE] + " (source_id, timestamp, sequencenumber, fragmentoffset, expiretime);"
		};

		ibrcommon::Mutex SQLiteBundleStorage::TaskIdle::_mutex;
		bool SQLiteBundleStorage::TaskIdle::_idle = false;

		SQLiteBundleStorage::SQLBundleQuery::SQLBundleQuery()
		 : _statement(NULL)
		{ }

		SQLiteBundleStorage::SQLBundleQuery::~SQLBundleQuery()
		{
			// free the statement if used before
			if (_statement != NULL)
			{
				sqlite3_finalize(_statement);
			}
		}

		SQLiteBundleStorage::AutoResetLock::AutoResetLock(ibrcommon::Mutex &mutex, sqlite3_stmt *st)
		 : _lock(mutex), _st(st)
		{
			//sqlite3_clear_bindings(st);
		}

		SQLiteBundleStorage::AutoResetLock::~AutoResetLock()
		{
			sqlite3_reset(_st);
		}

		SQLiteBundleStorage::SQLiteBLOB::SQLiteBLOB(const ibrcommon::File &path)
		 : _blobPath(path)
		{
			// generate a new temporary file
			_file = ibrcommon::TemporaryFile(_blobPath, "blob");
		}

		SQLiteBundleStorage::SQLiteBLOB::~SQLiteBLOB()
		{
			// delete the file if the last reference is destroyed
			_file.remove();
		}

		void SQLiteBundleStorage::SQLiteBLOB::clear()
		{
			// close the file
			_filestream.close();

			// remove the old file
			_file.remove();

			// generate a new temporary file
			_file = ibrcommon::TemporaryFile(_blobPath, "blob");

			// open temporary file
			_filestream.open(_file.getPath().c_str(), ios::in | ios::out | ios::trunc | ios::binary );

			if (!_filestream.is_open())
			{
				IBRCOMMON_LOGGER(error) << "can not open temporary file " << _file.getPath() << IBRCOMMON_LOGGER_ENDL;
				throw ibrcommon::CanNotOpenFileException(_file);
			}
		}

		void SQLiteBundleStorage::SQLiteBLOB::open()
		{
			ibrcommon::BLOB::_filelimit.wait();

			// open temporary file
			_filestream.open(_file.getPath().c_str(), ios::in | ios::out | ios::binary );

			if (!_filestream.is_open())
			{
				IBRCOMMON_LOGGER(error) << "can not open temporary file " << _file.getPath() << IBRCOMMON_LOGGER_ENDL;
				throw ibrcommon::CanNotOpenFileException(_file);
			}
		}

		void SQLiteBundleStorage::SQLiteBLOB::close()
		{
			// flush the filestream
			_filestream.flush();

			// close the file
			_filestream.close();

			ibrcommon::BLOB::_filelimit.post();
		}

		size_t SQLiteBundleStorage::SQLiteBLOB::__get_size()
		{
			return _file.size();
		}

		ibrcommon::BLOB::Reference SQLiteBundleStorage::create()
		{
			return ibrcommon::BLOB::Reference(new SQLiteBLOB(_blobPath));
		}

		SQLiteBundleStorage::SQLiteBundleStorage(const ibrcommon::File &path, const size_t &size)
		 : dbPath(path), dbFile(path.get("sqlite.db")), dbSize(size), _next_expiration(0)
		{
			//Configure SQLite Library
			SQLiteConfigure::configure();

			// check if SQLite is thread-safe
			if (sqlite3_threadsafe() == 0)
			{
				IBRCOMMON_LOGGER(critical) << "sqlite library has not compiled with threading support." << IBRCOMMON_LOGGER_ENDL;
				throw ibrcommon::Exception("need threading support in sqlite!");
			}
		}

		SQLiteBundleStorage::~SQLiteBundleStorage()
		{
			stop();
			join();

			ibrcommon::MutexLock l(*this);

			// free all statements
			for (int i = 0; i < SQL_QUERIES_END; i++)
			{
				// prepare the statement
				sqlite3_finalize(_statements[i]);
			}

			//close Databaseconnection
			if (sqlite3_close(_database) != SQLITE_OK)
			{
				IBRCOMMON_LOGGER(error) << "unable to close database" << IBRCOMMON_LOGGER_ENDL;
			}

			// shutdown sqlite library
			SQLiteConfigure::shutdown();
		}

		void SQLiteBundleStorage::openDatabase(const ibrcommon::File &path)
		{
			ibrcommon::MutexLock l(*this);

			// set the block path
			_blockPath = dbPath.get(_tables[SQL_TABLE_BLOCK]);
			_blobPath = dbPath.get("blob");

			//open the database
			if (sqlite3_open_v2(path.getPath().c_str(), &_database,  SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL))
			{
				IBRCOMMON_LOGGER(error) << "Can't open database: " << sqlite3_errmsg(_database) << IBRCOMMON_LOGGER_ENDL;
				sqlite3_close(_database);
				throw ibrcommon::Exception("Unable to open sqlite database");
			}

			int err = 0;

			// create all tables
			for (size_t i = 0; i < 10; i++)
			{
				sqlite3_stmt *st = prepare(_db_structure[i]);
				err = sqlite3_step(st);
				if(err != SQLITE_DONE)
				{
					IBRCOMMON_LOGGER(error) << "SQLiteBundleStorage: failed to create database" << err << IBRCOMMON_LOGGER_ENDL;
				}
				sqlite3_reset(st);
				sqlite3_finalize(st);
			}

			// delete all old BLOB container
			_blobPath.remove(true);

			// create BLOB folder
			ibrcommon::File::createDirectory( _blobPath );

			// create the bundle folder
			ibrcommon::File::createDirectory( _blockPath );

			// prepare all statements
			for (int i = 0; i < SQL_QUERIES_END; i++)
			{
				// prepare the statement
				int err = sqlite3_prepare_v2(_database, _sql_queries[i].c_str(), _sql_queries[i].length(), &_statements[i], 0);
				if ( err != SQLITE_OK )
				{
					IBRCOMMON_LOGGER(error) << "SQLiteBundlestorage: failed to prepare statement: " << err << " with query: " << _sql_queries[i] << IBRCOMMON_LOGGER_ENDL;
				}
			}

			// disable synchronous mode
			sqlite3_exec(_database, "PRAGMA synchronous = OFF;", NULL, NULL, NULL);

			// enable sqlite tracing if debug level is higher than 50
			if (IBRCOMMON_LOGGER_LEVEL >= 50)
			{
				sqlite3_trace(_database, &sql_tracer, NULL);
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
		};

		bool SQLiteBundleStorage::__cancellation()
		{
			_tasks.abort();
			return true;
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

			sqlite3_stmt *getPayloadfiles = prepare("SELECT source, destination, reportto, custodian, procflags, timestamp, sequencenumber, lifetime, fragmentoffset, appdatalength FROM "+_tables[SQL_TABLE_BUNDLE]+" WHERE fragmentoffset != NULL;");

			for(err = sqlite3_step(getPayloadfiles); err == SQLITE_ROW; err = sqlite3_step(getPayloadfiles))
			{
				filename = (const char*)sqlite3_column_text(getPayloadfiles,10);
				file_it = payloadFiles.find(filename);


				if (file_it == payloadFiles.end())
				{
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

			sqlite3_reset(getPayloadfiles);
			sqlite3_finalize(getPayloadfiles);
		}

		void SQLiteBundleStorage::check_bundles(std::set<std::string> &blockFiles)
		{
			std::set<dtn::data::BundleID> corrupt_bundle_ids;

			sqlite3_stmt *blockConistencyCheck = prepare("SELECT source_id, timestamp, sequencenumber, fragmentoffset, filename, ordernumber FROM "+ _tables[SQL_TABLE_BLOCK] +";");

			while (sqlite3_step(blockConistencyCheck) == SQLITE_ROW)
			{
				dtn::data::BundleID id;

				// get the bundleid
				get_bundleid(blockConistencyCheck, id);

				// get the filename
				std::string filename = (const char*)sqlite3_column_text(blockConistencyCheck, 4);

				// if the filename is not in the list of block files
				if (blockFiles.find(filename) == blockFiles.end())
				{
					// add the bundle to the deletion list
					corrupt_bundle_ids.insert(id);
				}
				else
				{
					// file is available, everything is fine
					blockFiles.erase(filename);
				}
			}
			sqlite3_reset(blockConistencyCheck);
			sqlite3_finalize(blockConistencyCheck);

			for(std::set<dtn::data::BundleID>::const_iterator iter = corrupt_bundle_ids.begin(); iter != corrupt_bundle_ids.end(); iter++)
			{
				try {
					const dtn::data::BundleID &id = (*iter);

					// get meta data of this bundle
					const dtn::data::MetaBundle m = get_meta(id);

					// remove the hole bundle
					remove(id);
				} catch (const dtn::core::SQLiteBundleStorage::SQLiteQueryException&) { };
			}

			// delete block files still in the blockfile list
			for (std::set<std::string>::const_iterator iter = blockFiles.begin(); iter != blockFiles.end(); iter++)
			{
				ibrcommon::File blockfile(*iter);
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

			// check if the bundle is already on the deletion list
			if (contains_deletion(id)) throw dtn::core::BundleStorage::NoBundleFoundException();

			size_t stmt_key = BUNDLE_GET_ID;
			if (id.fragment) stmt_key = FRAGMENT_GET_ID;

			// lock the prepared statement
			AutoResetLock l(_locks[stmt_key], _statements[stmt_key]);

			// bind bundle id to the statement
			set_bundleid(_statements[stmt_key], id);

			// execute the query and check for error
			if (sqlite3_step(_statements[stmt_key]) != SQLITE_ROW)
			{
				stringstream error;
				error << "SQLiteBundleStorage: No Bundle found with BundleID: " << id.toString();
				IBRCOMMON_LOGGER_DEBUG(15) << error.str() << IBRCOMMON_LOGGER_ENDL;
				throw SQLiteQueryException(error.str());
			}

			// query bundle data
			get(_statements[stmt_key], bundle);

			return bundle;
		}

		void SQLiteBundleStorage::get(sqlite3_stmt *st, dtn::data::MetaBundle &bundle, size_t offset)
		{
			bundle.source = dtn::data::EID( (const char*) sqlite3_column_text(st, offset + 0) );
			bundle.destination = dtn::data::EID( (const char*) sqlite3_column_text(st, offset + 1) );
			bundle.reportto = dtn::data::EID( (const char*) sqlite3_column_text(st, offset + 2) );
			bundle.custodian = dtn::data::EID( (const char*) sqlite3_column_text(st, offset + 3) );
			bundle.procflags = sqlite3_column_int(st, offset + 4);
			bundle.timestamp = sqlite3_column_int64(st, offset + 5);
			bundle.sequencenumber = sqlite3_column_int64(st, offset + 6);
			bundle.lifetime = sqlite3_column_int64(st, offset + 7);
			bundle.expiretime = dtn::utils::Clock::getExpireTime(bundle.timestamp, bundle.lifetime);

			if (bundle.procflags & data::Bundle::FRAGMENT)
			{
				bundle.offset = sqlite3_column_int64(st, offset + 8);
				bundle.appdatalength = sqlite3_column_int64(st, offset + 9);
			}
		}

		void SQLiteBundleStorage::get(sqlite3_stmt *st, dtn::data::Bundle &bundle, size_t offset)
		{
			bundle._source = dtn::data::EID( (const char*) sqlite3_column_text(st, offset + 0) );
			bundle._destination = dtn::data::EID( (const char*) sqlite3_column_text(st, offset + 1) );
			bundle._reportto = dtn::data::EID( (const char*) sqlite3_column_text(st, offset + 2) );
			bundle._custodian = dtn::data::EID( (const char*) sqlite3_column_text(st, offset + 3) );
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
					"SELECT source, destination, reportto, custodian, procflags, timestamp, sequencenumber, lifetime, fragmentoffset, appdatalength FROM " + _tables[SQL_TABLE_BUNDLE];

			sqlite3_stmt *st = NULL;
			size_t bind_offset = 1;

			try {
				SQLBundleQuery &query = dynamic_cast<SQLBundleQuery&>(cb);

				if (query._statement == NULL)
				{
					std::string sql = base_query + " WHERE " + query.getWhere() + " ORDER BY priority DESC";

					if (cb.limit() > 0)
					{
						sql += " LIMIT ?,?";
					}

					// add closing delimiter
					sql += ";";

					// prepare the hole statement
					query._statement = prepare(sql);
				}

				st = query._statement;
				bind_offset = query.bind(st, 1);
			} catch (const std::bad_cast&) {
				// this query is not optimized for sql, use the generic way (slow)
				st = _statements[BUNDLE_GET_FILTER];
			};

			size_t offset = 0;
			while (true)
			{
				// lock the database
				AutoResetLock l(_locks[BUNDLE_GET_FILTER], st);

				if (cb.limit() > 0)
				{
					sqlite3_bind_int64(st, bind_offset, offset);
					sqlite3_bind_int64(st, bind_offset + 1, cb.limit());
				}

				if (sqlite3_step(st) == SQLITE_DONE)
				{
					// returned no result
					break;
				}

				while (true)
				{
					dtn::data::MetaBundle m;

					// extract the primary values and set them in the bundle object
					get(st, m, 0);

					// check if the bundle is already on the deletion list
					if (!contains_deletion(m))
					{
						// ask the filter if this bundle should be added to the return list
						if (cb.shouldAdd(m))
						{
							IBRCOMMON_LOGGER_DEBUG(40) << "add bundle to query selection list: " << m.toString() << IBRCOMMON_LOGGER_ENDL;

							// add the bundle to the list
							ret.push_back(m);
						}
					}

					if (sqlite3_step(st) != SQLITE_ROW)
					{
						break;
					}

					// abort if enough bundles are found
					if ((cb.limit() > 0) && (ret.size() >= cb.limit())) break;
				}

				// if a limit is set
				if (cb.limit() > 0)
				{
					// increment the offset, because we might not have enough
					offset += cb.limit();

					// abort if enough bundles are found
					if (ret.size() >= cb.limit()) break;
				}
				else
				{
					break;
				}
			}

			return ret;
		}

		dtn::data::Bundle SQLiteBundleStorage::get(const dtn::data::BundleID &id)
		{
			dtn::data::Bundle bundle;
			int err = 0;

			IBRCOMMON_LOGGER_DEBUG(25) << "get bundle from sqlite storage " << id.toString() << IBRCOMMON_LOGGER_ENDL;

			// if bundle is deleted?
			if (contains_deletion(id)) throw dtn::core::BundleStorage::NoBundleFoundException();

			size_t stmt_key = BUNDLE_GET_ID;
			if (id.fragment) stmt_key = FRAGMENT_GET_ID;

			// do this while db is locked
			AutoResetLock l(_locks[stmt_key], _statements[stmt_key]);

			// set the bundle key values
			set_bundleid(_statements[stmt_key], id);

			// execute the query and check for error
			if ((err = sqlite3_step(_statements[stmt_key])) != SQLITE_ROW)
			{
				IBRCOMMON_LOGGER_DEBUG(15) << "sql error: " << err << "; No bundle found with id: " << id.toString() << IBRCOMMON_LOGGER_ENDL;
				throw dtn::core::BundleStorage::NoBundleFoundException();
			}

			// read bundle data
			get(_statements[stmt_key], bundle);

			try {
				// read all blocks
				get_blocks(bundle, id);
			} catch (const ibrcommon::Exception &ex) {
				IBRCOMMON_LOGGER(error) << "could not get bundle blocks: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
				throw dtn::core::BundleStorage::NoBundleFoundException();
			}

			return bundle;
		}

#ifdef SQLITE_STORAGE_EXTENDED
		void SQLiteBundleStorage::setPriority(const int priority, const dtn::data::BundleID &id)
		{
			int err;
			size_t procflags;
			{
				sqlite3_bind_text(_statements[PROCFLAGS_GET],1,id.toString().c_str(), id.toString().length(), SQLITE_TRANSIENT);
				err = sqlite3_step(_statements[PROCFLAGS_GET]);
				if(err == SQLITE_ROW)
					procflags = sqlite3_column_int(_statements[PROCFLAGS_GET],0);
				sqlite3_reset(_statements[PROCFLAGS_GET]);
				if(err != SQLITE_DONE){
					stringstream error;
					error << "SQLiteBundleStorage: error while Select Querry: " << err;
					IBRCOMMON_LOGGER(error) << error.str() << IBRCOMMON_LOGGER_ENDL;
					throw SQLiteQueryException(error.str());
				}

				//Priority auf 0 Setzen
				procflags -= ( 128*(procflags & dtn::data::Bundle::PRIORITY_BIT1) + 256*(procflags & dtn::data::Bundle::PRIORITY_BIT2));

				//Set Prioritybits
				switch(priority){
				case 1: procflags += data::Bundle::PRIORITY_BIT1; break; //Set PRIORITY_BIT1
				case 2: procflags += data::Bundle::PRIORITY_BIT2; break;
				case 3: procflags += (data::Bundle::PRIORITY_BIT1 + data::Bundle::PRIORITY_BIT2); break;
				}

				sqlite3_bind_text(_statements[PROCFLAGS_SET],1,id.toString().c_str(), id.toString().length(), SQLITE_TRANSIENT);
				sqlite3_bind_int64(_statements[PROCFLAGS_SET],2,priority);
				sqlite3_step(_statements[PROCFLAGS_SET]);
				sqlite3_reset(_statements[PROCFLAGS_SET]);

				if(err != SQLITE_DONE)
				{
					stringstream error;
					error << "SQLiteBundleStorage: setPriority error while Select Querry: " << err;
					IBRCOMMON_LOGGER(error) << error.str() << IBRCOMMON_LOGGER_ENDL;
					throw SQLiteQueryException(error.str());
				}
			}
		}
#endif

		void SQLiteBundleStorage::store(const dtn::data::Bundle &bundle)
		{
			IBRCOMMON_LOGGER_DEBUG(25) << "store bundle " << bundle.toString() << IBRCOMMON_LOGGER_ENDL;

			// determine if the bundle is for local delivery
			bool local = (bundle._destination.sameHost(BundleCore::local)
					&& (bundle._procflags & dtn::data::PrimaryBlock::DESTINATION_IS_SINGLETON));

//			// if the bundle is a fragment store it as one
//			if (bundle.get(dtn::data::Bundle::FRAGMENT) && local)
//			{
//				storeFragment(bundle);
//				return;
//			}

			int err;

			const dtn::data::EID _sourceid = bundle._source;
			size_t TTL = bundle._timestamp + bundle._lifetime;

			AutoResetLock l(_locks[BUNDLE_STORE], _statements[BUNDLE_STORE]);

			set_bundleid(_statements[BUNDLE_STORE], bundle);

			sqlite3_bind_text(_statements[BUNDLE_STORE], 5, bundle._source.getString().c_str(), bundle._source.getString().length(), SQLITE_TRANSIENT);
			sqlite3_bind_text(_statements[BUNDLE_STORE], 6, bundle._destination.getString().c_str(), bundle._destination.getString().length(), SQLITE_TRANSIENT);
			sqlite3_bind_text(_statements[BUNDLE_STORE], 7, bundle._reportto.getString().c_str(), bundle._reportto.getString().length(), SQLITE_TRANSIENT);
			sqlite3_bind_text(_statements[BUNDLE_STORE], 8, bundle._custodian.getString().c_str(), bundle._custodian.getString().length(), SQLITE_TRANSIENT);
			sqlite3_bind_int(_statements[BUNDLE_STORE], 9, bundle._procflags);
			sqlite3_bind_int64(_statements[BUNDLE_STORE], 10, bundle._lifetime);

			if (bundle.get(dtn::data::Bundle::FRAGMENT))
			{
				sqlite3_bind_int64(_statements[BUNDLE_STORE], 11, bundle._appdatalength);
			}
			else
			{
				sqlite3_bind_null(_statements[BUNDLE_STORE], 4);
				sqlite3_bind_null(_statements[BUNDLE_STORE], 11);
			}

			sqlite3_bind_int64(_statements[BUNDLE_STORE], 12, TTL);
			sqlite3_bind_int64(_statements[BUNDLE_STORE], 13, dtn::data::MetaBundle(bundle).getPriority());

			// start a transaction
			sqlite3_exec(_database, "BEGIN TRANSACTION;", NULL, NULL, NULL);

			// store the bundle data in the database
			err = sqlite3_step(_statements[BUNDLE_STORE]);

			if (err == SQLITE_CONSTRAINT)
			{
				IBRCOMMON_LOGGER(warning) << "Bundle is already in the storage" << IBRCOMMON_LOGGER_ENDL;
			}
			else if (err != SQLITE_DONE)
			{
				stringstream error;
				error << "SQLiteBundleStorage: store() failure: " << err << " " <<  sqlite3_errmsg(_database);
				IBRCOMMON_LOGGER(error) << error.str() << IBRCOMMON_LOGGER_ENDL;

				// rollback the whole transaction
				sqlite3_exec(_database, "ROLLBACK TRANSACTION;", NULL, NULL, NULL);

				throw SQLiteQueryException(error.str());
			}
			else
			{
				// stores the blocks to a file
				store_blocks(bundle);

				IBRCOMMON_LOGGER_DEBUG(10) << "bundle " << bundle.toString() << " stored" << IBRCOMMON_LOGGER_ENDL;
			}

			// commit the transaction
			sqlite3_exec(_database, "END TRANSACTION;", NULL, NULL, NULL);

			// set new expire time
			new_expire_time(TTL);

			try {
				// the bundle is stored sucessfully, we could accept custody if it is requested
				const dtn::data::EID custodian = acceptCustody(bundle);

				// update the custody address of this bundle
				update_custodian(bundle, custodian);
			} catch (const ibrcommon::Exception&) {
				// this bundle has no request for custody transfers
			}
		}

		void SQLiteBundleStorage::remove(const dtn::data::BundleID &id)
		{
			add_deletion(id);
			_tasks.push(new TaskRemove(id));
		}

		void SQLiteBundleStorage::TaskRemove::run(SQLiteBundleStorage &storage)
		{
			{
				// select the right statement to use
				const size_t stmt_key = _id.fragment ? BLOCK_GET_ID_FRAGMENT : BLOCK_GET_ID;

				// lock the database
				AutoResetLock l(storage._locks[stmt_key], storage._statements[stmt_key]);

				// set the bundle key values
				storage.set_bundleid(storage._statements[stmt_key], _id);

				// step through all blocks
				while (sqlite3_step(storage._statements[stmt_key]) == SQLITE_ROW)
				{
					// delete each referenced block file
					ibrcommon::File blockfile( (const char*)sqlite3_column_text(storage._statements[stmt_key], 0) );
					blockfile.remove();
				}
			}

			{
				const size_t stmt_key = _id.fragment ? FRAGMENT_DELETE : BUNDLE_DELETE;

				// lock the database
				AutoResetLock l(storage._locks[stmt_key], storage._statements[stmt_key]);

				// then remove the bundle data
				storage.set_bundleid(storage._statements[stmt_key], _id);
				sqlite3_step(storage._statements[stmt_key]);

				IBRCOMMON_LOGGER_DEBUG(10) << "bundle " << _id.toString() << " deleted" << IBRCOMMON_LOGGER_ENDL;
			}

			//update deprecated timer
			storage.update_expire_time();

			// remove it from the deletion list
			storage.remove_deletion(_id);
		}

#ifdef SQLITE_STORAGE_EXTENDED
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
#endif

		void SQLiteBundleStorage::clearAll()
		{
#ifdef SQLITE_STORAGE_EXTENDED
			sqlite3_step(_statements[ROUTING_CLEAR]);
			sqlite3_reset(_statements[ROUTING_CLEAR]);
			sqlite3_step(_statements[NODE_CLEAR]);
			sqlite3_reset(_statements[NODE_CLEAR]);
#endif

			clear();
		}

		void SQLiteBundleStorage::clear()
		{
			AutoResetLock l(_locks[VACUUM], _statements[VACUUM]);

			sqlite3_step(_statements[BUNDLE_CLEAR]);
			sqlite3_reset(_statements[BUNDLE_CLEAR]);
			sqlite3_step(_statements[BLOCK_CLEAR]);
			sqlite3_reset(_statements[BLOCK_CLEAR]);

			if(SQLITE_DONE != sqlite3_step(_statements[VACUUM]))
			{
				sqlite3_reset(_statements[VACUUM]);
				throw SQLiteQueryException("SQLiteBundleStore: clear(): vacuum failed.");
			}

			//Delete Folder SQL_TABLE_BLOCK containing Blocks
			{
				_blockPath.remove(true);
				ibrcommon::File::createDirectory(_blockPath);
			}

			reset_expire_time();
		}



		bool SQLiteBundleStorage::empty()
		{
			AutoResetLock l(_locks[EMPTY_CHECK], _statements[EMPTY_CHECK]);

			if (SQLITE_DONE == sqlite3_step(_statements[EMPTY_CHECK]))
			{
				return true;
			}
			else
			{
				return false;
			}
		}

		unsigned int SQLiteBundleStorage::count()
		{
			int rows = 0;
			int err = 0;

			AutoResetLock l(_locks[COUNT_ENTRIES], _statements[COUNT_ENTRIES]);

			if ((err = sqlite3_step(_statements[COUNT_ENTRIES])) == SQLITE_ROW)
			{
				rows = sqlite3_column_int(_statements[COUNT_ENTRIES], 0);
			}
			else
			{
				stringstream error;
				error << "SQLiteBundleStorage: count: failure " << err << " " << sqlite3_errmsg(_database);
				IBRCOMMON_LOGGER(error) << error.str() << IBRCOMMON_LOGGER_ENDL;
				throw SQLiteQueryException(error.str());
			}

			return rows;
		}

#ifdef SQLITE_STORAGE_EXTENDED
		int SQLiteBundleStorage::occupiedSpace()
		{
			int size = 0;
			std::list<ibrcommon::File> files;

			if (_blockPath.getFiles(files))
			{
				IBRCOMMON_LOGGER(error) << "occupiedSpace: unable to open Directory " << _blockPath.getPath() << IBRCOMMON_LOGGER_ENDL;
				return -1;
			}

			for (std::list<ibrcommon::File>::const_iterator iter = files.begin(); iter != files.end(); iter++)
			{
				const ibrcommon::File &f = (*iter);

				if (!f.isSystem())
				{
					size += f.size();
				}
			}

			// add db file size
			size += dbFile.size();

			return size;
		}

		void SQLiteBundleStorage::storeFragment(const dtn::data::Bundle &bundle)
		{
			int err = 0;
			size_t bitcounter = 0;
			bool allFragmentsReceived(true);
			size_t payloadsize, TTL, fragmentoffset;

			ibrcommon::File payloadfile, filename;

			// generate a bundle id string
			std::string bundleID = dtn::data::BundleID(bundle).toString();

			// get the destination id string
			std::string destination = bundle._destination.getString();

			// get the source id string
			std::string sourceEID = bundle._source.getString();

			// calculate the expiration timestamp
			TTL = bundle._timestamp + bundle._lifetime;

			const dtn::data::PayloadBlock &block = bundle.getBlock<dtn::data::PayloadBlock>();
			ibrcommon::BLOB::Reference payloadBlob = block.getBLOB();

			// get the payload size
			{
				ibrcommon::BLOB::iostream io = payloadBlob.iostream();
				payloadsize = io.size();
			}

			//Saves the blocks from the first and the last Fragment, they may contain Extentionblock
			bool last = bundle._fragmentoffset + payloadsize == bundle._appdatalength;
			bool first = bundle._fragmentoffset == 0;

			if(first || last)
			{
				int i = 0, blocknumber = 0;

				// get the list of blocks
				std::list<const dtn::data::Block*> blocklist = bundle.getBlocks();

				// iterate through all blocks
				for (std::list<const dtn::data::Block* >::const_iterator it = blocklist.begin(); it != blocklist.end(); it++, i++)
				{
					// generate a temporary block file
					ibrcommon::TemporaryFile tmpfile(_blockPath, "block");

					ofstream datei(tmpfile.getPath().c_str(), std::ios::out | std::ios::binary);

					dtn::data::SeparateSerializer serializer(datei);
					serializer << (*(*it));
					datei.close();

					/* Schreibt Blöcke in die DB.
					 * Die Blocknummern werden nach folgendem Schema in die DB geschrieben:
					 * 		Die Blöcke des ersten Fragments fangen bei -x an und gehen bis -1
					 * 		Die Blöcke des letzten Fragments beginnen bei 1 und gehen bis x
					 * 		Der Payloadblock wird an Stelle 0 eingefügt.
					 */
					sqlite3_bind_text(_statements[BLOCK_STORE], 1, bundleID.c_str(), bundleID.length(), SQLITE_TRANSIENT);
					sqlite3_bind_int(_statements[BLOCK_STORE], 2, (int)((*it)->getType()));
					sqlite3_bind_text(_statements[BLOCK_STORE], 3, tmpfile.getPath().c_str(), tmpfile.getPath().size(), SQLITE_TRANSIENT);

					blocknumber = blocklist.size() - i;

					if (first)
					{
						blocknumber = (blocklist.size() - i)*(-1);
					}

					sqlite3_bind_int(_statements[BLOCK_STORE],4,blocknumber);
					executeQuery(_statements[BLOCK_STORE]);
				}
			}

			//At first the database is checked for already received bundles and some
			//informations are read, which effect the following program flow.
			sqlite3_bind_text(_statements[BUNDLE_GET_FRAGMENT],1,sourceEID.c_str(),sourceEID.length(),SQLITE_TRANSIENT);
			sqlite3_bind_int64(_statements[BUNDLE_GET_FRAGMENT],2,bundle._timestamp);
			sqlite3_bind_int(_statements[BUNDLE_GET_FRAGMENT],3,bundle._sequencenumber);
			err = sqlite3_step(_statements[BUNDLE_GET_FRAGMENT]);

			//Es ist noch kein Eintrag vorhanden: generiere Payloadfilename
			if(err == SQLITE_DONE)
			{
				// generate a temporary block file
				ibrcommon::TemporaryFile tmpfile(_blockPath, "block");
				payloadfile = tmpfile;
			}
			//Es ist bereits ein eintrag vorhanden: speicher den payloadfilename
			else if (err == SQLITE_ROW)
			{
				payloadfile = ibrcommon::File( (const char*)sqlite3_column_text(_statements[BUNDLE_GET_FRAGMENT], 14) );				//14 = Payloadfilename
			}

			while (err == SQLITE_ROW){
				//The following codepice summs the payloadfragmentbytes, to check if all fragments were received
				//and the bundle is complete.
				fragmentoffset = sqlite3_column_int(_statements[BUNDLE_GET_FRAGMENT],9);								// 9 = fragmentoffset

				if(bitcounter >= fragmentoffset){
					bitcounter += fragmentoffset - bitcounter + sqlite3_column_int(_statements[BUNDLE_GET_FRAGMENT],13);	//13 = payloadlength
				}
				else if(bitcounter >= bundle._fragmentoffset){
					bitcounter += bundle._fragmentoffset - bitcounter + payloadsize;
				}
				else{
					allFragmentsReceived = false;
				}
				err = sqlite3_step(_statements[BUNDLE_GET_FRAGMENT]);
			}

			if(bitcounter +1 >= bundle._fragmentoffset && allFragmentsReceived) //Wenn das aktuelle fragment das größte ist, ist die Liste durchlaufen und das Frag nicht dazugezählt. Dies wird hier nachgeholt.
			{
				bitcounter += bundle._fragmentoffset - bitcounter + payloadsize;
			}

			sqlite3_reset(_statements[BUNDLE_GET_FRAGMENT]);
			if(err != SQLITE_DONE){
				stringstream error;
				error << "SQLiteBundleStorage: storeFragment: " << err << " errmsg: " << sqlite3_errmsg(_database);
				IBRCOMMON_LOGGER(error) << error.str() << IBRCOMMON_LOGGER_ENDL;
				throw SQLiteQueryException(error.str());
			}

			//Save the Payload in a separate file
			std::fstream datei(payloadfile.getPath().c_str(), std::ios::in | std::ios::out | std::ios::binary);
			datei.seekp(bundle._fragmentoffset);
			{
				ibrcommon::BLOB::iostream io = payloadBlob.iostream();

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
			if(bundle._appdatalength == bitcounter)
			{
				/*
				 * 1. Payloadblock zusammensetzen
				 * 2. PayloadBlock in der BlockTable sichern
				 * 3. Primary BlockInfos in die BundleTable eintragen.
				 * 4. Einträge der Fragmenttabelle löschen
				 */

				// 1. Get the payloadblockobject
				{
					ibrcommon::BLOB::Reference ref(ibrcommon::FileBLOB::create(payloadfile));
					dtn::data::PayloadBlock pb(ref);

					//Copy EID list
					if(pb.get(dtn::data::PayloadBlock::BLOCK_CONTAINS_EIDS))
					{
						std::list<dtn::data::EID> eidlist;
						std::list<dtn::data::EID>::iterator eidIt;
						eidlist = block.getEIDList();

						for(eidIt = eidlist.begin(); eidIt != eidlist.end(); eidIt++)
						{
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
					ibrcommon::TemporaryFile tmpfile(_blockPath, "block");

					std::ofstream datei(tmpfile.getPath().c_str(), std::ios::out | std::ios::binary);
					dtn::data::SeparateSerializer serializer(datei);
					serializer << (pb);
					datei.close();

					filename = tmpfile;
				}

				sqlite3_bind_text(_statements[BLOCK_STORE], 1,bundleID.c_str(), bundleID.length(), SQLITE_TRANSIENT);
//				sqlite3_bind_int(_statements[BLOCK_STORE], 2, (int)((*it)->getType()));
				sqlite3_bind_text(_statements[BLOCK_STORE], 3, filename.getPath().c_str(), filename.getPath().length(), SQLITE_TRANSIENT);
				sqlite3_bind_int(_statements[BLOCK_STORE], 4, 0);
				executeQuery(_statements[BLOCK_STORE]);

				//3. Delete reassembled Payload data
				payloadfile.remove();

				//4. Remove Fragment entries from BundleTable
				sqlite3_bind_text(_statements[BUNDLE_DELETE],1,sourceEID.c_str(), sourceEID.length(),SQLITE_TRANSIENT);
				sqlite3_bind_int64(_statements[BUNDLE_DELETE],2,bundle._sequencenumber);
				sqlite3_bind_int64(_statements[BUNDLE_DELETE],3,bundle._timestamp);
				executeQuery(_statements[BUNDLE_DELETE]);

				//5. Write the Primary Block to the BundleTable
				size_t procFlags = bundle._procflags;
				procFlags &= ~(dtn::data::PrimaryBlock::FRAGMENT);

				sqlite3_bind_text(_statements[BUNDLE_STORE], 1, bundleID.c_str(), bundleID.length(),SQLITE_TRANSIENT);
				sqlite3_bind_text(_statements[BUNDLE_STORE], 2,sourceEID.c_str(), sourceEID.length(),SQLITE_TRANSIENT);
				sqlite3_bind_text(_statements[BUNDLE_STORE], 3,destination.c_str(), destination.length(),SQLITE_TRANSIENT);
				sqlite3_bind_text(_statements[BUNDLE_STORE], 4,bundle._reportto.getString().c_str(), bundle._reportto.getString().length(),SQLITE_TRANSIENT);
				sqlite3_bind_text(_statements[BUNDLE_STORE], 5,bundle._custodian.getString().c_str(), bundle._custodian.getString().length(),SQLITE_TRANSIENT);
				sqlite3_bind_int(_statements[BUNDLE_STORE],6,procFlags);
				sqlite3_bind_int64(_statements[BUNDLE_STORE],7,bundle._timestamp);
				sqlite3_bind_int(_statements[BUNDLE_STORE],8,bundle._sequencenumber);
				sqlite3_bind_int64(_statements[BUNDLE_STORE],9,bundle._lifetime);
				sqlite3_bind_int64(_statements[BUNDLE_STORE],10,bundle._fragmentoffset);
				sqlite3_bind_int64(_statements[BUNDLE_STORE],11,bundle._appdatalength);
				sqlite3_bind_int64(_statements[BUNDLE_STORE],12,TTL);
				sqlite3_bind_int(_statements[BUNDLE_STORE],13,NULL);
				sqlite3_bind_int(_statements[BUNDLE_STORE],14,NULL);
				sqlite3_bind_text(_statements[BUNDLE_STORE],15,NULL,0,SQLITE_TRANSIENT);
				executeQuery(_statements[BUNDLE_STORE]);
			}

			//There are some fragments missing
			else{
				sqlite3_bind_text(_statements[BUNDLE_STORE], 1, bundleID.c_str(), bundleID.length(),SQLITE_TRANSIENT);
				sqlite3_bind_text(_statements[BUNDLE_STORE], 2,sourceEID.c_str(), sourceEID.length(),SQLITE_TRANSIENT);
				sqlite3_bind_text(_statements[BUNDLE_STORE], 3,destination.c_str(), destination.length(),SQLITE_TRANSIENT);
				sqlite3_bind_text(_statements[BUNDLE_STORE], 4,bundle._reportto.getString().c_str(), bundle._reportto.getString().length(),SQLITE_TRANSIENT);
				sqlite3_bind_text(_statements[BUNDLE_STORE], 5,bundle._custodian.getString().c_str(), bundle._custodian.getString().length(),SQLITE_TRANSIENT);
				sqlite3_bind_int(_statements[BUNDLE_STORE],6,bundle._procflags);
				sqlite3_bind_int64(_statements[BUNDLE_STORE],7,bundle._timestamp);
				sqlite3_bind_int(_statements[BUNDLE_STORE],8,bundle._sequencenumber);
				sqlite3_bind_int64(_statements[BUNDLE_STORE],9,bundle._lifetime);
				sqlite3_bind_int64(_statements[BUNDLE_STORE],10,bundle._fragmentoffset);
				sqlite3_bind_int64(_statements[BUNDLE_STORE],11,bundle._appdatalength);
				sqlite3_bind_int64(_statements[BUNDLE_STORE],12,TTL);
				sqlite3_bind_int(_statements[BUNDLE_STORE],13,NULL);
				sqlite3_bind_int(_statements[BUNDLE_STORE],14,payloadsize);
				sqlite3_bind_text(_statements[BUNDLE_STORE],15,payloadfile.getPath().c_str(),payloadfile.getPath().length(),SQLITE_TRANSIENT);
				executeQuery(_statements[BUNDLE_STORE]);
			}
		}
#endif

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
			size_t exp_time = storage.get_expire_time();
			if ((_timestamp < exp_time) || (exp_time == 0)) return;

			/*
			 * Performanceverbesserung: Damit die Abfragen nicht jede Sekunde ausgeführt werden müssen, speichert man den Zeitpunkt an dem
			 * das nächste Bündel gelöscht werden soll in eine Variable und fürhrt deleteexpired erst dann aus wenn ein Bündel abgelaufen ist.
			 * Nach dem Löschen wird die DB durchsucht und der nächste Ablaufzeitpunkt wird in die Variable gesetzt.
			 */

			{
				AutoResetLock l(storage._locks[EXPIRE_BUNDLE_FILENAMES], storage._statements[EXPIRE_BUNDLE_FILENAMES]);

				// query for blocks of expired bundles
				sqlite3_bind_int64(storage._statements[EXPIRE_BUNDLE_FILENAMES], 1, _timestamp);
				while (sqlite3_step(storage._statements[EXPIRE_BUNDLE_FILENAMES]) == SQLITE_ROW)
				{
					ibrcommon::File block((const char*)sqlite3_column_text(storage._statements[EXPIRE_BUNDLE_FILENAMES],0));
					block.remove();
				}
			}

			{
				AutoResetLock l(storage._locks[EXPIRE_BUNDLES], storage._statements[EXPIRE_BUNDLES]);

				// query expired bundles
				sqlite3_bind_int64(storage._statements[EXPIRE_BUNDLES], 1, _timestamp);
				while (sqlite3_step(storage._statements[EXPIRE_BUNDLES]) == SQLITE_ROW)
				{
					dtn::data::BundleID id;
					storage.get_bundleid(storage._statements[EXPIRE_BUNDLES], id);
					dtn::core::BundleExpiredEvent::raise(id);
				}
			}

			{
				AutoResetLock l(storage._locks[EXPIRE_BUNDLE_DELETE], storage._statements[EXPIRE_BUNDLE_DELETE]);

				// delete all expired db entries (bundles and blocks)
				sqlite3_bind_int64(storage._statements[EXPIRE_BUNDLE_DELETE], 1, _timestamp);
				sqlite3_step(storage._statements[EXPIRE_BUNDLE_DELETE]);
			}

			//update deprecated timer
			storage.update_expire_time();
		}

		void SQLiteBundleStorage::update_expire_time()
		{
			ibrcommon::MutexLock l(_locks[EXPIRE_NEXT_TIMESTAMP]);
			int err = sqlite3_step(_statements[EXPIRE_NEXT_TIMESTAMP]);

			if (err == SQLITE_ROW)
			{
				ibrcommon::MutexLock l(_next_expiration_lock);
				_next_expiration = sqlite3_column_int64(_statements[EXPIRE_NEXT_TIMESTAMP], 0);
			}
			else
			{
				ibrcommon::MutexLock l(_next_expiration_lock);
				_next_expiration = 0;
			}

			sqlite3_reset(_statements[EXPIRE_NEXT_TIMESTAMP]);
		}

		void SQLiteBundleStorage::update_custodian(const dtn::data::BundleID &id, const dtn::data::EID &custodian)
		{
			// select query with or without fragmentation extension
			STORAGE_STMT query = BUNDLE_UPDATE_CUSTODIAN;
			if (id.fragment)	query = FRAGMENT_UPDATE_CUSTODIAN;


			AutoResetLock l(_locks[query], _statements[query]);

			sqlite3_bind_text(_statements[query], 1, custodian.getString().c_str(), custodian.getString().length(), SQLITE_TRANSIENT);
			set_bundleid(_statements[query], id, 1);

			// update the custodian in the database
			int err = sqlite3_step(_statements[query]);

			if (err != SQLITE_DONE)
			{
				stringstream error;
				error << "SQLiteBundleStorage: update_custodian() failure: " << err << " " <<  sqlite3_errmsg(_database);
				IBRCOMMON_LOGGER(error) << error.str() << IBRCOMMON_LOGGER_ENDL;
			}
		}

		int SQLiteBundleStorage::store_blocks(const data::Bundle &bundle)
		{
			int blocktyp, blocknumber(1), storedBytes(0);

			// get all blocks of the bundle
			const list<const dtn::data::Block*> blocklist = bundle.getBlocks();

			// generate a bundle id
			dtn::data::BundleID id(bundle);

			for(std::list<const dtn::data::Block*>::const_iterator it = blocklist.begin() ;it != blocklist.end(); it++)
			{
				const dtn::data::Block &block = (**it);
				blocktyp = (int)block.getType();

				ibrcommon::TemporaryFile tmpfile(_blockPath, "block");

				try {
					try {
						const dtn::data::PayloadBlock &payload = dynamic_cast<const dtn::data::PayloadBlock&>(block);
						ibrcommon::BLOB::Reference ref = payload.getBLOB();
						ibrcommon::BLOB::iostream stream = ref.iostream();

						const SQLiteBLOB &blob = dynamic_cast<const SQLiteBLOB&>(*ref.getPointer());

						// first remove the tmp file
						tmpfile.remove();

						// make a hardlink to the origin blob file
						if ( ::link(blob._file.getPath().c_str(), tmpfile.getPath().c_str()) != 0 )
						{
							tmpfile = ibrcommon::TemporaryFile(_blockPath, "block");
							throw ibrcommon::Exception("hard-link failed");
						}
					} catch (const std::bad_cast&) {
						throw ibrcommon::Exception("not a Payload or SQLiteBLOB");
					}

					storedBytes += _blockPath.size();
				} catch (const ibrcommon::Exception&) {
					std::ofstream filestream(tmpfile.getPath().c_str(), std::ios_base::out | std::ios::binary);
					dtn::data::SeparateSerializer serializer(filestream);
					serializer << block;
					storedBytes += serializer.getLength(block);
					filestream.close();
				}

				// protect this query from concurrent access and enable the auto-reset feature
				AutoResetLock l(_locks[BLOCK_STORE], _statements[BLOCK_STORE]);

				// set bundle key data
				set_bundleid(_statements[BLOCK_STORE], id);

				// set the column four to null if this is not a fragment
				if (!id.fragment) sqlite3_bind_null(_statements[BLOCK_STORE], 4);

				// set the block type
				sqlite3_bind_int(_statements[BLOCK_STORE], 5, blocktyp);

				// the filename of the block data
				sqlite3_bind_text(_statements[BLOCK_STORE], 6, tmpfile.getPath().c_str(), tmpfile.getPath().size(), SQLITE_TRANSIENT);

				// the ordering number
				sqlite3_bind_int(_statements[BLOCK_STORE], 7, blocknumber);

				// execute the query and store the block in the database
				if (sqlite3_step(_statements[BLOCK_STORE]) != SQLITE_DONE)
				{
					throw SQLiteQueryException("can not store block of bundle");
				}

				//increment blocknumber
				blocknumber++;
			}

			return storedBytes;
		}

		void SQLiteBundleStorage::get_blocks(dtn::data::Bundle &bundle, const dtn::data::BundleID &id)
		{
			int err = 0;
			string file;

			// select the right statement to use
			const size_t stmt_key = id.fragment ? BLOCK_GET_ID_FRAGMENT : BLOCK_GET_ID;

			AutoResetLock l(_locks[stmt_key], _statements[stmt_key]);

			// set the bundle key values
			set_bundleid(_statements[stmt_key], id);

			// query the database and step through all blocks
			while ((err = sqlite3_step(_statements[stmt_key])) == SQLITE_ROW)
			{
				const ibrcommon::File f( (const char*) sqlite3_column_text(_statements[stmt_key], 0) );
				int blocktyp = sqlite3_column_int(_statements[stmt_key], 1);

				// open the file
				std::ifstream is(f.getPath().c_str(), std::ios::binary | std::ios::in);

				if (blocktyp == dtn::data::PayloadBlock::BLOCK_TYPE)
				{
					// create a new BLOB object
					SQLiteBLOB *blob = new SQLiteBLOB(_blobPath);

					// remove the corresponding file
					blob->_file.remove();

					// generate a hardlink, pointing to the BLOB file
					if ( ::link(f.getPath().c_str(), blob->_file.getPath().c_str()) == 0)
					{
						// create a reference of the BLOB
						ibrcommon::BLOB::Reference ref(blob);

						// add payload block to the bundle
						bundle.push_back(ref);
					}
					else
					{
						delete blob;
						IBRCOMMON_LOGGER(error) << "unable to load bundle: failed to create a hard-link" << IBRCOMMON_LOGGER_ENDL;
					}
				}
				else
				{
					// read the block
					dtn::data::Block &block = dtn::data::SeparateDeserializer(is, bundle).readBlock();

					// close the file
					is.close();

					// modify the age block if present
					try {
						dtn::data::AgeBlock &agebl = dynamic_cast<dtn::data::AgeBlock&>(block);

						// modify the AgeBlock with the age of the file
						time_t age = f.lastaccess() - f.lastmodify();

						agebl.addSeconds(age);
					} catch (const std::bad_cast&) { };
				}
			}

			if (err == SQLITE_DONE)
			{
				if (bundle.getBlocks().size() == 0)
				{
					IBRCOMMON_LOGGER(error) << "get_blocks: no blocks found for: " << id.toString() << IBRCOMMON_LOGGER_ENDL;
					throw SQLiteQueryException("no blocks found");
				}
			}
			else
			{
				IBRCOMMON_LOGGER(error) << "get_blocks() failure: "<< err << " " << sqlite3_errmsg(_database) << IBRCOMMON_LOGGER_ENDL;
				throw SQLiteQueryException("can not query for blocks");
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
					AutoResetLock l(storage._locks[VACUUM], storage._statements[VACUUM]);
					sqlite3_step(storage._statements[VACUUM]);
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

		void SQLiteBundleStorage::releaseCustody(const dtn::data::EID &custodian, const dtn::data::BundleID &id)
		{
			// custody is successful transferred to another node.
			// it is safe to delete this bundle now. (depending on the routing algorithm.)
			// update the custodian of this bundle with the new one
			update_custodian(id, custodian);
		}

		void SQLiteBundleStorage::new_expire_time(size_t ttl)
		{
			ibrcommon::MutexLock l(_next_expiration_lock);
			if (_next_expiration == 0 || ttl < _next_expiration)
			{
				_next_expiration = ttl;
			}
		}

		void SQLiteBundleStorage::reset_expire_time()
		{
			ibrcommon::MutexLock l(_next_expiration_lock);
			_next_expiration = 0;
		}

		size_t SQLiteBundleStorage::get_expire_time()
		{
			ibrcommon::MutexLock l(_next_expiration_lock);
			return _next_expiration;
		}

		void SQLiteBundleStorage::add_deletion(const dtn::data::BundleID &id)
		{
			ibrcommon::MutexLock l(_deletion_mutex);
			_deletion_list.insert(id);
		}

		void SQLiteBundleStorage::remove_deletion(const dtn::data::BundleID &id)
		{
			ibrcommon::MutexLock l(_deletion_mutex);
			_deletion_list.erase(id);
		}

		bool SQLiteBundleStorage::contains_deletion(const dtn::data::BundleID &id)
		{
			ibrcommon::MutexLock l(_deletion_mutex);
			return (_deletion_list.find(id) != _deletion_list.end());
		}

		void SQLiteBundleStorage::trylock() throw (ibrcommon::MutexException)
		{
			// not implemented
			throw ibrcommon::MutexException();
		}

		void SQLiteBundleStorage::enter() throw (ibrcommon::MutexException)
		{
			// lock all statements
			for (int i = 0; i < SQL_QUERIES_END; i++)
			{
				// lock the statement
				_locks[i].enter();
			}
		}

		void SQLiteBundleStorage::leave() throw (ibrcommon::MutexException)
		{
			// unlock all statements
			for (int i = 0; i < SQL_QUERIES_END; i++)
			{
				// unlock the statement
				_locks[i].leave();
			}
		}

		void SQLiteBundleStorage::set_bundleid(sqlite3_stmt *st, const dtn::data::BundleID &id, size_t offset) const
		{
			const std::string source_id = id.source.getString();
			sqlite3_bind_text(st, offset + 1, source_id.c_str(), source_id.length(), SQLITE_TRANSIENT);
			sqlite3_bind_int64(st, offset + 2, id.timestamp);
			sqlite3_bind_int64(st, offset + 3, id.sequencenumber);

			if (id.fragment)
			{
				sqlite3_bind_int64(st, offset + 4, id.offset);
			}
		}

		void SQLiteBundleStorage::get_bundleid(sqlite3_stmt *st, dtn::data::BundleID &id, size_t offset) const
		{
			id.source = dtn::data::EID((const char*)sqlite3_column_text(st, offset + 0));
			id.timestamp = sqlite3_column_int64(st, offset + 1);
			id.sequencenumber = sqlite3_column_int64(st, offset + 2);

			id.fragment = (sqlite3_column_text(st, offset + 3) != NULL);
			id.offset = sqlite3_column_int64(st, offset + 3);
		}
	}
}

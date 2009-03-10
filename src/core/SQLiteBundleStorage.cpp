/*
 * SQLiteBundleStorage.cpp
 *
 *  Created on: 13.10.2008
 *      Author: morgenro
 */

#include "core/SQLiteBundleStorage.h"

#ifdef HAVE_LIBSQLITE3

#include "data/BundleFactory.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include "utils/Utils.h"

using namespace dtn::data;
using namespace dtn::utils;

namespace dtn
{
	namespace core
	{
		SQLiteBundleStorage::SQLiteBundleStorage(string dbfile, bool flush)
		 : Service("SQLiteBundleStorage"), BundleStorage(), last_gettime(0)
		{
			int rc;
			rc = sqlite3_open(dbfile.c_str(), &m_db);

			if ( rc ) {
				fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(m_db));
				sqlite3_close(m_db);
				exit(1);
			}

			// prepare all needed statements
			m_stmt_size = prepareStatement("SELECT COUNT(*) FROM schedules;");
			m_stmt_getbytime = prepareStatement("SELECT bundles.rowid, schedules.eid, schedules.dtntime, bundles.data FROM schedules LEFT JOIN bundles ON (schedules.bundle = bundles.rowid) WHERE schedules.dtntime <= ? ORDER BY bundles.administrative DESC, schedules.dtntime, bundles.priority DESC LIMIT 0,1");
			m_stmt_getbyeid = prepareStatement("SELECT bundles.rowid, schedules.eid, schedules.dtntime, bundles.data FROM schedules LEFT JOIN bundles ON (schedules.bundle = bundles.rowid) WHERE schedules.eid LIKE ? ORDER BY bundles.administrative DESC, bundles.priority DESC, schedules.dtntime LIMIT 0,1");
			m_stmt_store_bundle = prepareStatement("INSERT INTO bundles (creation_ts, creation_sq, eid_source, eid_dest, lifetime, fragment_offset, fragment_length, priority, administrative, data) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
			m_stmt_store_schedule = prepareStatement("INSERT INTO schedules (dtntime, eid, bundle) VALUES (?, ?, ?);");

			m_stmt_store_fragment = prepareStatement("INSERT INTO fragments (creation_ts, creation_sq, eid_source, lifetime, fragment_offset, fragment_length, data) VALUES (?, ?, ?, ?, ?, ?, ?);");
			m_stmt_check_fragment = prepareStatement("SELECT SUM(fragment_length) FROM fragments WHERE creation_ts = ? AND creation_sq = ? AND eid_source = ?");
			m_stmt_get_fragment = prepareStatement("SELECT data FROM fragments WHERE creation_ts = ? AND creation_sq = ? AND eid_source = ?");
			m_stmt_delete_fragment = prepareStatement("DELETE FROM fragments WHERE creation_ts = ? AND creation_sq = ? AND eid_source = ?");

			m_stmt_outdate_bundles = prepareStatement("DELETE FROM bundles WHERE (creation_ts + lifetime) < ?");
			m_stmt_outdate_fragments = prepareStatement("DELETE FROM fragments WHERE (creation_ts + lifetime) < ?");
		}

		SQLiteBundleStorage::~SQLiteBundleStorage()
		{
			sqlite3_finalize(m_stmt_size);
			sqlite3_finalize(m_stmt_getbytime);
			sqlite3_finalize(m_stmt_getbyeid);
			sqlite3_finalize(m_stmt_store_bundle);
			sqlite3_finalize(m_stmt_store_schedule);

			sqlite3_finalize(m_stmt_store_fragment);
			sqlite3_finalize(m_stmt_check_fragment);
			sqlite3_finalize(m_stmt_get_fragment);
			sqlite3_finalize(m_stmt_delete_fragment);

			sqlite3_close(m_db);
		}

		sqlite3_stmt* SQLiteBundleStorage::prepareStatement(string query)
		{
			sqlite3_stmt *stmt;

			// prepare the statement
			int rc = sqlite3_prepare_v2(m_db, query.c_str(), query.length(), &stmt, 0);

			if ( rc != SQLITE_OK )
			{
				fprintf(stderr, "SQL error: %d\n", rc);
				throw new exceptions::SQLitePrepareException();
			}

			return stmt;
		}

		void SQLiteBundleStorage::sqlQuery(string query, int id1, int id2, int id3)
		{
			int rc;
			sqlite3_stmt *st = prepareStatement(query);

			if (id1 >= 0)	sqlite3_bind_int(st, 1, id1);
			if (id2 >= 0)	sqlite3_bind_int(st, 2, id2);
			if (id3 >= 0)	sqlite3_bind_int(st, 3, id3);

			// execute the statement
			rc = sqlite3_step(st);

			sqlite3_finalize(st);

			if( rc != SQLITE_DONE )
			{
				fprintf(stderr, "SQL error: %d\n", rc);
				return;
			}

			return;
		}

		void SQLiteBundleStorage::clear()
		{
			sqlQuery("DELETE FROM schedules;");
			sqlQuery("DELETE FROM bundles;");
			sqlQuery("DELETE FROM fragments;");
			sqlQuery("VACUUM;");
		}

		unsigned int SQLiteBundleStorage::getSize()
		{
			MutexLock l(m_dblock);

			int rc;
			sqlite3_stmt *st = m_stmt_size;
			int ret = -1;

			// execute the statement
			rc = sqlite3_step(st);

			if( rc != SQLITE_ROW ){
				fprintf(stderr, "SQL error: %d\n", rc);
				sqlite3_reset(st);
				return ret;
			}

			// fetch result
			ret = (unsigned int)sqlite3_column_int(st, 0);

			// reset statement
			sqlite3_reset(st);

			return ret;
		}

		bool SQLiteBundleStorage::isEmpty()
		{
			if (getSize() == 0)
				return true;
			else
				return false;
		}

		unsigned int SQLiteBundleStorage::getCount()
		{
			return getSize();
		}

		void SQLiteBundleStorage::tick()
		{
			sleep(5);
			MutexLock l(m_dblock);
			// check every 5 seconds for outdated bundles and fragments
			unsigned int dtntime = BundleFactory::getDTNTime();
			sqlite3_bind_int64(m_stmt_outdate_bundles, 1, sqlite3_uint64(dtntime));
			sqlite3_bind_int64(m_stmt_outdate_fragments, 1, sqlite3_uint64(dtntime));
			sqlite3_step(m_stmt_outdate_bundles);
			sqlite3_step(m_stmt_outdate_fragments);
			sqlite3_reset(m_stmt_outdate_bundles);
			sqlite3_reset(m_stmt_outdate_fragments);
		}

		BundleSchedule SQLiteBundleStorage::getSchedule(unsigned int dtntime)
		{
			MutexLock l(m_dblock);

			// if this query already done befor as negative, do a fast answer
			if (last_gettime == dtntime)
				throw exceptions::NoScheduleFoundException("No schedule for this time available (fast answer)");

			int rc;
			sqlite3_stmt *st = m_stmt_getbytime;

			// set searched destination
			rc = sqlite3_bind_int(st, 1, dtntime);

			// query database
			rc = sqlite3_step(st);

			if ( rc != SQLITE_ROW )
			{
				// remember that no schedule for this time exists
				last_gettime = dtntime;

				sqlite3_reset(st);

				// throw exception
				throw exceptions::NoScheduleFoundException("No schedule for this time available");
			}

			int bundleid = sqlite3_column_int(st, 0);
			const char *c_eid = (const char*) sqlite3_column_text(st, 1);
			string eid( c_eid );
			int scheduletime = sqlite3_column_int(st, 2);

			// process bundle data
			unsigned int size = sqlite3_column_bytes(st, 3);
			unsigned char *data = (unsigned char*)sqlite3_column_blob(st, 3);

			BundleFactory &fac = BundleFactory::getInstance();
			Bundle *b = fac.parse(data, size);

			sqlite3_reset(st);

			// delete the schedule and the bundle in the database
			sqlQuery("DELETE FROM bundles WHERE rowid = ?", bundleid);

			BundleSchedule s(*b, scheduletime, eid);
			delete b;
			return s;
		}

		BundleSchedule SQLiteBundleStorage::getSchedule(string destination)
		{
			MutexLock l(m_dblock);

			int rc;
			sqlite3_stmt *st = m_stmt_getbyeid;

			stringstream ss;
			ss << destination << "%";
			ss >> destination;

			// set searched destination
			rc = sqlite3_bind_text(st, 1, destination.c_str(), destination.length(), SQLITE_TRANSIENT);

			// query database
			rc = sqlite3_step(st);

			if ( rc != SQLITE_ROW )
			{
				sqlite3_reset(st);

				// throw exception
				throw exceptions::NoScheduleFoundException("No schedule for this time available");
			}

			int bundleid = sqlite3_column_int(st, 0);
			const char *c_eid = (const char*) sqlite3_column_text(st, 1);
			string eid( c_eid );
			int scheduletime = sqlite3_column_int(st, 2);

			// process bundle data
			unsigned int size = sqlite3_column_bytes(st, 3);
			unsigned char *data = (unsigned char*)sqlite3_column_blob(st, 3);

			BundleFactory &fac = BundleFactory::getInstance();
			Bundle *b = fac.parse(data, size);

			sqlite3_reset(st);

			// delete the schedule and the bundle in the database
			sqlQuery("DELETE FROM bundles WHERE rowid = ?", bundleid);

			BundleSchedule s(*b, scheduletime, eid);
			delete b;
			return s;
		}

		void SQLiteBundleStorage::store(const BundleSchedule &schedule)
		{
			MutexLock l(m_dblock);

			// set last get time to zero. this allows real querys to database
			last_gettime = 0;

			int rc;
			sqlite3_stmt *st = m_stmt_store_schedule;

			const Bundle &b = schedule.getBundle();

			if ( b == NULL ) return;

			try {
				// get id for the stored bundle
				sqlite3_int64 bundle_id = storeBundle(b);

				sqlite3_bind_int64(st, 1, sqlite3_uint64(schedule.getTime()));
				string target_eid = schedule.getEID();
				sqlite3_bind_text(st, 2, target_eid.c_str(), target_eid.length(), SQLITE_TRANSIENT);
				sqlite3_bind_int64(st, 3, bundle_id);

				// insert schedule into database
				rc = sqlite3_step(st);

				// if the database is busy retry in a few milliseconds
				while ( rc == SQLITE_BUSY )
				{
					usleep(100);
					rc = sqlite3_step(st);
				}

				sqlite3_reset(st);

				if (( rc != SQLITE_DONE ) && ( rc != SQLITE_CONSTRAINT ))
				{
					cerr << "database error while storing schedule: " << rc << endl;
					throw exceptions::NoSpaceLeftException("database error while storing schedule");
				}
			} catch (DuplicateException ex) {

			}
		}

		Bundle* SQLiteBundleStorage::storeFragment(const Bundle &bundle)
		{
			MutexLock l(m_dblock);

			int rc = 0;
			sqlite3_stmt *st = m_stmt_check_fragment;
			unsigned int ret = 0;

			// search for other fragments in the database
			sqlite3_bind_int64(st, 1, sqlite3_uint64(bundle.getInteger(CREATION_TIMESTAMP)));
			sqlite3_bind_int64(st, 2, sqlite3_uint64(bundle.getInteger(CREATION_TIMESTAMP_SEQUENCE)));

			string source = bundle.getSource();
			sqlite3_bind_text(st, 3, source.c_str(), source.length(), SQLITE_TRANSIENT);

			// execute the statement
			rc = sqlite3_step(st);

			if( rc != SQLITE_ROW ){
				fprintf(stderr, "SQL error: %d\n", rc);
				sqlite3_reset(st);
				return NULL;
			}

			// fetch result
			ret = (unsigned int)sqlite3_column_int64(st, 0);

			// reset statement
			sqlite3_reset(st);

			PayloadBlock *payload = bundle.getPayloadBlock();

			if (payload == NULL)
			{
				// huh!? no payload block?
				return NULL;
			}

			// add the size of the payload block
			ret += payload->getLength();

			// if "ret" is equal to the application data size
			if ( ret == bundle.getInteger(APPLICATION_DATA_LENGTH) )
			{
				// ... then all fragments available, merge and return
				st = m_stmt_get_fragment;

				// search for other fragments in the database
				sqlite3_bind_int64(st, 1, sqlite3_uint64(bundle.getInteger(CREATION_TIMESTAMP)));
				sqlite3_bind_int64(st, 2, sqlite3_uint64(bundle.getInteger(CREATION_TIMESTAMP_SEQUENCE)));

				string source = bundle.getSource();
				sqlite3_bind_text(st, 3, source.c_str(), source.length(), SQLITE_TRANSIENT);

				Bundle *b = NULL;
				list<Bundle> fragment_list;
				fragment_list.push_back( bundle );

				// execute the statement
				while ( sqlite3_step(st) == SQLITE_ROW ){
					unsigned char *data = (unsigned char*)sqlite3_column_blob(st, 0);
					unsigned int size = sqlite3_column_bytes(st, 0);

					BundleFactory &fac = BundleFactory::getInstance();
					b = fac.parse(data, size);
					fragment_list.push_back( *b );
				}

				// reset statement
				sqlite3_reset(st);

				// delete all fragments in database
				sqlite3_bind_int64(m_stmt_delete_fragment, 1, sqlite3_uint64(bundle.getInteger(CREATION_TIMESTAMP)));
				sqlite3_bind_int64(m_stmt_delete_fragment, 2, sqlite3_uint64(bundle.getInteger(CREATION_TIMESTAMP_SEQUENCE)));
				sqlite3_bind_text(m_stmt_delete_fragment, 3, source.c_str(), source.length(), SQLITE_TRANSIENT);
				sqlite3_step(m_stmt_delete_fragment);

				// reset statement
				sqlite3_reset(m_stmt_delete_fragment);

				// merge all bundle fragments to one bundle
				Bundle *merged = BundleFactory::getInstance().merge(fragment_list);

				return merged;
			}

			// ... else insert the fragment into the database
			st = m_stmt_store_fragment;

			sqlite3_bind_int(st, 1, bundle.getInteger(CREATION_TIMESTAMP));
			sqlite3_bind_int(st, 2, bundle.getInteger(CREATION_TIMESTAMP_SEQUENCE));
			sqlite3_bind_text(st, 3, source.c_str(), source.length(), SQLITE_TRANSIENT);
			sqlite3_bind_int(st, 4, bundle.getInteger(LIFETIME));
			sqlite3_bind_int(st, 5, bundle.getInteger(FRAGMENTATION_OFFSET));
			sqlite3_bind_int(st, 6, payload->getLength());

			unsigned char *data = bundle.getData();
			sqlite3_bind_blob(st, 7, data, bundle.getLength(), SQLITE_TRANSIENT);
			free(data);

			// insert bundle into database
			rc = sqlite3_step(st);

			sqlite3_reset(st);

			if (( rc != SQLITE_DONE ) && ( rc != SQLITE_CONSTRAINT ))
			{
				cerr << "database error while storing fragment: " << rc << endl;
				throw exceptions::NoSpaceLeftException("database error while storing fragment");
			}

			return NULL;
		}

		sqlite3_int64 SQLiteBundleStorage::storeBundle(const Bundle &b)
		{
			int rc;
			sqlite3_stmt *st = m_stmt_store_bundle;

			if ( b == NULL ) return 0;

			sqlite3_bind_int(st, 1, b.getInteger(CREATION_TIMESTAMP));
			sqlite3_bind_int(st, 2, b.getInteger(CREATION_TIMESTAMP_SEQUENCE));

			string source = b.getSource();
			sqlite3_bind_text(st, 3, source.c_str(), source.length(), SQLITE_TRANSIENT);

			string destination = b.getDestination();
			sqlite3_bind_text(st, 4, destination.c_str(), destination.length(), SQLITE_TRANSIENT);

			sqlite3_bind_int(st, 5, b.getInteger(LIFETIME));

			if ( b.getPrimaryFlags().isFragment() )
			{
				sqlite3_bind_int(st, 6, b.getInteger(FRAGMENTATION_OFFSET));
				sqlite3_bind_int(st, 7, b.getInteger(APPLICATION_DATA_LENGTH));
			}
			else
			{
				sqlite3_bind_null(st, 6);
				sqlite3_bind_null(st, 7);
			}

			PrimaryFlags flags = b.getPrimaryFlags();
			sqlite3_bind_int(st, 8, flags.getPriority());
			if ( flags.isAdmRecord() ) 	sqlite3_bind_int(st, 9, 1);
			else						sqlite3_bind_int(st, 9, 0);

			unsigned char *data = b.getData();
			sqlite3_bind_blob(st, 10, data, b.getLength(), SQLITE_TRANSIENT);
			free(data);

			// insert bundle into database
			rc = sqlite3_step(st);

			// get id for the stored bundle
			sqlite3_int64 bundle_id = sqlite3_last_insert_rowid(m_db);

			sqlite3_reset(st);

			if ( rc == SQLITE_CONSTRAINT )
			{
				throw exceptions::DuplicateException();
			}

			if ( rc != SQLITE_DONE )
			{
				cerr << "database error while storing bundle: " << rc << endl;
				throw exceptions::NoSpaceLeftException("database error while storing bundle");
			}

			return bundle_id;
		}
	}
}

#endif

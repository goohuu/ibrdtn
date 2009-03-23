/*
 * SQLiteBundleStorage.h
 *
 *  Created on: 13.10.2008
 *      Author: morgenro
 */

#ifndef SQLITEBUNDLESTORAGE_H_
#define SQLITEBUNDLESTORAGE_H_

#include "config.h"

#ifdef HAVE_LIBSQLITE3

#include "core/BundleStorage.h"
#include "core/CustodyManager.h"
#include "data/Bundle.h"
#include "utils/Service.h"
#include <list>
#include <vector>
#include <sqlite3.h>
#include "data/Exceptions.h"
#include "utils/MutexLock.h"
#include "utils/Mutex.h"

using namespace dtn::data;
using namespace dtn::utils;

namespace dtn
{
	namespace exceptions
	{
		class SQLitePrepareException : public Exception
		{
			public:
				SQLitePrepareException() : Exception("error while prepare the sql statement")
				{
				};

				SQLitePrepareException(string what) : Exception(what)
				{
				};
		};

		class DuplicateException : public Exception
		{
			public:
				DuplicateException() : Exception("this item already exists!")
				{
				};

				DuplicateException(string what) : Exception(what)
				{
				};
		};
	}

	namespace core
	{
		/**
		 * This storage holds all bundles, fragments and schedules in a SQLite database.
		 */
		class SQLiteBundleStorage : public Service, public BundleStorage
		{
		public:
			/**
			 * construktor
			 * @param dbfile database filename
			 * @param flush clear the database on initialization
			 */
			SQLiteBundleStorage(string dbfile, bool flush = false);

			/**
			 * destructor
			 */
			virtual ~SQLiteBundleStorage();

			/**
			 * @sa BundleStorage::store(BundleSchedule schedule)
			 */
			void store(const BundleSchedule &schedule);

			/**
			 * @sa BundleStorage::clear()
			 */
			void clear();

			/**
			 * @sa BundleStorage::isEmpty()
			 */
			bool isEmpty();

			/**
			 * @sa Service::tick()
			 */
			virtual void tick();

			unsigned int getSize();

			/**
			 * @sa BundleStorage::getCount()
			 */
			unsigned int getCount();

			/**
			 * @sa getSchedule(unsigned int dtntime)
			 */
			BundleSchedule getSchedule(unsigned int dtntime);

			/**
			 * @sa getSchedule(string destination)
			 */
			BundleSchedule getSchedule(string destination);

			/**
			 * @sa storeFragment();
			 */
			Bundle* storeFragment(const Bundle &bundle);

		private:
			void sqlQuery(string query, int id1 = -1, int id2 = -1, int id3 = -1);
			sqlite3_stmt* prepareStatement(string query);
			sqlite3_int64 storeBundle(const Bundle &bundle);

			sqlite3 *m_db;

			Mutex m_dblock;
			unsigned int last_gettime;

			sqlite3_stmt *m_stmt_size;
			sqlite3_stmt *m_stmt_getbytime;
			sqlite3_stmt *m_stmt_getbyeid;
			sqlite3_stmt *m_stmt_store_bundle;
			sqlite3_stmt *m_stmt_store_schedule;

			sqlite3_stmt *m_stmt_store_fragment;
			sqlite3_stmt *m_stmt_check_fragment;
			sqlite3_stmt *m_stmt_get_fragment;
			sqlite3_stmt *m_stmt_delete_fragment;

			sqlite3_stmt *m_stmt_outdate_fragments;
			sqlite3_stmt *m_stmt_outdate_bundles;

		};
	}
}

#endif

#endif /* SQLITEBUNDLESTORAGE_H_ */


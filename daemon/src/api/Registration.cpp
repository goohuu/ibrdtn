/*
 * Registration.cpp
 *
 *  Created on: 15.06.2011
 *      Author: morgenro
 */

#include "config.h"
#include "api/Registration.h"
#include "core/BundleStorage.h"
#include "core/BundleCore.h"

#ifdef HAVE_SQLITE
#include "core/SQLiteBundleStorage.h"
#endif

#include <ibrdtn/utils/Random.h>

#include <limits.h>
#include <stdint.h>

namespace dtn
{
	namespace api
	{
		std::set<std::string> Registration::_handles;

		const std::string Registration::alloc_handle()
		{
			static dtn::utils::Random rand;

			std::string new_handle = rand.gen_chars(16);

			while (_handles.find(new_handle) != _handles.end())
			{
				new_handle = rand.gen_chars(16);
			}

			Registration::_handles.insert(new_handle);

			return new_handle;
		}

		void Registration::free_handle(const std::string &handle)
		{
			Registration::_handles.erase(handle);
		}

		Registration::Registration() : _handle(alloc_handle())
		{
		}

		Registration::~Registration()
		{
			free_handle(_handle);
		}

		void Registration::queue(const dtn::data::BundleID &id)
		{
			// add bundle to the queue
			_queue.push(id);

			// notify the client about the new bundle
			notify(NOTIFY_BUNDLE_QUEUED);
		}

		bool Registration::hasSubscribed(const dtn::data::EID &endpoint) const
		{
			return (_endpoints.find(endpoint) != _endpoints.end());
		}

		const std::set<dtn::data::EID>& Registration::getSubscriptions() const
		{
			return _endpoints;
		}

		void Registration::subscribe(const dtn::data::EID &endpoint)
		{
			_endpoints.insert(endpoint);

			/**
			 * search for bundles in the storage
			 */
#ifdef HAVE_SQLITE
			class BundleFilter : public dtn::core::BundleStorage::BundleFilterCallback, public dtn::core::SQLiteBundleStorage::SQLBundleQuery
#else
			class BundleFilter : public dtn::core::BundleStorage::BundleFilterCallback
#endif
			{
			public:
				BundleFilter(const dtn::data::EID &endpoint)
				 : _endpoint(endpoint)
				{};

				virtual ~BundleFilter() {};

				virtual size_t limit() const { return 0; };

				virtual bool shouldAdd(const dtn::data::MetaBundle &meta) const
				{
					if (_endpoint == meta.destination)
					{
						return true;
					}

					return false;
				};

#ifdef HAVE_SQLITE
				const std::string getWhere() const
				{
					return "destination = ?";
				};

				size_t bind(sqlite3_stmt *st, size_t offset) const
				{
					sqlite3_bind_text(st, offset, _endpoint.getString().c_str(), _endpoint.getString().size(), SQLITE_TRANSIENT);
					return offset + 1;
				}
#endif

			private:
				const dtn::data::EID &_endpoint;
			} filter(endpoint);

			// get the global storage
			dtn::core::BundleStorage &storage = dtn::core::BundleCore::getInstance().getStorage();

			const std::list<dtn::data::MetaBundle> list = storage.get( filter );

			for (std::list<dtn::data::MetaBundle>::const_iterator iter = list.begin(); iter != list.end(); iter++)
			{
				queue(*iter);
			}
		}

		void Registration::unsubscribe(const dtn::data::EID &endpoint)
		{
			_endpoints.erase(endpoint);
		}

		/**
		 * compares the local handle with the given one
		 */
		bool Registration::operator==(const std::string &other) const
		{
			return (_handle == other);
		}

		/**
		 * compares another registration with this one
		 */
		bool Registration::operator==(const Registration &other) const
		{
			return (_handle == other._handle);
		}

		/**
		 * compares and order a registration (using the handle)
		 */
		bool Registration::operator<(const Registration &other) const
		{
			return (_handle < other._handle);
		}

		ibrcommon::Queue<dtn::data::BundleID>& Registration::getQueue()
		{
			return _queue;
		}

		const std::string& Registration::getHandle() const
		{
			return _handle;
		}
	}
}

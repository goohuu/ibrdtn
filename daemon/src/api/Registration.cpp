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
#include "core/BundleEvent.h"

#ifdef HAVE_SQLITE
#include "core/SQLiteBundleStorage.h"
#endif

#include <ibrdtn/utils/Clock.h>
#include <ibrdtn/utils/Random.h>
#include <ibrcommon/Logger.h>

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

		void Registration::notify(const NOTIFY_CALL call)
		{
			ibrcommon::MutexLock l(_wait_for_cond);
			if (call == NOTIFY_BUNDLE_AVAILABLE)
			{
				_no_more_bundles = false;
				_wait_for_cond.signal(true);
			}
			else
			{
				_notify_queue.push(call);
			}
		}

		void Registration::wait_for_bundle()
		{
			ibrcommon::MutexLock l(_wait_for_cond);

			while (_no_more_bundles)
			{
				_wait_for_cond.wait();
			}
		}

		Registration::NOTIFY_CALL Registration::wait()
		{
			return _notify_queue.getnpop(true);
		}

		bool Registration::hasSubscribed(const dtn::data::EID &endpoint) const
		{
			return (_endpoints.find(endpoint) != _endpoints.end());
		}

		const std::set<dtn::data::EID>& Registration::getSubscriptions() const
		{
			return _endpoints;
		}

		void Registration::delivered(const dtn::data::MetaBundle &m)
		{
			// raise bundle event
			dtn::core::BundleEvent::raise(m, dtn::core::BUNDLE_DELIVERED);

			if (m.get(dtn::data::PrimaryBlock::DESTINATION_IS_SINGLETON))
			{
				// get the global storage
				dtn::core::BundleStorage &storage = dtn::core::BundleCore::getInstance().getStorage();

				// delete the bundle
				storage.remove(m);
			}
		}

		dtn::data::Bundle Registration::receive() throw (dtn::core::BundleStorage::NoBundleFoundException)
		{
			ibrcommon::MutexLock l(_receive_lock);

			// get the global storage
			dtn::core::BundleStorage &storage = dtn::core::BundleCore::getInstance().getStorage();

			while (true)
			{
				try {
					// get the first bundle in the queue
					dtn::data::MetaBundle b = _queue.getnpop(false);

					// load the bundle
					return storage.get(b);
				} catch (const ibrcommon::QueueUnblockedException &e) {
					if (e.reason == ibrcommon::QueueUnblockedException::QUEUE_ABORT)
					{
						// query for new bundles
						underflow();
					}
				} catch (const dtn::core::BundleStorage::NoBundleFoundException&) { }
			}

			throw dtn::core::BundleStorage::NoBundleFoundException();
		}

		void Registration::underflow()
		{
			// expire outdated bundles in the list
			_received_bundles.expire(dtn::utils::Clock::getTime());

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
				BundleFilter(const std::set<dtn::data::EID> endpoints, const dtn::data::BundleList &blist)
				 : _endpoints(endpoints), _blist(blist)
				{};

				virtual ~BundleFilter() {};

				virtual size_t limit() const { return 10; };

				virtual bool shouldAdd(const dtn::data::MetaBundle &meta) const
				{
					if (_endpoints.find(meta.destination) == _endpoints.end())
					{
						return false;
					}

					IBRCOMMON_LOGGER_DEBUG(10) << "search bundle in the list of delivered bundles: " << meta.toString() << IBRCOMMON_LOGGER_ENDL;

					if (_blist.contains(meta))
					{
						return false;
					}

					return true;
				};

#ifdef HAVE_SQLITE
				const std::string getWhere() const
				{
					if (_endpoints.size() > 1)
					{
						std::string where = "(";

						for (size_t i = _endpoints.size() - 1; i > 0; i--)
						{
							where += "destination = ? OR ";
						}

						return where + "destination = ?)";
					}
					else if (_endpoints.size() == 1)
					{
						return "destination = ?";
					}
					else
					{
						return "destination = null";
					}
				};

				size_t bind(sqlite3_stmt *st, size_t offset) const
				{
					size_t o = offset;

					for (std::set<dtn::data::EID>::const_iterator iter = _endpoints.begin(); iter != _endpoints.end(); iter++)
					{
						const std::string data = (*iter).getString();

						sqlite3_bind_text(st, o, data.c_str(), data.size(), SQLITE_TRANSIENT);
						o++;
					}

					return o;
				}
#endif

			private:
				const std::set<dtn::data::EID> _endpoints;
				const dtn::data::BundleList &_blist;
			} filter(_endpoints, _received_bundles);

			// get the global storage
			dtn::core::BundleStorage &storage = dtn::core::BundleCore::getInstance().getStorage();

			// query the database for more bundles
			const std::list<dtn::data::MetaBundle> list = storage.get( filter );

			if (list.size() == 0)
			{
				ibrcommon::MutexLock l(_wait_for_cond);
				_no_more_bundles = true;
				throw dtn::core::BundleStorage::NoBundleFoundException();
			}

			try {
				for (std::list<dtn::data::MetaBundle>::const_iterator iter = list.begin(); iter != list.end(); iter++)
				{
					_queue.push(*iter);

					IBRCOMMON_LOGGER_DEBUG(10) << "add bundle to list of delivered bundles: " << (*iter).toString() << IBRCOMMON_LOGGER_ENDL;
					_received_bundles.add(*iter);
				}
			} catch (const ibrcommon::Exception&) { }
		}

		void Registration::subscribe(const dtn::data::EID &endpoint)
		{
			_endpoints.insert(endpoint);
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

		void Registration::abort()
		{
			_queue.abort();
			_notify_queue.abort();

			ibrcommon::MutexLock l(_wait_for_cond);
			_wait_for_cond.abort();
		}

		const std::string& Registration::getHandle() const
		{
			return _handle;
		}
	}
}

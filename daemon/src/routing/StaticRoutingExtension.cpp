/*
 * StaticRoutingExtension.cpp
 *
 *  Created on: 16.02.2010
 *      Author: morgenro
 */

#include "routing/StaticRoutingExtension.h"
#include "routing/QueueBundleEvent.h"
#include "net/TransferAbortedEvent.h"
#include "net/TransferCompletedEvent.h"
#include "routing/RequeueBundleEvent.h"
#include "core/NodeEvent.h"
#include "core/SimpleBundleStorage.h"

#include <ibrcommon/Logger.h>
#include <ibrcommon/AutoDelete.h>

#include <typeinfo>

namespace dtn
{
	namespace routing
	{
		StaticRoutingExtension::StaticRoutingExtension(const std::list<StaticRoutingExtension::StaticRoute> &routes)
		 : _routes(routes)
		{
		}

		StaticRoutingExtension::~StaticRoutingExtension()
		{
			stop();
			join();
		}

		void StaticRoutingExtension::stopExtension()
		{
			_taskqueue.abort();
		}

		bool StaticRoutingExtension::__cancellation()
		{
			_taskqueue.abort();
			return true;
		}

		void StaticRoutingExtension::run()
		{
			class BundleFilter : public dtn::core::BundleStorage::BundleFilterCallback
			{
			public:
				BundleFilter(const NeighborDatabase::NeighborEntry &entry, const std::list<StaticRoutingExtension::StaticRoute> &routes)
				 : _entry(entry), _routes(routes)
				{};

				virtual ~BundleFilter() {};

				virtual size_t limit() const { return 10; };

				virtual bool shouldAdd(const dtn::data::MetaBundle &meta) const
				{
					// do not forward to any blacklisted destination
					const dtn::data::EID dest = meta.destination.getNodeEID();
					if (_blacklist.find(dest) != _blacklist.end())
					{
						return false;
					}

					// do not forward bundles already known by the destination
					if (_entry.has(meta))
					{
						return false;
					}

					// search for one rule that match
					for (std::list<StaticRoutingExtension::StaticRoute>::const_iterator iter = _routes.begin();
							iter != _routes.end(); iter++)
					{
						const StaticRoutingExtension::StaticRoute &route = (*iter);

						if (route.match(meta.destination))
						{
							return true;
						}
					}

					return false;
				};

				void blacklist(const dtn::data::EID& id)
				{
					_blacklist.insert(id);
				};

			private:
				std::set<dtn::data::EID> _blacklist;
				const NeighborDatabase::NeighborEntry &_entry;
				const std::list<StaticRoutingExtension::StaticRoute> _routes;
			};

			dtn::core::BundleStorage &storage = (**this).getStorage();

			while (true)
			{
				NeighborDatabase &db = (**this).getNeighborDB();

				try {
					Task *t = _taskqueue.getnpop(true);
					ibrcommon::AutoDelete<Task> killer(t);

					IBRCOMMON_LOGGER_DEBUG(5) << "processing static routing task " << t->toString() << IBRCOMMON_LOGGER_ENDL;

					try {
						SearchNextBundleTask &task = dynamic_cast<SearchNextBundleTask&>(*t);

						std::list<StaticRoutingExtension::StaticRoute> routes;

						// look for routes to this node
						for (std::list<StaticRoutingExtension::StaticRoute>::const_iterator iter = _routes.begin();
								iter != _routes.end(); iter++)
						{
							const StaticRoutingExtension::StaticRoute &route = (*iter);
							if (route.getDestination() == task.eid)
							{
								// add to the valid routes
								routes.push_back(route);
							}
						}

						if (routes.size() > 0)
						{
							// this destination is not handles by any static route
							ibrcommon::MutexLock l(db);
							NeighborDatabase::NeighborEntry &entry = db.get(task.eid);

							// get the bundle filter of the neighbor
							BundleFilter filter(entry, routes);

							// some debug
							IBRCOMMON_LOGGER_DEBUG(40) << "search some bundles not known by " << task.eid.getString() << IBRCOMMON_LOGGER_ENDL;

							// blacklist the neighbor itself, because this is handled by neighbor routing extension
							filter.blacklist(task.eid);

							// query all bundles from the storage
							const std::list<dtn::data::MetaBundle> list = storage.get(filter);

							// send the bundles as long as we have resources
							for (std::list<dtn::data::MetaBundle>::const_iterator iter = list.begin(); iter != list.end(); iter++)
							{
								try {
									// transfer the bundle to the neighbor
									transferTo(entry, *iter);
								} catch (const NeighborDatabase::AlreadyInTransitException&) { };
							}
						}
					} catch (const NeighborDatabase::NoMoreTransfersAvailable&) {
					} catch (const NeighborDatabase::NeighborNotAvailableException&) {
					} catch (const std::bad_cast&) { };

					try {
						const ProcessBundleTask &task = dynamic_cast<ProcessBundleTask&>(*t);

						// look for routes to this node
						for (std::list<StaticRoutingExtension::StaticRoute>::const_iterator iter = _routes.begin();
								iter != _routes.end(); iter++)
						{
							const StaticRoutingExtension::StaticRoute &route = (*iter);
							try {
								if (route.match(task.bundle.destination))
								{
									ibrcommon::MutexLock l(db);
									NeighborDatabase::NeighborEntry &n = db.get(route.getDestination());

									// transfer the bundle to the neighbor
									transferTo(n, task.bundle);
								}
							} catch (const NeighborDatabase::NeighborNotAvailableException&) {
								// neighbor is not available, can not forward this bundle
							} catch (const NeighborDatabase::NoMoreTransfersAvailable&) {
							} catch (const NeighborDatabase::AlreadyInTransitException&) { };
						}
					} catch (const std::bad_cast&) { };

				} catch (const std::exception &ex) {
					IBRCOMMON_LOGGER_DEBUG(20) << "static routing failed: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
					return;
				}

				yield();
			}
		}

		void StaticRoutingExtension::notify(const dtn::core::Event *evt)
		{
			try {
				const QueueBundleEvent &queued = dynamic_cast<const QueueBundleEvent&>(*evt);
				_taskqueue.push( new ProcessBundleTask(queued.bundle, queued.origin) );
				return;
			} catch (const std::bad_cast&) { };

			try {
				const dtn::core::NodeEvent &nodeevent = dynamic_cast<const dtn::core::NodeEvent&>(*evt);
				const dtn::core::Node &n = nodeevent.getNode();

				if (nodeevent.getAction() == NODE_AVAILABLE)
				{
					_taskqueue.push( new SearchNextBundleTask(n.getEID()) );
				}
				return;
			} catch (const std::bad_cast&) { };

			// The bundle transfer has been aborted
			try {
				const dtn::net::TransferAbortedEvent &aborted = dynamic_cast<const dtn::net::TransferAbortedEvent&>(*evt);
				_taskqueue.push( new SearchNextBundleTask(aborted.getPeer()) );
				return;
			} catch (const std::bad_cast&) { };

			// A bundle transfer was successful
			try {
				const dtn::net::TransferCompletedEvent &completed = dynamic_cast<const dtn::net::TransferCompletedEvent&>(*evt);
				_taskqueue.push( new SearchNextBundleTask(completed.getPeer()) );
				return;
			} catch (const std::bad_cast&) { };
		}

		StaticRoutingExtension::StaticRoute::StaticRoute(const std::string &regex, const std::string &dest)
			: _dest(dest), _regex_str(regex), _invalid(false)
		{
			if ( regcomp(&_regex, regex.c_str(), 0) )
			{
				IBRCOMMON_LOGGER(error) << "Could not compile regex: " << regex << IBRCOMMON_LOGGER_ENDL;
				_invalid = true;
			}
		}

		StaticRoutingExtension::StaticRoute::~StaticRoute()
		{
			if (!_invalid)
				regfree(&_regex);
		}

		StaticRoutingExtension::StaticRoute::StaticRoute(const StaticRoutingExtension::StaticRoute &obj)
			: _dest(obj._dest), _regex_str(obj._regex_str), _invalid(obj._invalid)
		{
			if ( regcomp(&_regex, _regex_str.c_str(), 0) )
			{
				_invalid = true;
			}
		}

		StaticRoutingExtension::StaticRoute& StaticRoutingExtension::StaticRoute::operator=(const StaticRoutingExtension::StaticRoute &obj)
		{
			if (!_invalid)
			{
				regfree(&_regex);
			}

			_dest = obj._dest;
			_regex_str = obj._regex_str;
			_invalid = obj._invalid;

			if (!_invalid)
			{
				if ( regcomp(&_regex, obj._regex_str.c_str(), 0) )
				{
					IBRCOMMON_LOGGER(error) << "Could not compile regex: " << _regex_str << IBRCOMMON_LOGGER_ENDL;
					_invalid = true;
				}
			}

			return *this;
		}

		bool StaticRoutingExtension::StaticRoute::match(const dtn::data::EID &eid) const
		{
			if (_invalid) return false;

			const std::string dest = eid.getString();

			// test against the regular expression
			int reti = regexec(&_regex, dest.c_str(), 0, NULL, 0);

			if( !reti )
			{
				// the expression match
				return true;
			}
			else if( reti == REG_NOMATCH )
			{
				// the expression not match
				return false;
			}
			else
			{
				char msgbuf[100];
				regerror(reti, &_regex, msgbuf, sizeof(msgbuf));
				IBRCOMMON_LOGGER(error) << "Regex match failed: " << std::string(msgbuf) << IBRCOMMON_LOGGER_ENDL;
				return false;
			}
		}

		const dtn::data::EID& StaticRoutingExtension::StaticRoute::getDestination() const
		{
			return _dest;
		}

		/****************************************/

		StaticRoutingExtension::SearchNextBundleTask::SearchNextBundleTask(const dtn::data::EID &e)
		 : eid(e)
		{ }

		StaticRoutingExtension::SearchNextBundleTask::~SearchNextBundleTask()
		{ }

		std::string StaticRoutingExtension::SearchNextBundleTask::toString()
		{
			return "SearchNextBundleTask: " + eid.getString();
		}

		/****************************************/

		StaticRoutingExtension::ProcessBundleTask::ProcessBundleTask(const dtn::data::MetaBundle &meta, const dtn::data::EID &o)
		 : bundle(meta), origin(o)
		{ }

		StaticRoutingExtension::ProcessBundleTask::~ProcessBundleTask()
		{ }

		std::string StaticRoutingExtension::ProcessBundleTask::toString()
		{
			return "ProcessBundleTask: " + bundle.toString();
		}
	}
}

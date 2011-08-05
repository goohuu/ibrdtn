/*
 * FloodRoutingExtension.cpp
 *
 *  Created on: 18.02.2010
 *      Author: morgenro
 */

#include "routing/flooding/FloodRoutingExtension.h"
#include "routing/QueueBundleEvent.h"
#include "core/NodeEvent.h"
#include "net/TransferCompletedEvent.h"
#include "net/TransferAbortedEvent.h"
#include "core/Node.h"
#include "net/ConnectionManager.h"
#include "Configuration.h"
#include "core/BundleCore.h"

#include <ibrdtn/data/MetaBundle.h>
#include <ibrcommon/thread/MutexLock.h>
#include <ibrcommon/Logger.h>
#include <ibrcommon/AutoDelete.h>

#include <functional>
#include <list>
#include <algorithm>
#include <iomanip>
#include <ios>
#include <iostream>

#include <stdlib.h>
#include <typeinfo>

namespace dtn
{
	namespace routing
	{
		FloodRoutingExtension::FloodRoutingExtension()
		{
			// write something to the syslog
			IBRCOMMON_LOGGER(info) << "Initializing flooding routing module" << IBRCOMMON_LOGGER_ENDL;
		}

		FloodRoutingExtension::~FloodRoutingExtension()
		{
			stop();
			join();
		}

		void FloodRoutingExtension::stopExtension()
		{
			_taskqueue.abort();
		}

		void FloodRoutingExtension::notify(const dtn::core::Event *evt)
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
					const dtn::data::EID &eid = n.getEID();

					// send all (multi-hop) bundles in the storage to the neighbor
					_taskqueue.push( new SearchNextBundleTask(eid) );
				}
				return;
			} catch (const std::bad_cast&) { };

			// The bundle transfer has been aborted
			try {
				const dtn::net::TransferAbortedEvent &aborted = dynamic_cast<const dtn::net::TransferAbortedEvent&>(*evt);

				// transfer the next bundle to this destination
				_taskqueue.push( new SearchNextBundleTask( aborted.getPeer() ) );
				return;
			} catch (const std::bad_cast&) { };

			// A bundle transfer was successful
			try {
				const dtn::net::TransferCompletedEvent &completed = dynamic_cast<const dtn::net::TransferCompletedEvent&>(*evt);

				// transfer the next bundle to this destination
				_taskqueue.push( new SearchNextBundleTask( completed.getPeer() ) );
				return;
			} catch (const std::bad_cast&) { };
		}

		bool FloodRoutingExtension::__cancellation()
		{
			_taskqueue.abort();
			return true;
		}

		void FloodRoutingExtension::run()
		{
			class BundleFilter : public dtn::core::BundleStorage::BundleFilterCallback
			{
			public:
				BundleFilter(const NeighborDatabase::NeighborEntry &entry)
				 : _entry(entry)
				{};

				virtual ~BundleFilter() {};

				virtual size_t limit() const { return 10; };

				virtual bool shouldAdd(const dtn::data::MetaBundle &meta) const
				{
					// check Scope Control Block - do not forward bundles with hop limit == 0
					if (meta.hopcount == 0)
					{
						return false;
					}

					// check Scope Control Block - do not forward non-group bundles with hop limit <= 1
					if ((meta.hopcount <= 1) && (meta.get(dtn::data::PrimaryBlock::DESTINATION_IS_SINGLETON)))
					{
						return false;
					}

					// do not forward to any blacklisted destination
					const dtn::data::EID dest = meta.destination.getNode();
					if (_blacklist.find(dest) != _blacklist.end())
					{
						return false;
					}

					// do not forward bundles already known by the destination
					if (_entry.has(meta))
					{
						return false;
					}

					return true;
				};

				void blacklist(const dtn::data::EID& id)
				{
					_blacklist.insert(id);
				};

			private:
				std::set<dtn::data::EID> _blacklist;
				const NeighborDatabase::NeighborEntry &_entry;
			};

			dtn::core::BundleStorage &storage = (**this).getStorage();

			while (true)
			{
				try {
					Task *t = _taskqueue.getnpop(true);
					ibrcommon::AutoDelete<Task> killer(t);

					IBRCOMMON_LOGGER_DEBUG(50) << "processing flooding task " << t->toString() << IBRCOMMON_LOGGER_ENDL;

					try {
						try {
							SearchNextBundleTask &task = dynamic_cast<SearchNextBundleTask&>(*t);
							NeighborDatabase &db = (**this).getNeighborDB();

							ibrcommon::MutexLock l(db);
							NeighborDatabase::NeighborEntry &entry = db.get(task.eid);

							// get the bundle filter of the neighbor
							BundleFilter filter(entry);

							// some debug
							IBRCOMMON_LOGGER_DEBUG(40) << "search some bundles not known by " << task.eid.getString() << IBRCOMMON_LOGGER_ENDL;

							// blacklist the neighbor itself, because this is handles by neighbor routing extension
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
						} catch (const NeighborDatabase::NoMoreTransfersAvailable&) {
						} catch (const NeighborDatabase::NeighborNotAvailableException&) {
						} catch (const std::bad_cast&) { };

						try {
							dynamic_cast<ProcessBundleTask&>(*t);

							// new bundles are forwarded to all neighbors
							const std::set<dtn::core::Node> nl = dtn::core::BundleCore::getInstance().getNeighbors();

							for (std::set<dtn::core::Node>::const_iterator iter = nl.begin(); iter != nl.end(); iter++)
							{
								const dtn::core::Node &n = (*iter);

								// transfer the next bundle to this destination
								_taskqueue.push( new SearchNextBundleTask( n.getEID() ) );
							}
						} catch (const std::bad_cast&) { };
					} catch (const ibrcommon::Exception &ex) {
						IBRCOMMON_LOGGER_DEBUG(20) << "Exception occurred in FloodRoutingExtension: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
					}
				} catch (const std::exception&) {
					return;
				}

				yield();
			}
		}

		/****************************************/

		FloodRoutingExtension::SearchNextBundleTask::SearchNextBundleTask(const dtn::data::EID &e)
		 : eid(e)
		{ }

		FloodRoutingExtension::SearchNextBundleTask::~SearchNextBundleTask()
		{ }

		std::string FloodRoutingExtension::SearchNextBundleTask::toString()
		{
			return "SearchNextBundleTask: " + eid.getString();
		}

		/****************************************/

		FloodRoutingExtension::ProcessBundleTask::ProcessBundleTask(const dtn::data::MetaBundle &meta, const dtn::data::EID &o)
		 : bundle(meta), origin(o)
		{ }

		FloodRoutingExtension::ProcessBundleTask::~ProcessBundleTask()
		{ }

		std::string FloodRoutingExtension::ProcessBundleTask::toString()
		{
			return "ProcessBundleTask: " + bundle.toString();
		}
	}
}

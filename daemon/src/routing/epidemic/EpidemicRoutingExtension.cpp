/*
 * EpidemicRoutingExtension.cpp
 *
 *  Created on: 18.02.2010
 *      Author: morgenro
 */

#include "routing/epidemic/EpidemicRoutingExtension.h"

#include "routing/QueueBundleEvent.h"
#include "routing/NodeHandshakeEvent.h"
#include "net/TransferCompletedEvent.h"
#include "net/TransferAbortedEvent.h"
#include "core/TimeEvent.h"
#include "core/Node.h"
#include "net/ConnectionManager.h"
#include "Configuration.h"
#include "core/BundleCore.h"
#include "core/BundleEvent.h"

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
#include <set>

#include <stdlib.h>
#include <typeinfo>

namespace dtn
{
	namespace routing
	{
		EpidemicRoutingExtension::EpidemicRoutingExtension()
		{
			// write something to the syslog
			IBRCOMMON_LOGGER(info) << "Initializing epidemic routing module" << IBRCOMMON_LOGGER_ENDL;
		}

		EpidemicRoutingExtension::~EpidemicRoutingExtension()
		{
			stop();
			join();
		}

		void EpidemicRoutingExtension::stopExtension()
		{
			_taskqueue.abort();
		}

		void EpidemicRoutingExtension::requestHandshake(NodeHandshake &request) const
		{
			request.addRequest(BloomFilterSummaryVector::identifier);
		}

		void EpidemicRoutingExtension::notify(const dtn::core::Event *evt)
		{
			// If an incoming bundle is received, forward it to all connected neighbors
			try {
				dynamic_cast<const QueueBundleEvent&>(*evt);

				// new bundles trigger a recheck for all neighbors
				const std::set<dtn::core::Node> nl = dtn::core::BundleCore::getInstance().getNeighbors();

				for (std::set<dtn::core::Node>::const_iterator iter = nl.begin(); iter != nl.end(); iter++)
				{
					const dtn::core::Node &n = (*iter);

					// transfer the next bundle to this destination
					_taskqueue.push( new SearchNextBundleTask( n.getEID() ) );
				}
				return;
			} catch (const std::bad_cast&) { };

			try {
				const NodeHandshakeEvent &handshake = dynamic_cast<const NodeHandshakeEvent&>(*evt);

				if (handshake.state == NodeHandshakeEvent::HANDSHAKE_UPDATED)
				{
					// transfer the next bundle to this destination
					_taskqueue.push( new SearchNextBundleTask( handshake.peer ) );
				}
				else if (handshake.state == NodeHandshakeEvent::HANDSHAKE_COMPLETED)
				{
					// transfer the next bundle to this destination
					_taskqueue.push( new SearchNextBundleTask( handshake.peer ) );
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

				// create a transfer completed task
				_taskqueue.push( new TransferCompletedTask( completed.getPeer(), completed.getBundle() ) );
				return;
			} catch (const std::bad_cast&) { };
		}

		bool EpidemicRoutingExtension::__cancellation()
		{
			_taskqueue.abort();
			return true;
		}

		void EpidemicRoutingExtension::run()
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

					// do not forward any routing control message
					// this is done by the neighbor routing module
					if (meta.source.getApplication() == "/routing")
					{
						return false;
					}

					// do not forward local bundles
					if ((meta.destination.getNode() == dtn::core::BundleCore::local)
							&& meta.get(dtn::data::PrimaryBlock::DESTINATION_IS_SINGLETON)
						)
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
					// throws BloomfilterNotAvailableException if no filter is available or it is expired
					if (_entry.has(meta, true))
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

					IBRCOMMON_LOGGER_DEBUG(50) << "processing epidemic task " << t->toString() << IBRCOMMON_LOGGER_ENDL;

					try {
						/**
						 * Execute an executable task
						 */
						try {
							ExecutableTask &etask = dynamic_cast<ExecutableTask&>(*t);
							etask.execute();
						} catch (const std::bad_cast&) { };

						/**
						 * SearchNextBundleTask triggers a search for a bundle to transfer
						 * to another host. This Task is generated by TransferCompleted, TransferAborted
						 * and node events.
						 */
						try {
							SearchNextBundleTask &task = dynamic_cast<SearchNextBundleTask&>(*t);
							NeighborDatabase &db = (**this).getNeighborDB();

							ibrcommon::MutexLock l(db);
							NeighborDatabase::NeighborEntry &entry = db.get(task.eid);

							try {
								// get the bundle filter of the neighbor
								BundleFilter filter(entry);

								// some debug output
								IBRCOMMON_LOGGER_DEBUG(40) << "search some bundles not known by " << task.eid.getString() << IBRCOMMON_LOGGER_ENDL;

								// blacklist the neighbor itself, because this is handled by neighbor routing extension
								filter.blacklist(task.eid);

								// query some unknown bundle from the storage, the list contains max. 10 items.
								const std::list<dtn::data::MetaBundle> list = storage.get(filter);

								// send the bundles as long as we have resources
								for (std::list<dtn::data::MetaBundle>::const_iterator iter = list.begin(); iter != list.end(); iter++)
								{
									try {
										// transfer the bundle to the neighbor
										transferTo(entry, *iter);
									} catch (const NeighborDatabase::AlreadyInTransitException&) { };
								}
							} catch (const NeighborDatabase::BloomfilterNotAvailableException&) {
								// query a new summary vector from this neighbor
								NodeHandshakeEvent::raiseEvent(NodeHandshakeEvent::HANDSHAKE_REQUEST, task.eid);
							}
						} catch (const NeighborDatabase::NoMoreTransfersAvailable&) {
						} catch (const NeighborDatabase::NeighborNotAvailableException&) {
						} catch (const std::bad_cast&) { };

						/**
						 * transfer was completed
						 */
						try {
							TransferCompletedTask &task = dynamic_cast<TransferCompletedTask&>(*t);

							// transfer the next bundle to this destination
							_taskqueue.push( new SearchNextBundleTask( task.peer ) );
						} catch (const std::bad_cast&) { };

					} catch (const ibrcommon::Exception &ex) {
						IBRCOMMON_LOGGER_DEBUG(20) << "Exception occurred in EpidemicRoutingExtension: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
					}
				} catch (const std::exception&) {
					return;
				}

				yield();
			}
		}

		/****************************************/

		EpidemicRoutingExtension::SearchNextBundleTask::SearchNextBundleTask(const dtn::data::EID &e)
		 : eid(e)
		{ }

		EpidemicRoutingExtension::SearchNextBundleTask::~SearchNextBundleTask()
		{ }

		std::string EpidemicRoutingExtension::SearchNextBundleTask::toString()
		{
			return "SearchNextBundleTask: " + eid.getString();
		}

		/****************************************/

		EpidemicRoutingExtension::TransferCompletedTask::TransferCompletedTask(const dtn::data::EID &e, const dtn::data::MetaBundle &m)
		 : peer(e), meta(m)
		{ }

		EpidemicRoutingExtension::TransferCompletedTask::~TransferCompletedTask()
		{ }

		std::string EpidemicRoutingExtension::TransferCompletedTask::toString()
		{
			return "TransferCompletedTask";
		}
	}
}

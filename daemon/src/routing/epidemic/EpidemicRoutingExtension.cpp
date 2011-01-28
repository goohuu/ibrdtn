/*
 * EpidemicRoutingExtension.cpp
 *
 *  Created on: 18.02.2010
 *      Author: morgenro
 */

#include "routing/epidemic/EpidemicRoutingExtension.h"
#include "routing/epidemic/EpidemicControlMessage.h"

#include "routing/QueueBundleEvent.h"
#include "net/TransferCompletedEvent.h"
#include "net/TransferAbortedEvent.h"
#include "core/BundleExpiredEvent.h"
#include "core/NodeEvent.h"
#include "core/TimeEvent.h"
#include "core/Node.h"
#include "net/ConnectionManager.h"
#include "Configuration.h"
#include "core/BundleCore.h"
#include "core/BundleGeneratedEvent.h"

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

		void EpidemicRoutingExtension::notify(const dtn::core::Event *evt)
		{
			// If an incoming bundle is received, forward it to all connected neighbors
			try {
				const QueueBundleEvent &queued = dynamic_cast<const QueueBundleEvent&>(*evt);
				_taskqueue.push( new ProcessBundleTask(queued.bundle, queued.origin) );
				return;
			} catch (std::bad_cast ex) { };

			// On each time event look for expired stuff
			try {
				const dtn::core::TimeEvent &time = dynamic_cast<const dtn::core::TimeEvent&>(*evt);

				if (time.getAction() == dtn::core::TIME_SECOND_TICK)
				{
					// check all lists for expired entries
					_taskqueue.push( new ExpireTask(time.getTimestamp()) );
				}
				return;
			} catch (std::bad_cast ex) { };

			// If a new neighbor comes available, send him a request for the summary vector
			// If a neighbor went away we can free the stored summary vector
			try {
				const dtn::core::NodeEvent &nodeevent = dynamic_cast<const dtn::core::NodeEvent&>(*evt);
				const dtn::core::Node &n = nodeevent.getNode();

				if (nodeevent.getAction() == NODE_AVAILABLE)
				{
					NeighborDatabase &db = (**this).getNeighborDB();

					try {
						// lock the list of neighbors
						ibrcommon::MutexLock l(db);
						NeighborDatabase::NeighborEntry &entry = db.get(n.getEID());

						// acquire resources to send a summary vector request
						entry.acquireFilterRequest();

						// query a new summary vector from this neighbor
						_taskqueue.push( new QuerySummaryVectorTask( n.getEID() ) );
					} catch (const NeighborDatabase::NoMoreTransfersAvailable&) {
					} catch (const NeighborDatabase::NeighborNotAvailableException&) { };
				}

				return;
			} catch (std::bad_cast ex) { };

			// The bundle transfer has been aborted
			try {
				const dtn::net::TransferAbortedEvent &aborted = dynamic_cast<const dtn::net::TransferAbortedEvent&>(*evt);

				// transfer the next bundle to this destination
				_taskqueue.push( new SearchNextBundleTask( aborted.getPeer() ) );

				return;
			} catch (std::bad_cast ex) { };

			// A bundle transfer was successful
			try {
				const dtn::net::TransferCompletedEvent &completed = dynamic_cast<const dtn::net::TransferCompletedEvent&>(*evt);

				// create a transfer completed task
				_taskqueue.push( new TransferCompletedTask( completed.getPeer(), completed.getBundle() ) );
				return;
			} catch (std::bad_cast ex) { };
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
				BundleFilter(const ibrcommon::BloomFilter &filter)
				 : _filter(filter)
				{};

				virtual ~BundleFilter() {};

				virtual size_t limit() const { return 10; };

				virtual bool shouldAdd(const dtn::data::MetaBundle &meta) const
				{
					// do not forward any epidemic control message
					// this is done by the neighbor routing module
					if (meta.source == (dtn::core::BundleCore::local + "/routing/epidemic"))
					{
						return false;
					}

					// do not forward to any blacklisted destination
					const dtn::data::EID dest = meta.destination.getNodeEID();
					if (_blacklist.find(dest) != _blacklist.end())
					{
						return false;
					}

					// do not forward bundles already known by the destination
					if (_filter.contains(meta.toString()))
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
				const ibrcommon::BloomFilter &_filter;
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
						} catch (std::bad_cast) { };

						/**
						 * The ExpireTask take care of expired bundles in the purge vector
						 */
						try {
							ExpireTask &task = dynamic_cast<ExpireTask&>(*t);
							_purge_vector.expire(task.timestamp);
						} catch (std::bad_cast) { };

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
								// getBundles throws BloomfilterNotAvailableException if no filter is available or it is expired
								BundleFilter filter(entry.getBundles());

								// some debug output
								IBRCOMMON_LOGGER_DEBUG(40) << "search some bundles not known by " << task.eid.getString() << IBRCOMMON_LOGGER_ENDL;

								// blacklist the neighbor itself, because this is handles by neighbor routing extension
								filter.blacklist(task.eid);

								// query an unknown bundle from the storage, the list contains max. 10 items.
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
								// acquire resources to send a summary vector request
								entry.acquireFilterRequest();

								// query a new summary vector from this neighbor
								_taskqueue.push( new QuerySummaryVectorTask( task.eid ) );
							}
						} catch (const NeighborDatabase::NoMoreTransfersAvailable&) {
						} catch (const NeighborDatabase::NeighborNotAvailableException&) {
						} catch (std::bad_cast) { };

						/**
						 * transfer was completed
						 */
						try {
							TransferCompletedTask &task = dynamic_cast<TransferCompletedTask&>(*t);

							// add this bundle to the purge vector if it is delivered to its destination
							if (( EID(task.peer.getNodeEID()) == EID(task.meta.destination.getNodeEID()) ) && (task.meta.procflags & dtn::data::Bundle::DESTINATION_IS_SINGLETON))
							{
								IBRCOMMON_LOGGER_DEBUG(15) << "singleton bundle delivered: " << task.meta.toString() << IBRCOMMON_LOGGER_ENDL;

								try {
									// add it to the purge vector
									_purge_vector.add(task.meta);
								} catch (const dtn::core::BundleStorage::NoBundleFoundException&) { };
							}

							// transfer the next bundle to this destination
							_taskqueue.push( new SearchNextBundleTask( task.peer ) );
						} catch (std::bad_cast) { };

						/**
						 * process a received bundle
						 */
						try {
							ProcessBundleTask &task = dynamic_cast<ProcessBundleTask&>(*t);

							// look for ECM bundles
							if ( task.bundle.destination == (dtn::core::BundleCore::local + "/routing/epidemic") )
							{
								// the bundle is an control message, get it from the storage
								dtn::data::Bundle bundle = storage.get(task.bundle);

								// process the incoming ECM
								processECM(task.origin, bundle);

								// we do not need it anymore in the storage
								storage.remove(bundle);
							}
							else
							{
								// new bundles trigger a recheck for all neighbors
								const std::set<dtn::core::Node> nl = dtn::core::BundleCore::getInstance().getNeighbors();

								for (std::set<dtn::core::Node>::const_iterator iter = nl.begin(); iter != nl.end(); iter++)
								{
									const dtn::core::Node &n = (*iter);

									// transfer the next bundle to this destination
									_taskqueue.push( new SearchNextBundleTask( n.getEID() ) );
								}
							}
						} catch (const dtn::core::BundleStorage::NoBundleFoundException&) {
							// if the bundle is not in the storage we have nothing to do
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

		EpidemicRoutingExtension::ProcessBundleTask::ProcessBundleTask(const dtn::data::MetaBundle &meta, const dtn::data::EID &o)
		 : bundle(meta), origin(o)
		{ }

		EpidemicRoutingExtension::ProcessBundleTask::~ProcessBundleTask()
		{ }

		std::string EpidemicRoutingExtension::ProcessBundleTask::toString()
		{
			return "ProcessBundleTask: " + bundle.toString();
		}

		/****************************************/

		EpidemicRoutingExtension::ExpireTask::ExpireTask(const size_t t)
		 : timestamp(t)
		{ }

		EpidemicRoutingExtension::ExpireTask::~ExpireTask()
		{ }

		std::string EpidemicRoutingExtension::ExpireTask::toString()
		{
			return "ExpireTask";
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

		/****************************************/

		EpidemicRoutingExtension::QuerySummaryVectorTask::QuerySummaryVectorTask(const dtn::data::EID &o)
		 : origin(o)
		{ }

		EpidemicRoutingExtension::QuerySummaryVectorTask::~QuerySummaryVectorTask()
		{ }

		void EpidemicRoutingExtension::QuerySummaryVectorTask::execute() const
		{
			// create a new request for the summary vector of the neighbor
			EpidemicControlMessage ecm;

			// set message type
			ecm.type = EpidemicControlMessage::ECM_QUERY_SUMMARY_VECTOR;

			// create a new bundle
			dtn::data::Bundle req;

			// set the source of the bundle
			req._source = dtn::core::BundleCore::local + "/routing/epidemic";

			// set the destination of the bundle
			req.set(dtn::data::PrimaryBlock::DESTINATION_IS_SINGLETON, true);
			req._destination = origin + "/routing/epidemic";

			// limit the lifetime to 60 seconds
			req._lifetime = 60;

			// set high priority
			req.set(dtn::data::PrimaryBlock::PRIORITY_BIT1, true);
			req.set(dtn::data::PrimaryBlock::PRIORITY_BIT2, true);

			dtn::data::PayloadBlock &p = req.push_back<PayloadBlock>();
			ibrcommon::BLOB::Reference ref = p.getBLOB();

			// serialize the request into the payload
			{
				ibrcommon::BLOB::iostream ios = ref.iostream();
				(*ios) << ecm;
			}

			// transfer the bundle to the neighbor
			dtn::core::BundleGeneratedEvent::raise(req);
		}

		std::string EpidemicRoutingExtension::QuerySummaryVectorTask::toString()
		{
			return "QuerySummaryVectorTask: " + origin.getString();
		}

		void EpidemicRoutingExtension::processECM(const dtn::data::EID &origin, const dtn::data::Bundle &bundle)
		{
			// read the ecm
			const dtn::data::PayloadBlock &p = bundle.getBlock<dtn::data::PayloadBlock>();
			ibrcommon::BLOB::Reference ref = p.getBLOB();
			EpidemicControlMessage ecm;

			// locked within this region
			{
				ibrcommon::BLOB::iostream s = ref.iostream();
				(*s) >> ecm;
			}

			// if this is a request answer with an summary vector
			if (ecm.type == EpidemicControlMessage::ECM_QUERY_SUMMARY_VECTOR)
			{
				// create a new request for the summary vector of the neighbor
				EpidemicControlMessage response_ecm;

				// set message type
				response_ecm.type = EpidemicControlMessage::ECM_RESPONSE;

				// add own summary vector to the message
				const SummaryVector vec = (**this).getSummaryVector();
				response_ecm.setSummaryVector(vec);

				// add own purge vector to the message
				response_ecm.setPurgeVector(_purge_vector);

				// create a new bundle
				dtn::data::Bundle answer;

				// set the source of the bundle
				answer._source = dtn::core::BundleCore::local + "/routing/epidemic";

				// set the destination of the bundle
				answer.set(dtn::data::PrimaryBlock::DESTINATION_IS_SINGLETON, true);
				answer._destination = bundle._source;

				// limit the lifetime to 60 seconds
				answer._lifetime = 60;

				// set high priority
				answer.set(dtn::data::PrimaryBlock::PRIORITY_BIT1, true);
				answer.set(dtn::data::PrimaryBlock::PRIORITY_BIT2, true);

				dtn::data::PayloadBlock &p = answer.push_back<PayloadBlock>();
				ibrcommon::BLOB::Reference ref = p.getBLOB();

				// serialize the request into the payload
				{
					ibrcommon::BLOB::iostream ios = ref.iostream();
					(*ios) << response_ecm;
				}

				// transfer the bundle to the neighbor
				dtn::core::BundleGeneratedEvent::raise(answer);
			}
			else if (ecm.type == EpidemicControlMessage::ECM_RESPONSE)
			{
				if (ecm.flags & EpidemicControlMessage::ECM_CONTAINS_SUMMARY_VECTOR)
				{
					// get the summary vector (bloomfilter) of this ECM
					const ibrcommon::BloomFilter &filter = ecm.getSummaryVector().getBloomFilter();

					/**
					 * Update the neighbor database with the received filter.
					 * The filter was sent by the owner, so we assign the contained summary vector to
					 * the EID of the sender of this bundle.
					 */
					{
						NeighborDatabase &db = (**this).getNeighborDB();
						ibrcommon::MutexLock l(db);
						db.updateBundles(bundle._source.getNodeEID(), filter, bundle._lifetime);
					}

					// trigger the search-for-next-bundle procedure
					_taskqueue.push( new SearchNextBundleTask( origin ) );
				}

				if (ecm.flags & EpidemicControlMessage::ECM_CONTAINS_PURGE_VECTOR)
				{
					// get the purge vector (bloomfilter) of this ECM
					const ibrcommon::BloomFilter &purge = ecm.getPurgeVector().getBloomFilter();

					dtn::core::BundleStorage &storage = (**this).getStorage();

					try {
						while (true)
						{
							// delete bundles in the purge vector
							const dtn::data::MetaBundle meta = storage.remove(purge);

							IBRCOMMON_LOGGER_DEBUG(15) << "bundle purged: " << meta.toString() << IBRCOMMON_LOGGER_ENDL;

							// add this bundle to the own purge vector
							_purge_vector.add(meta);
						}
					} catch (const dtn::core::BundleStorage::NoBundleFoundException&) { };
				}
			}
		}
	}
}

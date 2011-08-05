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
#include "core/BundleEvent.h"

#include <ibrdtn/data/ScopeControlHopLimitBlock.h>
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
		 : _endpoint(_taskqueue, _purge_vector)
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

			// On each time event look for expired stuff
			try {
				const dtn::core::TimeEvent &time = dynamic_cast<const dtn::core::TimeEvent&>(*evt);

				if (time.getAction() == dtn::core::TIME_SECOND_TICK)
				{
					// check all lists for expired entries
					_taskqueue.push( new ExpireTask(time.getTimestamp()) );
				}
				return;
			} catch (const std::bad_cast&) { };

			// If a new neighbor comes available, send him a request for the summary vector
			// If a neighbor went away we can free the stored summary vector
			try {
				const dtn::core::NodeEvent &nodeevent = dynamic_cast<const dtn::core::NodeEvent&>(*evt);
				const dtn::core::Node &n = nodeevent.getNode();

				if (nodeevent.getAction() == NODE_AVAILABLE)
				{
					// query a new summary vector from this neighbor
					_taskqueue.push( new QuerySummaryVectorTask( (**this).getNeighborDB(), n.getEID(), _endpoint ) );
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

					// check Scope Control Block - do not forward non-group bundles with hop limit <= 1
					if ((meta.hopcount <= 1) && (meta.get(dtn::data::PrimaryBlock::DESTINATION_IS_SINGLETON)))
					{
						return false;
					}

					// do not forward any epidemic control message
					// this is done by the neighbor routing module
					if (meta.source == (dtn::core::BundleCore::local + "/routing/epidemic"))
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
						 * The ExpireTask take care of expired bundles in the purge vector
						 */
						try {
							ExpireTask &task = dynamic_cast<ExpireTask&>(*t);
							_purge_vector.expire(task.timestamp);
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
								_taskqueue.push( new QuerySummaryVectorTask( (**this).getNeighborDB(), task.eid, _endpoint ) );
							}
						} catch (const NeighborDatabase::NoMoreTransfersAvailable&) {
						} catch (const NeighborDatabase::NeighborNotAvailableException&) {
						} catch (const std::bad_cast&) { };

						/**
						 * transfer was completed
						 */
						try {
							TransferCompletedTask &task = dynamic_cast<TransferCompletedTask&>(*t);

							try {
								// add this bundle to the purge vector if it is delivered to its destination
								if (( task.peer.getNode() == task.meta.destination.getNode() ) && (task.meta.procflags & dtn::data::Bundle::DESTINATION_IS_SINGLETON))
								{
									IBRCOMMON_LOGGER(notice) << "singleton bundle added to purge vector: " << task.meta.toString() << IBRCOMMON_LOGGER_ENDL;

									// add it to the purge vector
									_purge_vector.add(task.meta);
								}
							} catch (const dtn::core::BundleStorage::NoBundleFoundException&) { };

							// transfer the next bundle to this destination
							_taskqueue.push( new SearchNextBundleTask( task.peer ) );
						} catch (const std::bad_cast&) { };

						/**
						 * process a epidemic bundle
						 */
						try {
							const ProcessEpidemicBundleTask &task = dynamic_cast<ProcessEpidemicBundleTask&>(*t);
							processECM(task.bundle);
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

		EpidemicRoutingExtension::ProcessEpidemicBundleTask::ProcessEpidemicBundleTask(const dtn::data::Bundle &b)
		 : bundle(b)
		{ }

		EpidemicRoutingExtension::ProcessEpidemicBundleTask::~ProcessEpidemicBundleTask()
		{ }

		std::string EpidemicRoutingExtension::ProcessEpidemicBundleTask::toString()
		{
			return "ProcessEpidemicBundleTask: " + bundle.toString();
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

		EpidemicRoutingExtension::QuerySummaryVectorTask::QuerySummaryVectorTask(NeighborDatabase &db, const dtn::data::EID &o, EpidemicEndpoint &e)
		 : ndb(db), origin(o), endpoint(e)
		{ }

		EpidemicRoutingExtension::QuerySummaryVectorTask::~QuerySummaryVectorTask()
		{ }

		void EpidemicRoutingExtension::QuerySummaryVectorTask::execute() const
		{
			try {
				// lock the list of neighbors
				ibrcommon::MutexLock l(ndb);
				NeighborDatabase::NeighborEntry &entry = ndb.get(origin);

				// acquire resources to send a summary vector request
				entry.acquireFilterRequest();

				// call the query method at the epidemic endpoint instance
				endpoint.query(origin);
			} catch (const NeighborDatabase::NoMoreTransfersAvailable&) {
			} catch (const NeighborDatabase::NeighborNotAvailableException&) { };
		}

		std::string EpidemicRoutingExtension::QuerySummaryVectorTask::toString()
		{
			return "QuerySummaryVectorTask: " + origin.getString();
		}

		EpidemicRoutingExtension::EpidemicEndpoint::EpidemicEndpoint(ibrcommon::Queue<EpidemicRoutingExtension::Task* > &queue, dtn::routing::BundleSummary &purge)
		 : _taskqueue(queue), _purge_vector(purge)
		{
			AbstractWorker::initialize("/routing/epidemic", true);
		}

		EpidemicRoutingExtension::EpidemicEndpoint::~EpidemicEndpoint()
		{
		}

		void EpidemicRoutingExtension::EpidemicEndpoint::callbackBundleReceived(const Bundle &b)
		{
			_taskqueue.push( new ProcessEpidemicBundleTask(b) );
		}

		void EpidemicRoutingExtension::EpidemicEndpoint::send(const dtn::data::Bundle &b)
		{
			transmit(b);
		}

		void EpidemicRoutingExtension::EpidemicEndpoint::query(const dtn::data::EID &origin)
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
			req.set(dtn::data::PrimaryBlock::PRIORITY_BIT1, false);
			req.set(dtn::data::PrimaryBlock::PRIORITY_BIT2, true);

			dtn::data::PayloadBlock &p = req.push_back<PayloadBlock>();
			ibrcommon::BLOB::Reference ref = p.getBLOB();

			// serialize the request into the payload
			{
				ibrcommon::BLOB::iostream ios = ref.iostream();
				(*ios) << ecm;
			}

			// add a schl block
			dtn::data::ScopeControlHopLimitBlock &schl = req.push_front<dtn::data::ScopeControlHopLimitBlock>();
			schl.setLimit(1);

			// send the bundle
			transmit(req);
		}

		void EpidemicRoutingExtension::processECM(const dtn::data::Bundle &bundle)
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
				answer.set(dtn::data::PrimaryBlock::PRIORITY_BIT1, false);
				answer.set(dtn::data::PrimaryBlock::PRIORITY_BIT2, true);

				dtn::data::PayloadBlock &p = answer.push_back<PayloadBlock>();
				ibrcommon::BLOB::Reference ref = p.getBLOB();

				// serialize the request into the payload
				{
					ibrcommon::BLOB::iostream ios = ref.iostream();
					(*ios) << response_ecm;
				}

				// add a schl block
				dtn::data::ScopeControlHopLimitBlock &schl = answer.push_front<dtn::data::ScopeControlHopLimitBlock>();
				schl.setLimit(1);

				// transfer the bundle to the neighbor
				_endpoint.send(answer);
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
					try {
						NeighborDatabase &db = (**this).getNeighborDB();
						ibrcommon::MutexLock l(db);
						NeighborDatabase::NeighborEntry &entry = db.get(bundle._source.getNode());
						entry.update(filter, bundle._lifetime);

						// trigger the search-for-next-bundle procedure
						_taskqueue.push( new SearchNextBundleTask( entry.eid ) );
					} catch (const NeighborDatabase::NeighborNotAvailableException&) { };
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

							// log the purged bundle
							IBRCOMMON_LOGGER(notice) << "bundle purged: " << meta.toString() << IBRCOMMON_LOGGER_ENDL;

							// gen a report
							dtn::core::BundleEvent::raise(meta, BUNDLE_DELETED, StatusReportBlock::DEPLETED_STORAGE);

							// add this bundle to the own purge vector
							_purge_vector.add(meta);
						}
					} catch (const dtn::core::BundleStorage::NoBundleFoundException&) { };
				}
			}
		}
	}
}

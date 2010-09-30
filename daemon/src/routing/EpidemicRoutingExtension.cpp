/*
 * EpidemicRoutingExtension.cpp
 *
 *  Created on: 18.02.2010
 *      Author: morgenro
 */

#include "routing/EpidemicRoutingExtension.h"
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
#include "core/SimpleBundleStorage.h"

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
		struct FindNode: public std::binary_function< dtn::core::Node, dtn::core::Node, bool > {
			bool operator () ( const dtn::core::Node &n1, const dtn::core::Node &n2 ) const {
				return n1 == n2;
			}
		};

		EpidemicRoutingExtension::EpidemicRoutingExtension()
		{
			// get configuration
			dtn::daemon::Configuration &conf = dtn::daemon::Configuration::getInstance();

			// write something to the syslog
			IBRCOMMON_LOGGER(info) << "Initializing epidemic routing module for node " << conf.getNodename() << IBRCOMMON_LOGGER_ENDL;
		}

		EpidemicRoutingExtension::~EpidemicRoutingExtension()
		{
			stopExtension();
			join();
		}

		void EpidemicRoutingExtension::stopExtension()
		{
			_taskqueue.abort();
		}

		void EpidemicRoutingExtension::update(std::string &name, std::string &data)
		{
			name = "epidemic";
			data = "version=1";
		}

		void EpidemicRoutingExtension::notify(const dtn::core::Event *evt)
		{
			try {
				const QueueBundleEvent &queued = dynamic_cast<const QueueBundleEvent&>(*evt);
				_taskqueue.push( new ProcessBundleTask(queued.bundle, queued.origin) );
			} catch (std::bad_cast ex) { };

			try {
				const dtn::core::TimeEvent &time = dynamic_cast<const dtn::core::TimeEvent&>(*evt);

				if (time.getAction() == dtn::core::TIME_SECOND_TICK)
				{
					// check all lists for expired entries
					_taskqueue.push( new ExpireTask(time.getTimestamp()) );
				}
			} catch (std::bad_cast ex) { };

			try {
				const dtn::core::NodeEvent &nodeevent = dynamic_cast<const dtn::core::NodeEvent&>(*evt);

				dtn::core::Node n = nodeevent.getNode();
				dtn::data::EID eid(n.getURI());

				switch (nodeevent.getAction())
				{
					case NODE_AVAILABLE:
					{
						ibrcommon::MutexLock l(_list_mutex);
						_neighbors.setAvailable(eid);

						_taskqueue.push( new UpdateSummaryVectorTask( eid ) );
					}
					break;

					case NODE_UNAVAILABLE:
					{
						ibrcommon::MutexLock l(_list_mutex);
						_neighbors.setUnavailable(eid);
					}
					break;

					default:
						break;
				}
			} catch (std::bad_cast ex) { };

			// TransferAbortedEvent: send next bundle
			try {
				const dtn::net::TransferAbortedEvent &aborted = dynamic_cast<const dtn::net::TransferAbortedEvent&>(*evt);
				dtn::data::EID eid = aborted.getPeer();
				dtn::data::BundleID id = aborted.getBundleID();

				switch (aborted.reason)
				{
					case dtn::net::TransferAbortedEvent::REASON_UNDEFINED:
					{
						break;
					}

					case dtn::net::TransferAbortedEvent::REASON_RETRY_LIMIT_REACHED:
					{
						// transfer the next bundle to this destination
						_taskqueue.push( new SearchNextBundleTask( eid ) );
						break;
					}

					case dtn::net::TransferAbortedEvent::REASON_CONNECTION_DOWN:
					{
						break;
					}

					// How to exclude several bundles? Combine two bloomfilters? Use a blocklist. Delay transfers to the node?
					case dtn::net::TransferAbortedEvent::REASON_REFUSED:
					{
						// add the transferred bundle to the bloomfilter of the receiver
						ibrcommon::MutexLock l(_list_mutex);
						ibrcommon::BloomFilter &bf = _neighbors.get(eid)._filter;
						bf.insert(id.toString());

						if (IBRCOMMON_LOGGER_LEVEL >= 40)
						{
							IBRCOMMON_LOGGER_DEBUG(40) << "bloomfilter false-positive propability is " << bf.getAllocation() << IBRCOMMON_LOGGER_ENDL;
						}

						// transfer the next bundle to this destination
						_taskqueue.push( new SearchNextBundleTask( eid ) );
						break;
					}

					case dtn::net::TransferAbortedEvent::REASON_BUNDLE_DELETED:
					{
						// transfer the next bundle to this destination
						_taskqueue.push( new SearchNextBundleTask( eid ) );
						break;
					}
				}
			} catch (std::bad_cast ex) { };

			try {
				const dtn::net::TransferCompletedEvent &completed = dynamic_cast<const dtn::net::TransferCompletedEvent&>(*evt);

				dtn::data::EID eid = completed.getPeer();
				dtn::data::MetaBundle meta = completed.getBundle();

				// delete the bundle in the storage if
				if ( EID(eid.getNodeEID()) == EID(meta.destination.getNodeEID()) )
				{
					try {
						// bundle has been delivered to its destination
						// TODO: generate a "delete" message for routing algorithm

						// delete it from our storage
						getRouter()->getStorage().remove(meta);

						IBRCOMMON_LOGGER_DEBUG(15) << "bundle delivered: " << meta.toString() << IBRCOMMON_LOGGER_ENDL;

						// add it to the purge vector
						_purge_vector.add(meta);
					} catch (dtn::core::BundleStorage::NoBundleFoundException ex) {

					}
				}
				else
				{
					// add the transferred bundle to the bloomfilter of the receiver
					ibrcommon::MutexLock l(_list_mutex);
					ibrcommon::BloomFilter &bf = _neighbors.get(eid)._filter;
					bf.insert(meta.toString());

					if (IBRCOMMON_LOGGER_LEVEL >= 40)
					{
						IBRCOMMON_LOGGER_DEBUG(40) << "bloomfilter false-positive propability is " << bf.getAllocation() << IBRCOMMON_LOGGER_ENDL;
					}
				}

				// transfer the next bundle to this destination
				_taskqueue.push( new SearchNextBundleTask( eid ) );

			} catch (std::bad_cast ex) { };
		}

		void EpidemicRoutingExtension::transferEpidemicInformation(const dtn::data::EID &eid)
		{
			/**
			 * create a routing bundle
			 * which contains a summary vector
			 */
			dtn::data::Bundle routingbundle;
			routingbundle._lifetime = 10;
			routingbundle._source = dtn::core::BundleCore::local;
			routingbundle._destination = EID("dtn:epidemic-routing");

			// create a new epidemic block
			{
				// lock the lists
				ibrcommon::MutexLock l(_list_mutex);
				EpidemicExtensionBlock &eblock = routingbundle.push_back<EpidemicExtensionBlock>();
				const SummaryVector vec = getRouter()->getSummaryVector();
				eblock.setSummaryVector(vec);
				eblock.setPurgeVector(_purge_vector);
			}

			getRouter()->transferTo(eid, routingbundle);
		}

		void EpidemicRoutingExtension::run()
		{
			bool sendVectorUpdate = false;

			while (true)
			{
				try {
					Task *t = _taskqueue.getnpop(true);
					ibrcommon::AutoDelete<Task> killer(t);

					IBRCOMMON_LOGGER_DEBUG(10) << "processing epidemic task " << t->toString() << IBRCOMMON_LOGGER_ENDL;

					try {
						try {
							ExpireTask &task = dynamic_cast<ExpireTask&>(*t);

							// should i send a vector update?
							if (sendVectorUpdate)
							{
								sendVectorUpdate = false;
								_taskqueue.push( new BroadcastSummaryVectorTask() );
							}

							ibrcommon::MutexLock l(_list_mutex);
							_seenlist.expire(task.timestamp);
							_purge_vector.expire(task.timestamp);
						} catch (std::bad_cast) { };

						try {
							SearchNextBundleTask &task = dynamic_cast<SearchNextBundleTask&>(*t);

							IBRCOMMON_LOGGER_DEBUG(40) << "search one bundle not known by " << task.eid.getString() << IBRCOMMON_LOGGER_ENDL;

							ibrcommon::MutexLock l(_list_mutex);
							ibrcommon::BloomFilter &bf = _neighbors.get(task.eid)._filter;
							dtn::data::Bundle b = getRouter()->getStorage().get(bf);
							getRouter()->transferTo(task.eid, b);

						} catch (dtn::core::BundleStorage::NoBundleFoundException) {
						} catch (std::bad_cast) { };

						try {
							dynamic_cast<BroadcastSummaryVectorTask&>(*t);

							std::set<dtn::data::EID> list;
							{
								ibrcommon::MutexLock l(_list_mutex);
								list = _neighbors.getAvailable();
							}

							for (std::set<dtn::data::EID>::const_iterator iter = list.begin(); iter != list.end(); iter++)
							{
								transferEpidemicInformation(*iter);
							}

						} catch (std::bad_cast) { };

						try {
							UpdateSummaryVectorTask &task = dynamic_cast<UpdateSummaryVectorTask&>(*t);
							transferEpidemicInformation(task.eid);
						} catch (std::bad_cast) { };

						try {
							ProcessBundleTask &task = dynamic_cast<ProcessBundleTask&>(*t);
							ibrcommon::MutexLock l(_list_mutex);

							// check for special addresses
							if (task.bundle.destination == EID("dtn:epidemic-routing"))
							{
								// get the bundle out of the storage
								dtn::data::Bundle bundle = getRouter()->getStorage().get(task.bundle);

								// get all epidemic extension blocks of this bundle
								const EpidemicExtensionBlock &ext = bundle.getBlock<EpidemicExtensionBlock>();

								// get the bloomfilter of this bundle
								const ibrcommon::BloomFilter &filter = ext.getSummaryVector().getBloomFilter();
								const ibrcommon::BloomFilter &purge = ext.getPurgeVector().getBloomFilter();

								// update the neighbor database with this filter
								_neighbors.updateBundles(task.origin, filter);

								while (true)
								{
									// delete bundles in the purge vector
									dtn::data::MetaBundle meta = getRouter()->getStorage().remove(purge);

									IBRCOMMON_LOGGER_DEBUG(15) << "bundle purged: " << meta.toString() << IBRCOMMON_LOGGER_ENDL;

									// add this bundle to the own purge vector
									_purge_vector.add(meta);
								}

								// remove the bundle
								getRouter()->getStorage().remove(bundle);
							}
							else
							{
								// trigger new transmission of the summary vector
								sendVectorUpdate = true;

								// trigger transmission for all neighbors
								std::set<dtn::data::EID> list;
								{
									list = _neighbors.getAvailable();
								}

								for (std::set<dtn::data::EID>::const_iterator iter = list.begin(); iter != list.end(); iter++)
								{
									// transfer the next bundle to this destination
									_taskqueue.push( new SearchNextBundleTask( *iter ) );
								}
							}
						} catch (dtn::data::Bundle::NoSuchBlockFoundException) {
							// if the bundle does not contains the expected block
						} catch (dtn::core::BundleStorage::NoBundleFoundException) {
							// if the bundle is not in the storage we have nothing to do
						} catch (std::bad_cast) { };
					} catch (ibrcommon::Exception ex) {
						IBRCOMMON_LOGGER(error) << "Exception occurred in EpidemicRoutingExtension: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
					}
				} catch (std::exception) {
					return;
				}
			}
		}

		bool EpidemicRoutingExtension::wasSeenBefore(const dtn::data::BundleID &id) const
		{
			return ( std::find( _seenlist.begin(), _seenlist.end(), id ) != _seenlist.end());
		}

		dtn::data::Block* EpidemicRoutingExtension::EpidemicExtensionBlock::Factory::create()
		{
			return new EpidemicRoutingExtension::EpidemicExtensionBlock();
		}

		EpidemicRoutingExtension::EpidemicExtensionBlock::EpidemicExtensionBlock()
		 : dtn::data::Block(EpidemicExtensionBlock::BLOCK_TYPE), _data("forwarded through epidemic routing")
		{
		}

		EpidemicRoutingExtension::EpidemicExtensionBlock::~EpidemicExtensionBlock()
		{
		}

		void EpidemicRoutingExtension::EpidemicExtensionBlock::setSummaryVector(const SummaryVector &vector)
		{
			_vector = vector;
		}

		const SummaryVector& EpidemicRoutingExtension::EpidemicExtensionBlock::getSummaryVector() const
		{
			return _vector;
		}

		void EpidemicRoutingExtension::EpidemicExtensionBlock::setPurgeVector(const SummaryVector &vector)
		{
			_purge = vector;
		}

		const SummaryVector& EpidemicRoutingExtension::EpidemicExtensionBlock::getPurgeVector() const
		{
			return _purge;
		}

		void EpidemicRoutingExtension::EpidemicExtensionBlock::set(dtn::data::SDNV value)
		{
			_counter = value;
		}

		dtn::data::SDNV EpidemicRoutingExtension::EpidemicExtensionBlock::get() const
		{
			return _counter;
		}

		size_t EpidemicRoutingExtension::EpidemicExtensionBlock::getLength() const
		{
			return _counter.getLength() + _data.getLength() + _vector.getLength();
		}

		std::istream& EpidemicRoutingExtension::EpidemicExtensionBlock::deserialize(std::istream &stream)
		{
			stream >> _counter;
			stream >> _data;
			stream >> _vector;
			stream >> _purge;

			// unset block not processed bit
			dtn::data::Block::set(dtn::data::Block::FORWARDED_WITHOUT_PROCESSED, false);

			return stream;
		}

		std::ostream &EpidemicRoutingExtension::EpidemicExtensionBlock::serialize(std::ostream &stream) const
		{
			stream << _counter;
			stream << _data;
			stream << _vector;
			stream << _purge;

			return stream;
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

		EpidemicRoutingExtension::BroadcastSummaryVectorTask::BroadcastSummaryVectorTask()
		{ }

		EpidemicRoutingExtension::BroadcastSummaryVectorTask::~BroadcastSummaryVectorTask()
		{ }

		std::string EpidemicRoutingExtension::BroadcastSummaryVectorTask::toString()
		{
			return "BroadcastSummaryVectorTask";
		}

		/****************************************/

		EpidemicRoutingExtension::UpdateSummaryVectorTask::UpdateSummaryVectorTask(const dtn::data::EID &e)
		 : eid(e)
		{ }

		EpidemicRoutingExtension::UpdateSummaryVectorTask::~UpdateSummaryVectorTask()
		{ }

		std::string EpidemicRoutingExtension::UpdateSummaryVectorTask::toString()
		{
			return "UpdateSummaryVectorTask: " + eid.getString();
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
	}
}

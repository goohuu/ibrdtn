/*
 * FloodRoutingExtension.cpp
 *
 *  Created on: 18.02.2010
 *      Author: morgenro
 */

#include "routing/FloodRoutingExtension.h"
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

		FloodRoutingExtension::FloodRoutingExtension()
		{
			// get configuration
			dtn::daemon::Configuration &conf = dtn::daemon::Configuration::getInstance();

			// write something to the syslog
			IBRCOMMON_LOGGER(info) << "Initializing epidemic routing module for node " << conf.getNodename() << IBRCOMMON_LOGGER_ENDL;
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
				_taskqueue.push( new ProcessBundleTask(queued.bundle) );
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

						_taskqueue.push( new SearchNextBundleTask( eid ) );
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
					} catch (const dtn::core::BundleStorage::NoBundleFoundException&) {

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

		bool FloodRoutingExtension::__cancellation()
		{
			_taskqueue.abort();
			return true;
		}

		void FloodRoutingExtension::run()
		{
			while (true)
			{
				try {
					Task *t = _taskqueue.getnpop(true);
					ibrcommon::AutoDelete<Task> killer(t);

					IBRCOMMON_LOGGER_DEBUG(10) << "processing epidemic task " << t->toString() << IBRCOMMON_LOGGER_ENDL;

					try {
						try {
							ExpireTask &task = dynamic_cast<ExpireTask&>(*t);

							ibrcommon::MutexLock l(_list_mutex);
							_purge_vector.expire(task.timestamp);
						} catch (std::bad_cast) { };

						try {
							SearchNextBundleTask &task = dynamic_cast<SearchNextBundleTask&>(*t);

							IBRCOMMON_LOGGER_DEBUG(40) << "search one bundle not known by " << task.eid.getString() << IBRCOMMON_LOGGER_ENDL;

							ibrcommon::MutexLock l(_list_mutex);
							ibrcommon::BloomFilter &bf = _neighbors.get(task.eid)._filter;
							dtn::data::Bundle b = getRouter()->getStorage().get(bf);
							getRouter()->transferTo(task.eid, b);

						} catch (const dtn::core::BundleStorage::NoBundleFoundException&) {
						} catch (std::bad_cast) { };

						try {
							dynamic_cast<ProcessBundleTask&>(*t);
							ibrcommon::MutexLock l(_list_mutex);

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
						} catch (dtn::data::Bundle::NoSuchBlockFoundException) {
							// if the bundle does not contains the expected block
						} catch (const dtn::core::BundleStorage::NoBundleFoundException&) {
							// if the bundle is not in the storage we have nothing to do
						} catch (std::bad_cast) { };
					} catch (ibrcommon::Exception ex) {
						IBRCOMMON_LOGGER(error) << "Exception occurred in FloodRoutingExtension: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
					}
				} catch (std::exception) {
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

		FloodRoutingExtension::ProcessBundleTask::ProcessBundleTask(const dtn::data::MetaBundle &meta)
		 : bundle(meta)
		{ }

		FloodRoutingExtension::ProcessBundleTask::~ProcessBundleTask()
		{ }

		std::string FloodRoutingExtension::ProcessBundleTask::toString()
		{
			return "ProcessBundleTask: " + bundle.toString();
		}

		/****************************************/

		FloodRoutingExtension::ExpireTask::ExpireTask(const size_t t)
		 : timestamp(t)
		{ }

		FloodRoutingExtension::ExpireTask::~ExpireTask()
		{ }

		std::string FloodRoutingExtension::ExpireTask::toString()
		{
			return "ExpireTask";
		}
	}
}

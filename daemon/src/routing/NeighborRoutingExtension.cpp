/*
 * NeighborRoutingExtension.cpp
 *
 *  Created on: 16.02.2010
 *      Author: morgenro
 */

#include "routing/NeighborRoutingExtension.h"
#include "routing/QueueBundleEvent.h"
#include "core/TimeEvent.h"
#include "net/TransferCompletedEvent.h"
#include "net/TransferAbortedEvent.h"
#include "core/BundleExpiredEvent.h"
#include "core/NodeEvent.h"
#include "core/Node.h"
#include "net/ConnectionManager.h"
#include "ibrcommon/thread/MutexLock.h"
#include "core/SimpleBundleStorage.h"
#include <ibrcommon/Logger.h>
#include <ibrcommon/AutoDelete.h>

#include <functional>
#include <list>
#include <algorithm>
#include <typeinfo>

namespace dtn
{
	namespace routing
	{
		NeighborRoutingExtension::NeighborRoutingExtension()
		{
		}

		NeighborRoutingExtension::~NeighborRoutingExtension()
		{
			stop();
			join();
		}

		void NeighborRoutingExtension::stopExtension()
		{
			_taskqueue.abort();
		}

		bool NeighborRoutingExtension::__cancellation()
		{
			_taskqueue.abort();
			return true;
		}

		void NeighborRoutingExtension::run()
		{
			class BundleFilter : public dtn::core::BundleStorage::BundleFilterCallback
			{
			public:
				BundleFilter(const dtn::data::EID &destination)
				 : _destination(destination)
				{};

				virtual ~BundleFilter() {};

				virtual size_t limit() const { return 10; };

				virtual bool shouldAdd(const dtn::data::MetaBundle &meta) const
				{
					if (_destination.getNodeEID() != meta.destination.getNodeEID())
					{
						return false;
					}

					return true;
				};

			private:
				const dtn::data::EID &_destination;
			};

			dtn::core::BundleStorage &storage = (**this).getStorage();

			while (true)
			{
				try {
					Task *t = _taskqueue.getnpop(true);
					ibrcommon::AutoDelete<Task> killer(t);

					IBRCOMMON_LOGGER_DEBUG(5) << "processing neighbor routing task " << t->toString() << IBRCOMMON_LOGGER_ENDL;

					/**
					 * SearchNextBundleTask triggers a search for a bundle to transfer
					 * to another host. This Task is generated by TransferCompleted, TransferAborted
					 * and node events.
					 */
					try {
						SearchNextBundleTask &task = dynamic_cast<SearchNextBundleTask&>(*t);

						try {
							// create a new bundle filter
							BundleFilter filter(task.eid);

							// query an unknown bundle from the storage, the list contains max. 10 items.
							const std::list<dtn::data::MetaBundle> list = storage.get(filter);

							IBRCOMMON_LOGGER_DEBUG(5) << "got " << list.size() << " items to transfer to " << task.eid.getString() << IBRCOMMON_LOGGER_ENDL;

							// send the bundles as long as we have resources
							for (std::list<dtn::data::MetaBundle>::const_iterator iter = list.begin(); iter != list.end(); iter++)
							{
								try {
									// transfer the bundle to the neighbor
									transferTo(task.eid, *iter);
								} catch (const NeighborDatabase::AlreadyInTransitException&) { };
							}
						} catch (const NeighborDatabase::NoMoreTransfersAvailable&) { };
					} catch (const NeighborDatabase::NeighborNotAvailableException&) {
					} catch (std::bad_cast) { };

					/**
					 * process a received bundle
					 */
					try {
						dynamic_cast<ProcessBundleTask&>(*t);

						// new bundles trigger a recheck for all neighbors
						const std::set<dtn::core::Node> nl = dtn::core::BundleCore::getInstance().getNeighbors();

						for (std::set<dtn::core::Node>::const_iterator iter = nl.begin(); iter != nl.end(); iter++)
						{
							const dtn::core::Node &n = (*iter);

							// transfer the next bundle to this destination
							_taskqueue.push( new SearchNextBundleTask( n.getEID() ) );
						}
					} catch (std::bad_cast) { };

				} catch (const std::exception &ex) {
					IBRCOMMON_LOGGER_DEBUG(20) << "neighbor routing failed: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
					return;
				}

				yield();
			}
		}

		void NeighborRoutingExtension::notify(const dtn::core::Event *evt)
		{
			try {
				const QueueBundleEvent &queued = dynamic_cast<const QueueBundleEvent&>(*evt);
				_taskqueue.push( new ProcessBundleTask(queued.bundle, queued.origin) );
				return;
			} catch (std::bad_cast ex) { };

			try {
				const dtn::net::TransferCompletedEvent &completed = dynamic_cast<const dtn::net::TransferCompletedEvent&>(*evt);
				const dtn::data::MetaBundle &meta = completed.getBundle();
				const dtn::data::EID &peer = completed.getPeer();

				if ((meta.destination.getNodeEID() == peer.getNodeEID())
						&& (meta.procflags & dtn::data::Bundle::DESTINATION_IS_SINGLETON))
				{
					try {
						dtn::core::BundleStorage &storage = (**this).getStorage();

						// bundle has been delivered to its destination
						// delete it from our storage
						storage.remove(meta);

						IBRCOMMON_LOGGER_DEBUG(15) << "singleton bundle delivered: " << meta.toString() << IBRCOMMON_LOGGER_ENDL;
					} catch (const dtn::core::BundleStorage::NoBundleFoundException&) { };

					// transfer the next bundle to this destination
					_taskqueue.push( new SearchNextBundleTask( peer ) );
				}
				return;
			} catch (std::bad_cast ex) { };

			try {
				const dtn::net::TransferAbortedEvent &aborted = dynamic_cast<const dtn::net::TransferAbortedEvent&>(*evt);
				const dtn::data::EID &peer = aborted.getPeer();
				const dtn::data::BundleID &id = aborted.getBundleID();
				const dtn::data::MetaBundle meta = (**this).getStorage().get(id);

				switch (aborted.reason)
				{
					case dtn::net::TransferAbortedEvent::REASON_UNDEFINED:
						break;

					case dtn::net::TransferAbortedEvent::REASON_RETRY_LIMIT_REACHED:
						break;

					case dtn::net::TransferAbortedEvent::REASON_BUNDLE_DELETED:
						break;

					case dtn::net::TransferAbortedEvent::REASON_CONNECTION_DOWN:
						return;

					case dtn::net::TransferAbortedEvent::REASON_REFUSED:
					{
						if ((meta.destination.getNodeEID() == peer.getNodeEID())
								&& (meta.procflags & dtn::data::Bundle::DESTINATION_IS_SINGLETON))
						{
							// bundle is not deliverable
							try {
								// if the bundle has been sent by this module
								(**this).getStorage().remove(id);
							} catch (const dtn::core::BundleStorage::NoBundleFoundException&) { };
						}
					}
					break;
				}

				// transfer the next bundle to this destination
				_taskqueue.push( new SearchNextBundleTask( peer ) );

				return;
			} catch (std::bad_cast ex) { };

			// If a new neighbor comes available, send him a request for the summary vector
			// If a neighbor went away we can free the stored summary vector
			try {
				const dtn::core::NodeEvent &nodeevent = dynamic_cast<const dtn::core::NodeEvent&>(*evt);
				const dtn::core::Node &n = nodeevent.getNode();

				if (nodeevent.getAction() == NODE_AVAILABLE)
				{
					_taskqueue.push( new SearchNextBundleTask( n.getEID() ) );
				}

				return;
			} catch (std::bad_cast ex) { };
		}

		/****************************************/

		NeighborRoutingExtension::SearchNextBundleTask::SearchNextBundleTask(const dtn::data::EID &e)
		 : eid(e)
		{ }

		NeighborRoutingExtension::SearchNextBundleTask::~SearchNextBundleTask()
		{ }

		std::string NeighborRoutingExtension::SearchNextBundleTask::toString()
		{
			return "SearchNextBundleTask: " + eid.getString();
		}

		/****************************************/

		NeighborRoutingExtension::ProcessBundleTask::ProcessBundleTask(const dtn::data::MetaBundle &meta, const dtn::data::EID &o)
		 : bundle(meta), origin(o)
		{ }

		NeighborRoutingExtension::ProcessBundleTask::~ProcessBundleTask()
		{ }

		std::string NeighborRoutingExtension::ProcessBundleTask::toString()
		{
			return "ProcessBundleTask: " + bundle.toString();
		}
	}
}

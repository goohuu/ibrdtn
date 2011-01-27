/*
 * FloodRoutingExtension.cpp
 *
 *  Created on: 18.02.2010
 *      Author: morgenro
 */

#include "routing/flooding/FloodRoutingExtension.h"
#include "routing/QueueBundleEvent.h"
#include "core/NodeEvent.h"
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
#include <limits>

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
				_taskqueue.push( new ProcessBundleTask(queued.bundle) );
				return;
			} catch (std::bad_cast ex) { };

			try {
				const dtn::core::NodeEvent &nodeevent = dynamic_cast<const dtn::core::NodeEvent&>(*evt);
				dtn::core::Node n = nodeevent.getNode();

				if (nodeevent.getAction() == NODE_AVAILABLE)
				{
					// send all (multi-hop) bundles in the storage to the neighbor
					_taskqueue.push( new TransferAllBundlesTask(nodeevent.getNode().getEID()) );
				}
				return;
			} catch (std::bad_cast ex) { };
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
				BundleFilter()
				{};

				virtual ~BundleFilter() {};

				// unlimited bundles
				virtual size_t limit() const { return std::numeric_limits<std::size_t>::max(); };

				virtual bool shouldAdd(const dtn::data::MetaBundle &meta) const
				{
					const dtn::data::EID dest = meta.destination.getNodeEID();

					// do not forward to any blacklisted destination
					if (_blacklist.find(dest) != _blacklist.end())
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
							TransferAllBundlesTask &task = dynamic_cast<TransferAllBundlesTask&>(*t);

							// get the bundle filter of the neighbor
							BundleFilter filter;

							// blacklist the neighbor itself, because this is handles by neighbor routing extension
							filter.blacklist(task.eid);

							// some debug
							IBRCOMMON_LOGGER_DEBUG(40) << "transfer all bundles to " << task.eid.getString() << IBRCOMMON_LOGGER_ENDL;

							// query all bundles from the storage
							const std::list<dtn::data::MetaBundle> list = storage.get(filter);

							// send the bundles as long as we have resources
							for (std::list<dtn::data::MetaBundle>::const_iterator iter = list.begin(); iter != list.end(); iter++)
							{
								// transfer the bundle to the neighbor
								transferTo(task.eid, *iter);
							}
						} catch (std::bad_cast) { };

						try {
							ProcessBundleTask &task = dynamic_cast<ProcessBundleTask&>(*t);
							const dtn::data::Bundle b = storage.get(task.bundle);

							// new bundles are forwarded to all neighbors
							const std::set<dtn::core::Node> nl = dtn::core::BundleCore::getInstance().getNeighbors();

							for (std::set<dtn::core::Node>::const_iterator iter = nl.begin(); iter != nl.end(); iter++)
							{
								const dtn::core::Node &n = (*iter);

								if (EID(b._destination.getNodeEID()) != n.getEID())
								{
									// transfer the bundle to the next neighbor
									transferTo(n.getEID(), task.bundle);
								}
							}
						} catch (const dtn::core::BundleStorage::NoBundleFoundException&) {
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

		FloodRoutingExtension::TransferAllBundlesTask::TransferAllBundlesTask(const dtn::data::EID &e)
		 : eid(e)
		{ }

		FloodRoutingExtension::TransferAllBundlesTask::~TransferAllBundlesTask()
		{ }

		std::string FloodRoutingExtension::TransferAllBundlesTask::toString()
		{
			return "TransferAllBundlesTask: " + eid.getString();
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
	}
}

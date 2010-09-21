/*
 * EpidemicRoutingExtension.h
 *
 *  Created on: 18.02.2010
 *      Author: morgenro
 */

#ifndef FLOODROUTINGEXTENSION_H_
#define FLOODROUTINGEXTENSION_H_

#include "core/Node.h"

#include "routing/SummaryVector.h"
#include "routing/BaseRouter.h"
#include "routing/NeighborDatabase.h"

#include <ibrdtn/data/Block.h>
#include <ibrdtn/data/SDNV.h>
#include <ibrdtn/data/BundleString.h>
#include <ibrdtn/data/BundleList.h>

#include <ibrcommon/thread/ThreadSafeQueue.h>

#include <list>
#include <queue>

namespace dtn
{
	namespace routing
	{
		class FloodRoutingExtension : public BaseRouter::ThreadedExtension
		{
		public:
			FloodRoutingExtension();
			~FloodRoutingExtension();

			void notify(const dtn::core::Event *evt);

			virtual void stopExtension();

		protected:
			void run();

			/**
			 * Check if one bundle was seen before.
			 * @param id The ID of the Bundle.
			 * @return True, if the bundle was seen before. False if not.
			 */
			bool wasSeenBefore(const dtn::data::BundleID &id) const;

//			/**
//			 * remove a bundle out of all local bundle lists
//			 * @param id The ID of the Bundle.
//			 */
//			void remove(const dtn::data::MetaBundle &meta);

		private:
			class Task
			{
			public:
				virtual ~Task() {};
				virtual std::string toString() = 0;
			};

			class ExpireTask : public Task
			{
			public:
				ExpireTask(const size_t timestamp);
				virtual ~ExpireTask();

				virtual std::string toString();

				const size_t timestamp;
			};

			class SearchNextBundleTask : public Task
			{
			public:
				SearchNextBundleTask(const dtn::data::EID &eid);
				virtual ~SearchNextBundleTask();

				virtual std::string toString();

				const dtn::data::EID eid;
			};

			class ProcessBundleTask : public Task
			{
			public:
				ProcessBundleTask(const dtn::data::MetaBundle &meta);
				virtual ~ProcessBundleTask();

				virtual std::string toString();

				const dtn::data::MetaBundle bundle;
			};

			/**
			 * contains a lock for bundles lists (_bundles, _seenlist)
			 */
			ibrcommon::Mutex _list_mutex;

			/**
			 * contains a list of bundle references which has been seen previously
			 */
			dtn::data::BundleList _seenlist;

			/**
			 * contains the own summary vector for all stored bundles
			 */
			dtn::routing::BundleSummary _purge_vector;

			/**
			 * stores information about neighbors
			 */
			dtn::routing::NeighborDatabase _neighbors;

			/**
			 * hold queued tasks for later processing
			 */
			ibrcommon::ThreadSafeQueue<FloodRoutingExtension::Task* > _taskqueue;
		};
	}
}

#endif /* EPIDEMICROUTINGEXTENSION_H_ */

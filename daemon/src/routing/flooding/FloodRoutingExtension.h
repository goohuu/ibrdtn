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

#include <ibrcommon/thread/Queue.h>

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
			virtual ~FloodRoutingExtension();

			void notify(const dtn::core::Event *evt);

			virtual void stopExtension();

		protected:
			void run();
			bool __cancellation();

		private:
			class Task
			{
			public:
				virtual ~Task() {};
				virtual std::string toString() = 0;
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
				ProcessBundleTask(const dtn::data::MetaBundle &meta, const dtn::data::EID &origin);
				virtual ~ProcessBundleTask();

				virtual std::string toString();

				const dtn::data::MetaBundle bundle;
				const dtn::data::EID origin;
			};

			class TransferCompletedTask : public Task
			{
			public:
				TransferCompletedTask(const dtn::data::EID &eid, const dtn::data::MetaBundle &meta);
				virtual ~TransferCompletedTask();

				virtual std::string toString();

				const dtn::data::EID peer;
				const dtn::data::MetaBundle meta;
			};

			/**
			 * Add a bundle id to the summary vector of a neighbor.
			 * @param neighbor
			 * @param b
			 */
			void addToSummaryVector(const dtn::data::EID &neighbor, const dtn::data::BundleID &b);
			void addToSummaryVector(NeighborDatabase &db, const dtn::data::EID &neighbor, const dtn::data::BundleID &b);

			/**
			 * hold queued tasks for later processing
			 */
			ibrcommon::Queue<FloodRoutingExtension::Task* > _taskqueue;
		};
	}
}

#endif /* EPIDEMICROUTINGEXTENSION_H_ */

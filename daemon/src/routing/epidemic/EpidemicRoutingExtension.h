/*
 * EpidemicRoutingExtension.h
 *
 *  Created on: 18.02.2010
 *      Author: morgenro
 */

#ifndef EPIDEMICROUTINGEXTENSION_H_
#define EPIDEMICROUTINGEXTENSION_H_

#include "core/Node.h"
#include "core/AbstractWorker.h"

#include "routing/SummaryVector.h"
#include "routing/BaseRouter.h"
#include "routing/NeighborDatabase.h"

#include <ibrdtn/data/Block.h>
#include <ibrdtn/data/SDNV.h>
#include <ibrdtn/data/BundleString.h>
#include <ibrdtn/data/ExtensionBlock.h>

#include <ibrcommon/thread/Queue.h>

#include <list>
#include <queue>

namespace dtn
{
	namespace routing
	{
		class EpidemicRoutingExtension : public BaseRouter::ThreadedExtension
		{
		public:
			EpidemicRoutingExtension();
			virtual ~EpidemicRoutingExtension();

			void notify(const dtn::core::Event *evt);
			virtual void stopExtension();

			void requestHandshake(NodeHandshake &request) const;
//			void responseHandshake(const NodeHandshake &request, NodeHandshake &answer) const;

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

			class ExecutableTask : public Task
			{
			public:
				virtual ~ExecutableTask() {};
				virtual std::string toString() = 0;
				virtual void execute() const = 0;
			};

			class SearchNextBundleTask : public Task
			{
			public:
				SearchNextBundleTask(const dtn::data::EID &eid);
				virtual ~SearchNextBundleTask();

				virtual std::string toString();

				const dtn::data::EID eid;
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
			 * hold queued tasks for later processing
			 */
			ibrcommon::Queue<EpidemicRoutingExtension::Task* > _taskqueue;
		};
	}
}

#endif /* EPIDEMICROUTINGEXTENSION_H_ */

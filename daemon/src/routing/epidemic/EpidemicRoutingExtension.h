/*
 * EpidemicRoutingExtension.h
 *
 *  Created on: 18.02.2010
 *      Author: morgenro
 */

#ifndef EPIDEMICROUTINGEXTENSION_H_
#define EPIDEMICROUTINGEXTENSION_H_

#include "core/Node.h"

#include "routing/SummaryVector.h"
#include "routing/BaseRouter.h"
#include "routing/NeighborDatabase.h"

#include <ibrdtn/data/Block.h>
#include <ibrdtn/data/SDNV.h>
#include <ibrdtn/data/BundleString.h>
#include <ibrdtn/data/BundleList.h>
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
				ProcessBundleTask(const dtn::data::MetaBundle &meta, const dtn::data::EID &origin);
				virtual ~ProcessBundleTask();

				virtual std::string toString();

				const dtn::data::MetaBundle bundle;
				const dtn::data::EID origin;
			};

			class QuerySummaryVectorTask : public ExecutableTask
			{
			public:
				QuerySummaryVectorTask(const dtn::data::EID &origin);
				virtual ~QuerySummaryVectorTask();

				virtual std::string toString();
				virtual void execute() const;

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
			 * Process an ECM bundle. ECM are requests for or contain summary vectors used
			 * for epidemic routing
			 * @param origin The ECM is received from this EID.
			 * @param b The bundle containing the ECM.
			 */
			void processECM(const dtn::data::EID &origin, const dtn::data::Bundle &b);

			/**
			 * contains the own summary vector for all delivered bundles
			 */
			dtn::routing::BundleSummary _purge_vector;

			/**
			 * stores information about neighbors
			 */
			dtn::routing::NeighborDatabase _neighbors;

			/**
			 * hold queued tasks for later processing
			 */
			ibrcommon::Queue<EpidemicRoutingExtension::Task* > _taskqueue;
		};
	}
}

#endif /* EPIDEMICROUTINGEXTENSION_H_ */

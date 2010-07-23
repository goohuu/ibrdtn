/*
 * EpidemicRoutingExtension.h
 *
 *  Created on: 18.02.2010
 *      Author: morgenro
 */

#ifndef EPIDEMICROUTINGEXTENSION_H_
#define EPIDEMICROUTINGEXTENSION_H_

#include "core/Node.h"

#include "net/DiscoveryServiceProvider.h"
#include "routing/SummaryVector.h"
#include "routing/BaseRouter.h"
#include "routing/NeighborDatabase.h"

#include <ibrdtn/data/Block.h>
#include <ibrdtn/data/SDNV.h>
#include <ibrdtn/data/BundleString.h>
#include <ibrdtn/data/BundleList.h>
#include <ibrdtn/data/ExtensionBlockFactory.h>

#include <ibrcommon/thread/ThreadSafeQueue.h>

#include <list>
#include <queue>

namespace dtn
{
	namespace routing
	{
		class EpidemicRoutingExtension : public BaseRouter::ThreadedExtension, public dtn::net::DiscoveryServiceProvider
		{
		public:
			EpidemicRoutingExtension();
			~EpidemicRoutingExtension();

			void notify(const dtn::core::Event *evt);

			void update(std::string &name, std::string &data);

			virtual void stopExtension();

			class EpidemicExtensionBlock : public dtn::data::Block
			{
			public:
				class Factory : public dtn::data::ExtensionBlockFactory
				{
				public:
					Factory() : dtn::data::ExtensionBlockFactory(EpidemicExtensionBlock::BLOCK_TYPE) {};
					virtual ~Factory() {};
					virtual dtn::data::Block* create();
				};

				static const char BLOCK_TYPE = 201;

				EpidemicExtensionBlock();
				~EpidemicExtensionBlock();

				void set(dtn::data::SDNV value);
				dtn::data::SDNV get() const;

				void setSummaryVector(const SummaryVector &vector);
				const SummaryVector& getSummaryVector() const;

				virtual size_t getLength() const;
				virtual std::ostream &serialize(std::ostream &stream) const;
				virtual std::istream &deserialize(std::istream &stream);

			private:
				dtn::data::SDNV _counter;
				dtn::data::BundleString _data;
				SummaryVector _vector;
			};

		protected:
			void run();

			/**
			 * Check if one bundle was seen before.
			 * @param id The ID of the Bundle.
			 * @return True, if the bundle was seen before. False if not.
			 */
			bool wasSeenBefore(const dtn::data::BundleID &id) const;

			/**
			 * remove a bundle out of all local bundle lists
			 * @param id The ID of the Bundle.
			 */
			void remove(const dtn::data::MetaBundle &meta);

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

			class BroadcastSummaryVectorTask : public Task
			{
			public:
				BroadcastSummaryVectorTask();
				virtual ~BroadcastSummaryVectorTask();

				virtual std::string toString();
			};

			class UpdateSummaryVectorTask : public Task
			{
			public:
				UpdateSummaryVectorTask(const dtn::data::EID &eid);
				virtual ~UpdateSummaryVectorTask();

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

			void transferEpidemicInformation(const dtn::data::EID &eid);

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
			SummaryVector _bundle_vector;

			/**
			 * stores information about neighbors
			 */
			dtn::routing::NeighborDatabase _neighbors;

			/**
			 * hold queued tasks for later processing
			 */
			ibrcommon::ThreadSafeQueue<EpidemicRoutingExtension::Task* > _taskqueue;
		};

		/**
		 * This creates a static bundle factory
		 */
		static EpidemicRoutingExtension::EpidemicExtensionBlock::Factory __EpidemicExtensionFactory__;
	}
}

#endif /* EPIDEMICROUTINGEXTENSION_H_ */

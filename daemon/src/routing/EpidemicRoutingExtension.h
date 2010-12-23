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
#include <ibrdtn/data/ExtensionBlock.h>

#include <ibrcommon/thread/Queue.h>

#include <list>
#include <queue>

namespace dtn
{
	namespace routing
	{
		class EpidemicRoutingExtension : public BaseRouter::ThreadedExtension, public dtn::net::DiscoveryServiceProvider
		{
		private:
			static const dtn::data::EID EPIDEMIC_ROUTING_ADDRESS;

		public:
			EpidemicRoutingExtension();
			virtual ~EpidemicRoutingExtension();

			void notify(const dtn::core::Event *evt);

			/**
			 * this method updates the given values
			 */
			void update(const ibrcommon::vinterface &iface, std::string &name, std::string &data) throw(dtn::net::DiscoveryServiceProvider::NoServiceHereException);

			virtual void stopExtension();

			class EpidemicExtensionBlock : public dtn::data::Block
			{
			public:
				class Factory : public dtn::data::ExtensionBlock::Factory
				{
				public:
					Factory() : dtn::data::ExtensionBlock::Factory(EpidemicExtensionBlock::BLOCK_TYPE) {};
					virtual ~Factory() {};
					virtual dtn::data::Block* create();
				};

				static const char BLOCK_TYPE = 201;

				EpidemicExtensionBlock();
				virtual ~EpidemicExtensionBlock();

				void set(dtn::data::SDNV value);
				dtn::data::SDNV get() const;

				void setPurgeVector(const SummaryVector &vector);
				void setSummaryVector(const SummaryVector &vector);
				const SummaryVector& getSummaryVector() const;
				const SummaryVector& getPurgeVector() const;

				virtual size_t getLength() const;
				virtual std::ostream &serialize(std::ostream &stream) const;
				virtual std::istream &deserialize(std::istream &stream);

			private:
				dtn::data::SDNV _counter;
				dtn::data::BundleString _data;
				SummaryVector _vector;
				SummaryVector _purge;
			};

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

			class TransferSummaryVectorTask : public Task
			{
			public:
				TransferSummaryVectorTask(const dtn::data::EID &eid);
				virtual ~TransferSummaryVectorTask();

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

			/**
			 * Prepare an epidemic bundle. This includes setting all parameters
			 * like lifetime, source and destination. Additional the bundle is added
			 * to the list of known bundles and stored in the bundle storage.
			 * @param b The bundle sample.
			 */
			void prepareEpidemicInfo(dtn::data::Bundle &b);

			/**
			 * contains a lock for bundles lists (_bundles, _seenlist)
			 */
			ibrcommon::Mutex _list_mutex;

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

			/**
			 * The current epidemic bundle is stored in this variable.
			 */
			dtn::data::Bundle _epidemic_bundle;
		};

		/**
		 * This creates a static bundle factory
		 */
		static EpidemicRoutingExtension::EpidemicExtensionBlock::Factory __EpidemicExtensionFactory__;
	}
}

#endif /* EPIDEMICROUTINGEXTENSION_H_ */

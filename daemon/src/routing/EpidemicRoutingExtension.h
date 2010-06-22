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

#include <ibrdtn/data/Block.h>
#include <ibrdtn/data/SDNV.h>
#include <ibrdtn/data/BundleString.h>
#include <ibrdtn/data/BundleList.h>
#include <ibrdtn/data/ExtensionBlockFactory.h>

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
			 * transmit a bundle to all neighbors
			 * @param id
			 * @param filter
			 */
			void broadcast(const dtn::data::BundleID &id, bool filter = false);

			/**
			 * extract the bloomfilter of an epidemic routing extension block
			 * @param id
			 */
			void readExtensionBlock(const dtn::data::BundleID &id);

			/**
			 * remove a bundle out of all local bundle lists
			 * @param id The ID of the Bundle.
			 */
			void remove(const dtn::data::MetaBundle &meta);

			/**
			 * contains a lock for bundles lists (_bundles, _seenlist)
			 */
			ibrcommon::Mutex _list_mutex;

			/**
			 * contains all available neighbors
			 */
			std::set<dtn::data::EID> _neighbors;

			/**
			 * contains a list of all new neighbors
			 */
			std::queue<dtn::data::EID> _new_neighbors;

			/**
			 * contains a list of bundle references which has been seen previously
			 */
			dtn::data::BundleList _seenlist;

			/**
			 * contains a map of bloomfilters of other nodes
			 */
			std::map<dtn::data::EID, ibrcommon::BloomFilter> _filterlist;

			/**
			 * contains references of bundles to process
			 */
			std::queue<dtn::data::MetaBundle> _bundle_queue;

			/**
			 * contains the current timestamp or is set to zero
			 */
			size_t _timestamp;

			/**
			 * contains the own summary vector for all stored bundles
			 */
			SummaryVector _bundle_vector;

			/**
			 * contains references to all stored bundles
			 */
			dtn::data::BundleList _bundles;
		};

		/**
		 * This creates a static bundle factory
		 */
		static EpidemicRoutingExtension::EpidemicExtensionBlock::Factory __EpidemicExtensionFactory__;
	}
}

#endif /* EPIDEMICROUTINGEXTENSION_H_ */

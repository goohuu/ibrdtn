/*
 * EpidemicRoutingExtension.h
 *
 *  Created on: 18.02.2010
 *      Author: morgenro
 */

#ifndef EPIDEMICROUTINGEXTENSION_H_
#define EPIDEMICROUTINGEXTENSION_H_

#include "routing/BaseRouter.h"
#include "routing/BundleList.h"
#include "core/Node.h"
#include "ibrdtn/data/Block.h"
#include "ibrdtn/data/SDNV.h"
#include "ibrdtn/data/BundleString.h"
#include "net/DiscoveryServiceProvider.h"
#include "routing/SummaryVector.h"
#include <list>

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

		protected:
			void run();

		private:
			class EpidemicExtensionBlock : public dtn::data::Block
			{
			public:
				static const char BLOCK_TYPE = 201;

				EpidemicExtensionBlock(const SummaryVector &vector);
				EpidemicExtensionBlock(Block *block);
				~EpidemicExtensionBlock();

				void read();
				void commit();

				void set(dtn::data::SDNV value);
				dtn::data::SDNV get() const;

				const SummaryVector& getSummaryVector() const;

			private:
				dtn::data::SDNV _counter;
				dtn::data::BundleString _data;
				SummaryVector _vector;
			};

//			class BundleEIDList
//			{
//			private:
//				class ExpiringList
//				{
//				public:
//					ExpiringList(const MetaBundle b);
//
//					~ExpiringList();
//
//					bool operator!=(const ExpiringList& other) const;
//
//					bool operator==(const ExpiringList& other) const;
//
//					bool operator<(const ExpiringList& other) const;
//					bool operator>(const ExpiringList& other) const;
//					void add(const dtn::data::EID eid);
//
//					void remove(const dtn::data::EID eid);
//
//					bool contains(const dtn::data::EID eid) const;
//
//					const MetaBundle bundle;
//					const size_t expiretime;
//
//				private:
//					std::set<dtn::data::EID> _items;
//				};
//
//			public:
//				BundleEIDList();
//				~BundleEIDList();
//
//				void add(const dtn::routing::MetaBundle bundle, const dtn::data::EID eid);
//
//				void remove(const dtn::routing::MetaBundle bundle);
//
//				bool contains(const dtn::routing::MetaBundle bundle, const dtn::data::EID eid);
//
//				void expire(const size_t timestamp);
//
//			private:
//				std::set<ExpiringList> _bundles;
//			};

			/**
			 * Check if one bundle was seen before.
			 * @param id The ID of the Bundle.
			 * @return True, if the bundle was seen before. False if not.
			 */
			bool wasSeenBefore(const dtn::data::BundleID &id) const;

			void broadcast(const dtn::data::BundleID &id);

			void readExtensionBlock(const dtn::data::BundleID &id);

			std::list<dtn::core::Node> _neighbors;
			std::queue<dtn::data::EID> _available;

			ibrcommon::Mutex _list_mutex;
			dtn::routing::BundleList _seenlist;
			dtn::routing::BundleList _bundles;
			//BundleEIDList _forwarded;

			std::map<dtn::data::EID, ibrcommon::BloomFilter> _filterlist;

			std::queue<dtn::routing::MetaBundle> _out_queue;

			size_t _timestamp;

			SummaryVector _bundle_vector;
		};
	}
}

#endif /* EPIDEMICROUTINGEXTENSION_H_ */

/*
 * NeighborDatabase.h
 *
 *  Created on: 23.07.2010
 *      Author: morgenro
 */

#ifndef NEIGHBORDATABASE_H_
#define NEIGHBORDATABASE_H_

#include <ibrdtn/data/EID.h>
#include <ibrdtn/data/BundleID.h>
#include <ibrcommon/data/BloomFilter.h>
#include <ibrcommon/Exceptions.h>
#include <map>

namespace dtn
{
	namespace routing
	{
		/**
		 * The neighbor database contains collected information about neighbors.
		 * This includes the last timestamp on which a neighbor was seen, the bundles
		 * this neighbors has received (bloomfilter with age).
		 */
		class NeighborDatabase
		{
		public:
			class BloomfilterNotAvailableException : public ibrcommon::Exception
			{
			public:
				BloomfilterNotAvailableException(const dtn::data::EID &host)
				: ibrcommon::Exception("Bloom filter is not available for this node."), eid(host) { };

				virtual ~BloomfilterNotAvailableException() throw () { };

				const dtn::data::EID eid;
			};

			class NeighborEntry
			{
			public:
				NeighborEntry();
				NeighborEntry(const dtn::data::EID &eid);
				~NeighborEntry();

				void updateLastSeen();
				void updateBundles(const ibrcommon::BloomFilter &bf);

				dtn::data::EID _eid;
				ibrcommon::BloomFilter _filter;
				size_t _filter_age;
				size_t _lastseen;
				bool _available;
			};

			NeighborDatabase();
			virtual ~NeighborDatabase();

			void updateLastSeen(const dtn::data::EID &eid);
			void updateBundles(const dtn::data::EID &eid, const ibrcommon::BloomFilter &bf);

			/**
			 * Use the bloom filter of a known neighbor to determine if a specific bundle
			 * is known by the neighbor and should not transferred to it.
			 * @param eid
			 * @param bundle
			 * @return True, if the bundle is known by the neighbor.
			 */
			bool knowBundle(const dtn::data::EID &eid, const dtn::data::BundleID &bundle) throw (BloomfilterNotAvailableException);

			void setAvailable(const dtn::data::EID &eid);
			void setUnavailable(const dtn::data::EID &eid);

			std::set<dtn::data::EID> getAvailable() const;

			NeighborDatabase::NeighborEntry& get(const dtn::data::EID &eid);

		private:
			std::map<dtn::data::EID, NeighborDatabase::NeighborEntry> _entries;
		};
	}
}

#endif /* NEIGHBORDATABASE_H_ */
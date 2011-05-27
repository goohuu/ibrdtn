/*
 * NeighborDatabase.h
 *
 *  Created on: 23.07.2010
 *      Author: morgenro
 */

#ifndef NEIGHBORDATABASE_H_
#define NEIGHBORDATABASE_H_

#include "routing/BundleSummary.h"

#include <ibrdtn/data/EID.h>
#include <ibrdtn/data/BundleID.h>
#include <ibrcommon/data/BloomFilter.h>
#include <ibrcommon/Exceptions.h>
#include <ibrcommon/thread/ThreadsafeState.h>
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
		class NeighborDatabase : public ibrcommon::Mutex
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

			class NoMoreTransfersAvailable : public ibrcommon::Exception
			{
			public:
				NoMoreTransfersAvailable() : ibrcommon::Exception("No more transfers allowed.") { };
				virtual ~NoMoreTransfersAvailable() throw () { };
			};

			class AlreadyInTransitException : public ibrcommon::Exception
			{
			public:
				AlreadyInTransitException() : ibrcommon::Exception("This bundle is already in transit.") { };
				virtual ~AlreadyInTransitException() throw () { };
			};

			class NeighborNotAvailableException : public ibrcommon::Exception
			{
			public:
				NeighborNotAvailableException() : ibrcommon::Exception("Entry for this neighbor not found.") { };
				virtual ~NeighborNotAvailableException() throw () { };
			};

			class NeighborEntry
			{
			public:
				NeighborEntry();
				NeighborEntry(const dtn::data::EID &eid);
				virtual ~NeighborEntry();

				/**
				 * updates the bloomfilter of this entry with a new one
				 * @param bf The bloomfilter object
				 * @param lifetime The desired lifetime of this bloomfilter
				 */
				void update(const ibrcommon::BloomFilter &bf, const size_t lifetime = 0);

				void reset();

				void add(const dtn::data::MetaBundle&);

				bool has(const dtn::data::BundleID&, const bool require_bloomfilter = false) const;

				/**
				 * acquire resource to send a filter request.
				 * The resources are reset once if the filter expires.
				 */
				void acquireFilterRequest() throw (NoMoreTransfersAvailable);

				/**
				 * Acquire transfer resources. If no resources is left,
				 * an exception is thrown.
				 */
				void acquireTransfer(const dtn::data::BundleID &id) throw (NoMoreTransfersAvailable, AlreadyInTransitException);

				/**
				 * Release a transfer resource, but never exceed the maxium
				 * resource limit.
				 */
				void releaseTransfer(const dtn::data::BundleID &id);

				// the EID of the corresponding node
				const dtn::data::EID eid;

				/**
				 * trigger expire mechanisms for bloomfilter and bundle summary
				 * @param timestamp
				 */
				void expire(const size_t timestamp);

			private:
				// stores bundle currently in transit
				ibrcommon::Mutex _transit_lock;
				std::set<dtn::data::BundleID> _transit_bundles;
				size_t _transit_max;

				// bloomfilter used as summary vector
				ibrcommon::BloomFilter _filter;
				BundleSummary _summary;
				size_t _filter_expire;

				enum FILTER_REQUEST_STATE
				{
					FILTER_AWAITING = 0,
					FILTER_AVAILABLE = 1,
					FILTER_EXPIRED = 2,
					FILTER_FINAL = 3
				};

				ibrcommon::ThreadsafeState<FILTER_REQUEST_STATE> _filter_state;
			};

			NeighborDatabase();
			virtual ~NeighborDatabase();

			/**
			 * Query a neighbor entry of the database. It throws an exception
			 * if the neighbor is not available.
			 * @param eid The EID of the neighbor
			 * @return The neighbor entry reference.
			 */
			NeighborDatabase::NeighborEntry& get(const dtn::data::EID &eid) throw (NeighborNotAvailableException);

			/**
			 * Query a neighbor entry of the database. If the entry does not
			 * exists, a new entry is created and returned.
			 * @param eid The EID of the neighbor.
			 * @return The neighbor entry reference.
			 */
			NeighborDatabase::NeighborEntry& create(const dtn::data::EID &eid);

			/**
			 * reset bloom filter of this neighbor
			 * @param eid
			 * @return
			 */
			NeighborDatabase::NeighborEntry& reset(const dtn::data::EID &eid);

			/**
			 * Remove an entry of the database.
			 * @param eid The EID of the neighbor to remove.
			 */
			void remove(const dtn::data::EID &eid);

			/**
			 * Add a bundle id to the bloomfilter of a neighbor
			 */
			void addBundle(const dtn::data::EID &neighbor, const dtn::data::MetaBundle &b);

			/**
			 * trigger expire mechanisms for bloomfilter and bundle summary
			 * @param timestamp
			 */
			void expire(const size_t timestamp);

		private:
			std::map<dtn::data::EID, NeighborDatabase::NeighborEntry* > _entries;
		};
	}
}

#endif /* NEIGHBORDATABASE_H_ */

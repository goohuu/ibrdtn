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
				void updateBundles(const ibrcommon::BloomFilter &bf, const size_t lifetime = 0);

				/**
				 * Get a reference to the summary vector.
				 * @return The summary vector as bloomfilter.
				 */
				ibrcommon::BloomFilter& getBundles() throw (BloomfilterNotAvailableException);

				/**
				 * acquire resource to send a filter request.
				 * The resources are reset once if the filter expires.
				 */
				void acquireFilterRequest() throw (NoMoreTransfersAvailable);

				/**
				 * Acquire transfer resources. If no resources is left,
				 * an exception is thrown.
				 */
				void acquireTransfer() throw (NoMoreTransfersAvailable);

				/**
				 * Release a transfer resource, but never exceed the maxium
				 * resource limit.
				 */
				void releaseTransfer();

			private:
				// the EID of the corresponding node
				const dtn::data::EID _eid;

				// a triple of lock, counter and max value for transfer resources
				ibrcommon::Mutex _transfer_lock;
				size_t _transfer_semaphore;
				size_t _transfer_max;

				// bloomfilter used as summary vector
				ibrcommon::BloomFilter _filter;
				size_t _filter_expire;

				enum FILTER_REQUEST_STATE
				{
					FILTER_AWAITING = 0,
					FILTER_AVAILABLE = 1,
					FILTER_EXPIRED = 2
				} _filter_state;
			};

			NeighborDatabase();
			virtual ~NeighborDatabase();

			/**
			 * updates the bloomfilter of this entry with a new one
			 * @param eid The EID of the ndoe
			 * @param bf The bloomfilter object
			 * @param lifetime The desired lifetime of this bloomfilter
			 */
			void updateBundles(const dtn::data::EID &eid, const ibrcommon::BloomFilter &bf, const size_t lifetime = 0);

			/**
			 * Use the bloom filter of a known neighbor to determine if a specific bundle
			 * is known by the neighbor and should not transferred to it.
			 * @param eid
			 * @param bundle
			 * @return True, if the bundle is known by the neighbor.
			 */
			bool knowBundle(const dtn::data::EID &eid, const dtn::data::BundleID &bundle) throw (BloomfilterNotAvailableException);

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
			 * Remove an entry of the database.
			 * @param eid The EID of the neighbor to remove.
			 */
			void remove(const dtn::data::EID &eid);

		private:
			std::map<dtn::data::EID, NeighborDatabase::NeighborEntry* > _entries;
		};
	}
}

#endif /* NEIGHBORDATABASE_H_ */

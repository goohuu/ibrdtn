/*
 * NeighborDatabase.cpp
 *
 *  Created on: 23.07.2010
 *      Author: morgenro
 */

#include "routing/NeighborDatabase.h"
#include <ibrdtn/utils/Clock.h>
#include <ibrcommon/Logger.h>

namespace dtn
{
	namespace routing
	{
		NeighborDatabase::NeighborEntry::NeighborEntry()
		 : _eid(), _transit_max(5), _filter(), _filter_expire(0), _filter_state(FILTER_EXPIRED)
		{};

		NeighborDatabase::NeighborEntry::NeighborEntry(const dtn::data::EID &eid)
		 : _eid(eid), _transit_max(5), _filter(), _filter_expire(0), _filter_state(FILTER_EXPIRED)
		{ }

		NeighborDatabase::NeighborEntry::~NeighborEntry()
		{ }

		void NeighborDatabase::NeighborEntry::updateBundles(const ibrcommon::BloomFilter &bf, const size_t lifetime)
		{
			_filter = bf;

			if (lifetime == 0)
			{
				_filter_expire = 0;
			}
			else
			{
				_filter_expire = dtn::utils::Clock::getExpireTime(lifetime);
			}

			_filter_state = FILTER_AVAILABLE;
		}

		ibrcommon::BloomFilter& NeighborDatabase::NeighborEntry::getBundles() throw (BloomfilterNotAvailableException)
		{
			if (_filter_state != FILTER_AVAILABLE)
				throw BloomfilterNotAvailableException(_eid);

			if ((_filter_expire > 0) && (_filter_expire < dtn::utils::Clock::getTime()))
			{
				IBRCOMMON_LOGGER_DEBUG(15) << "summary vector of " << _eid.getString() << " is expired" << IBRCOMMON_LOGGER_ENDL;

				// set the filter state to expired once
				_filter_state = FILTER_EXPIRED;

				throw BloomfilterNotAvailableException(_eid);
			}

			return _filter;
		}

		void NeighborDatabase::NeighborEntry::acquireFilterRequest() throw (NoMoreTransfersAvailable)
		{
			if (_filter_state != FILTER_EXPIRED)
				throw NoMoreTransfersAvailable();

			_filter_state = FILTER_AWAITING;
		}

		void NeighborDatabase::NeighborEntry::acquireTransfer(const dtn::data::BundleID &id) throw (NoMoreTransfersAvailable, AlreadyInTransitException)
		{
			ibrcommon::MutexLock l(_transit_lock);

			// check if the bundle is already in transit
			if (_transit_bundles.find(id) != _transit_bundles.end()) throw AlreadyInTransitException();

			// check if enough resources available to transfer the bundle
			if (_transit_bundles.size() >= _transit_max) throw NoMoreTransfersAvailable();

			IBRCOMMON_LOGGER_DEBUG(20) << "acquire transfer of " << id.toString() << " (" << _transit_bundles.size() << " bundles in transit)" << IBRCOMMON_LOGGER_ENDL;
		}

		void NeighborDatabase::NeighborEntry::releaseTransfer(const dtn::data::BundleID &id)
		{
			ibrcommon::MutexLock l(_transit_lock);
			_transit_bundles.erase(id);

			IBRCOMMON_LOGGER_DEBUG(20) << "release transfer of " << id.toString() << " (" << _transit_bundles.size() << " bundles in transit)" << IBRCOMMON_LOGGER_ENDL;
		}

		NeighborDatabase::NeighborDatabase()
		{
		}

		NeighborDatabase::~NeighborDatabase()
		{
			std::set<dtn::data::EID> ret;

			for (std::map<dtn::data::EID, NeighborDatabase::NeighborEntry* >::const_iterator iter = _entries.begin(); iter != _entries.end(); iter++)
			{
				delete (*iter).second;
			}
		}

		NeighborDatabase::NeighborEntry& NeighborDatabase::create(const dtn::data::EID &eid)
		{
			if (_entries.find(eid) == _entries.end())
			{
				NeighborEntry *entry = new NeighborEntry(eid);
				_entries[eid] = entry;
			}

			return (*_entries[eid]);
		}

		NeighborDatabase::NeighborEntry& NeighborDatabase::get(const dtn::data::EID &eid) throw (NeighborNotAvailableException)
		{
			if (_entries.find(eid) == _entries.end())
			{
				throw NeighborNotAvailableException();
			}

			return (*_entries[eid]);
		}

		void NeighborDatabase::updateBundles(const dtn::data::EID &eid, const ibrcommon::BloomFilter &bf, const size_t lifetime)
		{
			NeighborEntry &entry = create(eid);
			entry.updateBundles(bf, lifetime);
		}

		bool NeighborDatabase::knowBundle(const dtn::data::EID &eid, const dtn::data::BundleID &bundle) throw (BloomfilterNotAvailableException)
		{
			try {
				NeighborEntry &entry = get(eid);
				return entry.getBundles().contains(bundle.toString());
			} catch (const NeighborNotAvailableException&) {
				throw BloomfilterNotAvailableException(eid);
			}
		}

		void NeighborDatabase::remove(const dtn::data::EID &eid)
		{
			_entries.erase(eid);
		}
	}
}

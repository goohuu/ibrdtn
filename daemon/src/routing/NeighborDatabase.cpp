/*
 * NeighborDatabase.cpp
 *
 *  Created on: 23.07.2010
 *      Author: morgenro
 */

#include "routing/NeighborDatabase.h"
#include <ibrdtn/utils/Clock.h>

namespace dtn
{
	namespace routing
	{
		NeighborDatabase::NeighborEntry::NeighborEntry()
		 : _eid(), _filter(), _filter_age(0), _lastseen(0), _lastupdate(0), _available(false)
		{};

		NeighborDatabase::NeighborEntry::NeighborEntry(const dtn::data::EID &eid)
		 : _eid(eid), _filter(), _filter_age(0), _lastseen(0), _lastupdate(0), _available(false)
		{ }

		NeighborDatabase::NeighborEntry::~NeighborEntry()
		{ }

		void NeighborDatabase::NeighborEntry::updateLastSeen()
		{
			_lastseen = dtn::utils::Clock::getTime();
		}

		void NeighborDatabase::NeighborEntry::updateBundles(const ibrcommon::BloomFilter &bf)
		{
			_filter = bf;
			_lastupdate = dtn::utils::Clock::getTime();
		}

		NeighborDatabase::NeighborDatabase()
		{
		}

		NeighborDatabase::~NeighborDatabase()
		{
		}

		NeighborDatabase::NeighborEntry& NeighborDatabase::get(const dtn::data::EID &eid)
		{
			if (_entries.find(eid) == _entries.end())
			{
				NeighborEntry entry(eid);
				_entries[eid] = entry;
			}

			return _entries[eid];
		}

		void NeighborDatabase::updateBundles(const dtn::data::EID &eid, const ibrcommon::BloomFilter &bf)
		{
			NeighborEntry &entry = get(eid);
			entry.updateBundles(bf);
		}

		bool NeighborDatabase::knowBundle(const dtn::data::EID &eid, const dtn::data::BundleID &bundle) throw (BloomfilterNotAvailableException)
		{
			NeighborEntry &entry = get(eid);
			return entry._filter.contains(bundle.toString());
		}

		void NeighborDatabase::setAvailable(const dtn::data::EID &eid)
		{
			NeighborEntry &entry = get(eid);
			entry.updateLastSeen();
			entry._available = true;
		}

		void NeighborDatabase::setUnavailable(const dtn::data::EID &eid)
		{
			NeighborEntry &entry = get(eid);
			entry.updateLastSeen();
			entry._available = false;
		}

		std::set<dtn::data::EID> NeighborDatabase::getAvailable() const
		{
			std::set<dtn::data::EID> ret;

			for (std::map<dtn::data::EID, NeighborDatabase::NeighborEntry>::const_iterator iter = _entries.begin(); iter != _entries.end(); iter++)
			{
				if ((*iter).second._available)
				{
					ret.insert((*iter).first);
				}
			}

			return ret;
		}
	}
}

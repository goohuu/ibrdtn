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
		 : _eid(), _transfer_semaphore(5), _transfer_max(5), _filter(), _filter_expire(0), _filter_state(FILTER_EXPIRED)
		{};

		NeighborDatabase::NeighborEntry::NeighborEntry(const dtn::data::EID &eid)
		 : _eid(eid), _transfer_semaphore(5), _transfer_max(5), _filter(), _filter_expire(0), _filter_state(FILTER_EXPIRED)
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
				// set the filter state to expired once
				_filter_state = FILTER_EXPIRED;
				throw BloomfilterNotAvailableException(_eid);
			}

			return _filter;
		}

		void NeighborDatabase::NeighborEntry::acquireFilterRequest() throw (NoMoreTransfersAvailable)
		{
			if (_filter_state == FILTER_EXPIRED)
				throw NoMoreTransfersAvailable();

			_filter_state = FILTER_AWAITING;
		}

		void NeighborDatabase::NeighborEntry::acquireTransfer() throw (NoMoreTransfersAvailable)
		{
			ibrcommon::MutexLock l(_transfer_lock);
			if (_transfer_semaphore == 0) throw NoMoreTransfersAvailable();
			_transfer_semaphore--;

			IBRCOMMON_LOGGER_DEBUG(20) << "acquire transfer (" << _transfer_semaphore << " left)" << IBRCOMMON_LOGGER_ENDL;
		}

		void NeighborDatabase::NeighborEntry::releaseTransfer()
		{
			ibrcommon::MutexLock l(_transfer_lock);
			if (_transfer_semaphore >= _transfer_max) return;
			_transfer_semaphore++;

			IBRCOMMON_LOGGER_DEBUG(20) << "release transfer (" << _transfer_semaphore << " left)" << IBRCOMMON_LOGGER_ENDL;
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

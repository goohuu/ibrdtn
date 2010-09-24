/*
 * BundleList.cpp
 *
 *  Created on: 19.02.2010
 *      Author: morgenro
 */

#include "ibrdtn/data/BundleList.h"
#include "ibrdtn/utils/Clock.h"

namespace dtn
{
	namespace data
	{
		BundleList::BundleList()
		{ }

		BundleList::~BundleList()
		{ }

		void BundleList::add(const dtn::data::MetaBundle bundle)
		{
			// insert bundle id to the private list
			_bundles.insert(bundle);

			// insert the bundle to the public list
			std::set<dtn::data::MetaBundle>::insert(bundle);
		}

		void BundleList::remove(const dtn::data::MetaBundle bundle)
		{
			// delete bundle id in the private list
			_bundles.erase(bundle);

			// delete bundle id in the public list
			std::set<dtn::data::MetaBundle>::erase(bundle);
		}

		void BundleList::clear()
		{
			_bundles.clear();
			std::set<dtn::data::MetaBundle>::clear();
		}

		bool BundleList::contains(const dtn::data::BundleID bundle) const
		{
			for (std::set<dtn::data::MetaBundle>::const_iterator iter = begin(); iter != end(); iter++)
			{
				if ((*iter) == bundle)
				{
					return true;
				}

				if ((*iter) > bundle)
				{
					return false;
				}
			}

			return false;
		}

		void BundleList::expire(const size_t timestamp)
		{
			bool commit = false;

			// we can not expire bundles if we have no idea of time
			if (dtn::utils::Clock::quality == 0) return;

			std::set<ExpiringBundle>::iterator iter = _bundles.begin();

			while (iter != _bundles.end())
			{
				const ExpiringBundle &b = (*iter);

				if ( b.expiretime >= timestamp ) break;

				// raise expired event
				eventBundleExpired( b );

				// remove this item in public list
				std::set<dtn::data::MetaBundle>::erase( b.bundle );

				// remove this item in private list
				_bundles.erase( iter++ );

				commit = true;
			}

			if (commit) eventCommitExpired();
		}

		BundleList::ExpiringBundle::ExpiringBundle(const MetaBundle b)
		 : bundle(b), expiretime(dtn::utils::Clock::getExpireTime(b.getTimestamp(), b.lifetime))
		{ }

		BundleList::ExpiringBundle::~ExpiringBundle()
		{ }

		bool BundleList::ExpiringBundle::operator!=(const ExpiringBundle& other) const
		{
			return !(other == *this);
		}

		bool BundleList::ExpiringBundle::operator==(const ExpiringBundle& other) const
		{
			return (other.bundle == this->bundle);
		}

		bool BundleList::ExpiringBundle::operator<(const ExpiringBundle& other) const
		{
			if (expiretime < other.expiretime) return true;
			if (expiretime != other.expiretime) return false;

			if (bundle < other.bundle) return true;

			return false;
		}

		bool BundleList::ExpiringBundle::operator>(const ExpiringBundle& other) const
		{
			return !(((*this) < other) || ((*this) == other));
		}
	}
}

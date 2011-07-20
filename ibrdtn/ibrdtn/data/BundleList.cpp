/*
 * BundleList.cpp
 *
 *  Created on: 19.02.2010
 *      Author: morgenro
 */

#include "ibrdtn/data/BundleList.h"
#include "ibrdtn/utils/Clock.h"
#include <algorithm>

namespace dtn
{
	namespace data
	{
		BundleList::BundleList()
		 : _version(0)
		{ }

		BundleList::~BundleList()
		{ }

		void BundleList::add(const dtn::data::MetaBundle &bundle)
		{
			// insert bundle id to the private list
			_bundles.insert(bundle);

			// insert the bundle to the public list
			std::set<dtn::data::MetaBundle>::insert(bundle);

			// increment the version
			_version++;
		}

		void BundleList::remove(const dtn::data::MetaBundle &bundle)
		{
			// delete bundle id in the private list
			_bundles.erase(bundle);

			// delete bundle id in the public list
			std::set<dtn::data::MetaBundle>::erase(bundle);

			// increment the version
			_version++;
		}

		void BundleList::clear()
		{
			_bundles.clear();
			std::set<dtn::data::MetaBundle>::clear();

			// increment the version
			_version++;
		}

		bool BundleList::contains(const dtn::data::BundleID &bundle) const
		{
			if (::find(begin(), end(), bundle) == end())
			{
				return false;
			}

			return true;
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

			if (commit)
			{
				eventCommitExpired();

				// increment the version
				_version++;
			}
		}

		bool BundleList::operator==(const size_t version) const
		{
			return (version == _version);
		}

		size_t BundleList::getVersion() const
		{
			return _version;
		}

		BundleList::ExpiringBundle::ExpiringBundle(const MetaBundle &b)
		 : bundle(b), expiretime(b.expiretime)
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

/*
 * BundleList.cpp
 *
 *  Created on: 19.02.2010
 *      Author: morgenro
 */

#include "routing/BundleList.h"

namespace dtn
{
	namespace routing
	{
		BundleList::BundleList()
		{ }

		BundleList::~BundleList()
		{ }

		void BundleList::add(const dtn::routing::MetaBundle bundle)
		{
			// insert bundle id to the private list
			_bundles.insert(bundle);

			// insert the bundle to the public list
			this->insert(bundle);
		}

		void BundleList::remove(const dtn::routing::MetaBundle bundle)
		{
			// delete bundle id in the private list
			_bundles.erase(bundle);

			// delete bundle id in the public list
			this->erase(bundle);
		}

		void BundleList::clear()
		{
			_bundles.clear();
			std::set<dtn::routing::MetaBundle>::clear();
		}

		void BundleList::expire(const size_t timestamp)
		{
			std::multiset<ExpiringBundle>::iterator iter = _bundles.begin();

			while (iter != _bundles.end())
			{
				const ExpiringBundle &b = (*iter);

				if ( b.expiretime >= timestamp )
				{
					break;
				}

				// raise expired event
				eventBundleExpired( b );

				// remove this item in public list
				(*this).erase( b.bundle );

				// remove this item in private list
				_bundles.erase( iter++ );
			}
		}

		BundleList::ExpiringBundle::ExpiringBundle(const MetaBundle b)
		 : bundle(b), expiretime(b.getTimestamp() + b.lifetime)
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
//			return (expiretime == other.expiretime);
		}

		bool BundleList::ExpiringBundle::operator<(const ExpiringBundle& other) const
		{
			return (expiretime < other.expiretime);
		}

		bool BundleList::ExpiringBundle::operator>(const ExpiringBundle& other) const
		{
			return (expiretime > other.expiretime);
		}
	}
}

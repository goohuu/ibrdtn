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
			this->insert(bundle);
			_bundles.insert(bundle);
		}

		void BundleList::remove(const dtn::routing::MetaBundle bundle)
		{
			this->erase(bundle);
			_bundles.erase(bundle);
		}

		void BundleList::expire(const size_t timestamp)
		{
			for (std::set<ExpiringBundle>::iterator iter = _bundles.begin(); iter != _bundles.end(); iter++)
			{
				if ((*iter).expiretime >= timestamp )
				{
					break;
				}

				// remove this item in public list
				(*this).erase( (*iter).bundle );

				// remove this item in private list
				_bundles.erase( iter );
			}
		}

		BundleList::ExpiringBundle::ExpiringBundle(const MetaBundle b)
		 : bundle(b), expiretime(b.getTimestamp() + b.lifetime)
		{ }

		BundleList::ExpiringBundle::~ExpiringBundle()
		{ }

		bool BundleList::ExpiringBundle::operator!=(const ExpiringBundle& other) const
		{
			return (other.bundle != this->bundle);
		}

		bool BundleList::ExpiringBundle::operator==(const ExpiringBundle& other) const
		{
			return (other.bundle == this->bundle);
		}

		bool BundleList::ExpiringBundle::operator<(const ExpiringBundle& other) const
		{
			return (other.expiretime < expiretime);
		}

		bool BundleList::ExpiringBundle::operator>(const ExpiringBundle& other) const
		{
			return (other.expiretime > expiretime);
		}
	}
}

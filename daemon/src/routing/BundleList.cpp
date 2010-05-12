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
			ExpiringBundle exbundle(bundle);

			// sorted insert of the bundle
			for (std::list<ExpiringBundle>::iterator iter = _bundles.begin(); iter != _bundles.end(); iter++)
			{
				if ((*iter) == exbundle) return;

				if ((*iter) > exbundle)
				{
					// put the new bundle in front of the greater bundle
					_bundles.insert( iter, exbundle );

					// insert the bundle
					this->insert(bundle);

					return;
				}
			}

			// the list is empty or no "greater" bundle available,
			// put the bundle at the end of the list
			_bundles.push_back( exbundle );

			// insert the bundle
			this->insert(bundle);
		}

		void BundleList::remove(const dtn::routing::MetaBundle bundle)
		{
			this->erase(bundle);

			for (std::list<ExpiringBundle>::iterator iter = _bundles.begin(); iter != _bundles.end(); iter++)
			{
				if ((*iter) == bundle)
				{
					_bundles.erase( iter );
					return;
				}
			}
		}

		void BundleList::clear()
		{
			_bundles.clear();
			std::set<dtn::routing::MetaBundle>::clear();
		}

		void BundleList::expire(const size_t timestamp)
		{
			std::list<ExpiringBundle>::iterator iter = _bundles.begin();

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
			return (other.bundle != this->bundle);
		}

		bool BundleList::ExpiringBundle::operator==(const ExpiringBundle& other) const
		{
			return (other.bundle == this->bundle);
		}

		bool BundleList::ExpiringBundle::operator<(const ExpiringBundle& other) const
		{
			if (expiretime < other.expiretime) return true;
			if (bundle < other.bundle) return true;

			return false;
		}

		bool BundleList::ExpiringBundle::operator>(const ExpiringBundle& other) const
		{
			if (expiretime > other.expiretime) return true;
			if (bundle > other.bundle) return true;

			return false;
		}
	}
}

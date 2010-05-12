/*
 * BundleList.h
 *
 *  Created on: 19.02.2010
 *      Author: morgenro
 */

#ifndef BUNDLELIST_H_
#define BUNDLELIST_H_

#include "routing/MetaBundle.h"
#include <list>
#include <set>

namespace dtn
{
	namespace routing
	{
		class BundleList : public std::set<dtn::routing::MetaBundle>
		{
		public:
			BundleList();
			~BundleList();

			void add(const dtn::routing::MetaBundle bundle);
			void remove(const dtn::routing::MetaBundle bundle);
			void clear();

			void expire(const size_t timestamp);

		protected:
			class ExpiringBundle
			{
			public:
				ExpiringBundle(const MetaBundle b);
				~ExpiringBundle();

				bool operator!=(const ExpiringBundle& other) const;
				bool operator==(const ExpiringBundle& other) const;
				bool operator<(const ExpiringBundle& other) const;
				bool operator>(const ExpiringBundle& other) const;

				const MetaBundle bundle;
				const size_t expiretime;
			};

			virtual void eventBundleExpired(const ExpiringBundle &b) {};

			std::list<ExpiringBundle> _bundles;
		};

	}
}

#endif /* BUNDLELIST_H_ */

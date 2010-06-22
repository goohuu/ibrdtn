/*
 * BundleList.h
 *
 *  Created on: 19.02.2010
 *      Author: morgenro
 */

#ifndef BUNDLELIST_H_
#define BUNDLELIST_H_

#include <ibrdtn/data/MetaBundle.h>
#include <set>

namespace dtn
{
	namespace data
	{
		class BundleList : public std::set<dtn::data::MetaBundle>
		{
		public:
			BundleList();
			~BundleList();

			void add(const dtn::data::MetaBundle bundle);
			void remove(const dtn::data::MetaBundle bundle);
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

			virtual void eventBundleExpired(const ExpiringBundle&) {};

			std::multiset<ExpiringBundle> _bundles;
		};

	}
}

#endif /* BUNDLELIST_H_ */

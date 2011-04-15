/*
 * BundleFragment.h
 *
 *  Created on: 14.04.2011
 *      Author: morgenro
 */

#ifndef BUNDLEFRAGMENT_H_
#define BUNDLEFRAGMENT_H_

namespace dtn
{
	namespace data
	{
		class BundleFragment
		{
			public:
				BundleFragment(const dtn::data::Bundle &bundle, size_t payload_length);
				BundleFragment(const dtn::data::Bundle &bundle, size_t offset, size_t payload_length);
				virtual ~BundleFragment();

				const size_t _offset;
				const size_t _length;
				const dtn::data::Bundle &_bundle;
		};
	}
}

#endif /* BUNDLEFRAGMENT_H_ */

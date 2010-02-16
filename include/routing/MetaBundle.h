/*
 * MetaBundle.h
 *
 *  Created on: 16.02.2010
 *      Author: morgenro
 */

#ifndef METABUNDLE_H_
#define METABUNDLE_H_

#include "ibrdtn/data/BundleID.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/data/DTNTime.h"
#include "ibrdtn/data/EID.h"

namespace dtn
{
	namespace routing
	{
		class MetaBundle : public dtn::data::BundleID
		{
		public:
			MetaBundle(const dtn::data::Bundle &b);
			~MetaBundle();

			const dtn::data::DTNTime received;
			const size_t lifetime;
			const dtn::data::EID destination;
		};
	}
}

#endif /* METABUNDLE_H_ */

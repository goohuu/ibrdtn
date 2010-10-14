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
	namespace data
	{
		class MetaBundle : public dtn::data::BundleID
		{
		public:
			MetaBundle();

			MetaBundle(const dtn::data::BundleID id, const size_t lifetime, const dtn::data::DTNTime received = dtn::data::DTNTime(),
					const dtn::data::EID destination = dtn::data::EID(), const dtn::data::EID reportto = dtn::data::EID(),
					const dtn::data::EID custodian = dtn::data::EID(), const size_t appdatalength = 0, const size_t procflags = 0);

			MetaBundle(const dtn::data::Bundle &b);
			~MetaBundle();

			int getPriority() const;

			dtn::data::DTNTime received;
			size_t lifetime;
			dtn::data::EID destination;
			dtn::data::EID reportto;
			dtn::data::EID custodian;
			size_t appdatalength;
			size_t procflags;
		};
	}
}

#endif /* METABUNDLE_H_ */

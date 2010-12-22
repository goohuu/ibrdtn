/*
 * BundleStorage.cpp
 *
 *  Created on: 24.03.2009
 *      Author: morgenro
 */

#include "core/BundleCore.h"
#include "core/BundleStorage.h"
#include "core/CustodyEvent.h"
#include "core/BundleGeneratedEvent.h"
#include <ibrdtn/data/BundleID.h>

namespace dtn
{
	namespace core
	{
		BundleStorage::BundleStorage()
		{
			_destination_filter.insert(dtn::core::BundleCore::local);
		}

		BundleStorage::~BundleStorage()
		{
		}

		void BundleStorage::addToFilter(const dtn::data::EID &eid)
		{
			_destination_filter.insert(eid);
		}

		void BundleStorage::remove(const dtn::data::Bundle &b)
		{
			remove(dtn::data::BundleID(b));
		};

		void BundleStorage::acceptCustody(dtn::data::Bundle &bundle)
		{
			if (bundle._custodian == EID()) return;

			if (bundle.get(Bundle::CUSTODY_REQUESTED))
			{
				// we are the new custodian for this bundle
				bundle._custodian = dtn::core::BundleCore::local;

				// create a new bundle
				Bundle b;

				// send a custody signal with accept flag
				CustodySignalBlock &signal = b.push_back<CustodySignalBlock>();

				// set the bundle to match
				signal.setMatch(bundle);

				// set accepted
				signal._status |= 1;

				b.set(dtn::data::PrimaryBlock::DESTINATION_IS_SINGLETON, true);
				b._destination = bundle._custodian;
				b._source = dtn::core::BundleCore::local;

				// send the custody accepted bundle
				dtn::core::BundleGeneratedEvent::raise(b);

				// raise the custody accepted event
				dtn::core::CustodyEvent::raise(b, CUSTODY_ACCEPT);
			}
		}

		void BundleStorage::rejectCustody(const dtn::data::Bundle &bundle)
		{
			if (bundle._custodian == EID()) return;

			if (bundle.get(Bundle::CUSTODY_REQUESTED))
			{
				// create a new bundle
				Bundle b;

				// send a custody signal with reject flag
				CustodySignalBlock &signal = b.push_back<CustodySignalBlock>();

				// set the bundle to match
				signal.setMatch(bundle);

				b._destination = bundle._custodian;
				b._source = BundleCore::local;

				// send the custody rejected bundle
				dtn::core::BundleGeneratedEvent::raise(b);

				// raise the custody rejected event
				dtn::core::CustodyEvent::raise(b, CUSTODY_REJECT);
			}
		}
	}
}

/*
 * BundleStorage.cpp
 *
 *  Created on: 24.03.2009
 *      Author: morgenro
 */

#include "core/BundleStorage.h"
#include "ibrdtn/data/BundleID.h"

namespace dtn
{
	namespace core
	{
		BundleStorage::BundleStorage()
		{
		}

		BundleStorage::~BundleStorage()
		{
		}

		void BundleStorage::remove(const dtn::data::Bundle &b)
		{
			remove(dtn::data::BundleID(b));
		};
	}
}

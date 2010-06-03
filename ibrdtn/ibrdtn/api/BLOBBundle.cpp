/*
 * BLOBBundle.cpp
 *
 *  Created on: 25.01.2010
 *      Author: morgenro
 */


#include "ibrdtn/api/BLOBBundle.h"

namespace dtn
{
	namespace api
	{
		BLOBBundle::BLOBBundle(dtn::data::EID destination, ibrcommon::BLOB::Reference &ref) : Bundle(destination)
		{
			dtn::data::PayloadBlock &payload = _b.appendPayloadBlock(ref);
		}

		BLOBBundle::~BLOBBundle()
		{
		}
	}
}

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
			_payload = new dtn::data::PayloadBlock(ref);

			// add the payload block to the bundle
			_b.addBlock(_payload);
		}

		BLOBBundle::~BLOBBundle()
		{
		}
	}
}

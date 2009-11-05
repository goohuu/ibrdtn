/*
 * StringBundle.cpp
 *
 *  Created on: 24.07.2009
 *      Author: morgenro
 */

#include "ibrdtn/api/StringBundle.h"


namespace dtn
{
	namespace api
	{
		StringBundle::StringBundle(dtn::data::EID destination) : Bundle(destination)
		{
			_payload = new dtn::data::PayloadBlock(dtn::blob::BLOBManager::BLOB_HARDDISK);

			// add the payload block to the bundle
			_b.addBlock(_payload);
		}

		StringBundle::~StringBundle()
		{
		}

		void StringBundle::append(string data)
		{
			_payload->getBLOBReference().append(data.c_str(), data.length());
		}
	}
}

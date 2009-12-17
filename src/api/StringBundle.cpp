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
			_payload = new dtn::data::PayloadBlock(ibrcommon::StringBLOB::create());

			// add the payload block to the bundle
			_b.addBlock(_payload);
		}

		StringBundle::~StringBundle()
		{
		}

		void StringBundle::append(string data)
		{
			(*_payload->getBLOB()) << data;
		}
	}
}

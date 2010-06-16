/*
 * StringBundle.cpp
 *
 *  Created on: 24.07.2009
 *      Author: morgenro
 */

#include "ibrdtn/config.h"
#include "ibrdtn/api/StringBundle.h"


namespace dtn
{
	namespace api
	{
		StringBundle::StringBundle(dtn::data::EID destination)
		 : Bundle(destination), _payload(_b.push_back<dtn::data::PayloadBlock>())
		{
		}

		StringBundle::~StringBundle()
		{
		}

		void StringBundle::append(string data)
		{
			(*_payload.getBLOB()) << data;
		}
	}
}

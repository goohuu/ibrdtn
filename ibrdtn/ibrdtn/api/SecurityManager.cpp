/*
 * SecurityManager.cpp
 *
 *  Created on: 27.07.2009
 *      Author: morgenro
 */

#include "ibrdtn/config.h"
#include "ibrdtn/api/SecurityManager.h"
#include "ibrdtn/api/Bundle.h"

namespace dtn
{
	namespace api
	{
		// default is sec off.
		Bundle::BUNDLE_SECURITY SecurityManager::_default = Bundle::SEC_OFF;

		SecurityManager::SecurityManager()
		{
		}

		SecurityManager::~SecurityManager()
		{
		}

		void SecurityManager::initialize(const std::string&, const std::string&, const std::string&)
		{
		}

		void SecurityManager::setDefault(const Bundle::BUNDLE_SECURITY s)
		{
			SecurityManager::_default = s;
		}

		Bundle::BUNDLE_SECURITY SecurityManager::getDefault()
		{
			return SecurityManager::_default;
		}

		bool SecurityManager::validate(const dtn::api::Bundle&)
		{
			return false;
		}

	}
}

/*
 * SecurityKeyManager.cpp
 *
 *  Created on: 02.12.2010
 *      Author: morgenro
 */

#include "security/SecurityKeyManager.h"

namespace dtn
{
	namespace security
	{
		SecurityKeyManager& SecurityKeyManager::getInstance()
		{
			static SecurityKeyManager instance;
			return instance;
		}

		SecurityKeyManager::SecurityKeyManager()
		{

		}

		SecurityKeyManager::~SecurityKeyManager()
		{

		}

		void SecurityKeyManager::initialize(const ibrcommon::File &path)
		{

		}

		void SecurityKeyManager::prefetchKey(const dtn::data::EID &ref, const KeyType type)
		{

		}

		bool SecurityKeyManager::hasKey(const dtn::data::EID &ref, const KeyType type) const
		{
			return false;
		}

		SecurityKeyManager::SecurityKey SecurityKeyManager::get(const dtn::data::EID &ref, const KeyType type) const throw (SecurityKeyManager::KeyNotFoundException)
		{
			return SecurityKeyManager::SecurityKey();
		}

		void SecurityKeyManager::store(const dtn::data::EID &ref, const std::string &data, const KeyType type)
		{

		}
	}
}

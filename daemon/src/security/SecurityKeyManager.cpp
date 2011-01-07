/*
 * SecurityKeyManager.cpp
 *
 *  Created on: 02.12.2010
 *      Author: morgenro
 */

#include "security/SecurityKeyManager.h"
#include <ibrcommon/Logger.h>
#include <sstream>
#include <iomanip>
#include <fstream>

#include <openssl/pem.h>
#include <openssl/err.h>

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

		void SecurityKeyManager::initialize(const ibrcommon::File &path, const ibrcommon::File &ca, const ibrcommon::File &key)
		{
			IBRCOMMON_LOGGER(info) << "security key manager initialized; path: " << path.getPath() << IBRCOMMON_LOGGER_ENDL;

			// store all paths locally
			_path = path; _key = key; _ca = ca;
		}

		const std::string SecurityKeyManager::hash(const dtn::data::EID &eid)
		{
			std::string value = eid.getNodeEID();
			std::stringstream ss;
			for (std::string::const_iterator iter = value.begin(); iter != value.end(); iter++)
			{
				ss << std::hex << std::setw( 2 ) << std::setfill( '0' ) << (int)(*iter);
			}
			return ss.str();
		}

		void SecurityKeyManager::prefetchKey(const dtn::data::EID &ref, const dtn::security::SecurityKey::KeyType type)
		{
		}

		bool SecurityKeyManager::hasKey(const dtn::data::EID &ref, const dtn::security::SecurityKey::KeyType type) const
		{
			const ibrcommon::File keyfile = _path.get(hash(ref.getNodeEID()) + ".pem");
			return keyfile.exists();
		}

		dtn::security::SecurityKey SecurityKeyManager::get(const dtn::data::EID &ref, const dtn::security::SecurityKey::KeyType type) const throw (SecurityKeyManager::KeyNotFoundException)
		{
			const ibrcommon::File keyfile = _path.get(hash(ref.getNodeEID()) + ".pem");

			if (!keyfile.exists())
			{
				IBRCOMMON_LOGGER(warning) << "Key file for " << ref.getString() << " (" << keyfile.getPath() << ") not found" << IBRCOMMON_LOGGER_ENDL;
				throw SecurityKeyManager::KeyNotFoundException();
			}

			dtn::security::SecurityKey keydata;
			keydata.file = keyfile;
			keydata.type = type;
			keydata.lastupdate = keyfile.lastmodify();
			keydata.reference = ref;

			return keydata;
		}

		void SecurityKeyManager::store(const dtn::data::EID &ref, const std::string &data, const dtn::security::SecurityKey::KeyType type)
		{
			ibrcommon::File keyfile = _path.get(hash(ref.getNodeEID()) + ".pem");

			// delete if already exists
			if (keyfile.exists()) keyfile.remove();

			std::ofstream keystream(keyfile.getPath().c_str());
			keystream << data;
			keystream.close();
		}
	}
}

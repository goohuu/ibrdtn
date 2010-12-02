/*
 * SecurityKeyManager.h
 *
 *  Created on: 02.12.2010
 *      Author: morgenro
 */

#ifndef SECURITYKEYMANAGER_H_
#define SECURITYKEYMANAGER_H_

#include <ibrdtn/data/EID.h>
#include <ibrdtn/data/DTNTime.h>
#include <ibrdtn/data/BundleString.h>
#include <ibrdtn/data/SDNV.h>
#include <ibrcommon/data/File.h>
#include <iostream>

namespace dtn
{
	namespace security
	{
		class SecurityKeyManager
		{
		public:
			enum KeyType
			{
				KEY_UNSPEC = 0,
				KEY_SHARED = 1,
				KEY_PRIVATE = 2,
				KEY_PUBLIC = 3
			};

			class KeyNotFoundException : public ibrcommon::Exception
			{
			public:
				KeyNotFoundException(std::string what = "Requested key not found.") : ibrcommon::Exception(what)
				{};

				virtual ~KeyNotFoundException() throw() {};
			};

			class SecurityKey
			{
			public:
				SecurityKey() {}
				virtual ~SecurityKey() {};

				// key type
				KeyType type;

				// referencing EID of this key
				dtn::data::EID reference;

				// last update time
				dtn::data::DTNTime lastupdate;

				// key data
				std::string data;

				friend std::ostream &operator<<(std::ostream &stream, const SecurityKey &key)
				{
					// key type
					stream << dtn::data::SDNV(key.type);

					// EID reference
					stream << dtn::data::BundleString(key.reference.getString());

					// timestamp of last update
					stream << key.lastupdate;

					// key data
					stream << dtn::data::BundleString(key.data);

					// To support concatenation of streaming calls, we return the reference to the output stream.
					return stream;
				}

				friend std::istream &operator>>(std::istream &stream, SecurityKey &key)
				{
					// key type
					dtn::data::SDNV sdnv_type; stream >> sdnv_type;
					key.type = KeyType(sdnv_type.getValue());

					// EID reference
					dtn::data::BundleString eid_reference; stream >> eid_reference;
					key.reference = dtn::data::EID(eid_reference);

					// timestamp of last update
					stream >> key.lastupdate;

					// key data
					dtn::data::BundleString data_string; stream >> data_string;
					key.data = data_string;

					// To support concatenation of streaming calls, we return the reference to the input stream.
					return stream;
				}
			};

			static SecurityKeyManager& getInstance();

			SecurityKeyManager();
			virtual ~SecurityKeyManager();
			void initialize(const ibrcommon::File &path);

			void prefetchKey(const dtn::data::EID &ref, const KeyType type = KEY_UNSPEC);

			bool hasKey(const dtn::data::EID &ref, const KeyType type = KEY_UNSPEC) const;
			SecurityKeyManager::SecurityKey get(const dtn::data::EID &ref, const KeyType type = KEY_UNSPEC) const throw (SecurityKeyManager::KeyNotFoundException);
			void store(const dtn::data::EID &ref, const std::string &data, const KeyType type = KEY_UNSPEC);
		};
	}
}

#endif /* SECURITYKEYMANAGER_H_ */

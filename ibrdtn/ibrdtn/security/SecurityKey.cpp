/*
 * SecurityKey.cpp
 *
 *  Created on: 06.01.2011
 *      Author: morgenro
 */

#include "ibrdtn/security/SecurityKey.h"
#include <ibrcommon/Logger.h>

#include <openssl/pem.h>
#include <openssl/err.h>

namespace dtn
{
	namespace security
	{
		SecurityKey::SecurityKey()
		{};

		SecurityKey::~SecurityKey()
		{};

		void SecurityKey::free(RSA* key)
		{
			RSA_free(key);
		}

		RSA* SecurityKey::getRSA() const
		{
			switch (type)
			{
			case KEY_PRIVATE:
				return getPrivateRSA();
			case KEY_PUBLIC:
				return getPublicRSA();
			}
		}

		RSA* SecurityKey::getPrivateRSA() const
		{
			RSA *rsa = RSA_new();

			FILE * rsa_pkey_file = fopen(file.getPath().c_str(), "r");
			if (!rsa_pkey_file) {
				IBRCOMMON_LOGGER_ex(critical) << "Failed to open " << file.getPath() << IBRCOMMON_LOGGER_ENDL;
				throw ibrcommon::Exception("Failed to open " + file.getPath());
			}
			if (!PEM_read_RSAPrivateKey(rsa_pkey_file, &rsa, NULL, NULL)) {
				IBRCOMMON_LOGGER_ex(critical) << "Error loading RSA private key file: " << file.getPath() << IBRCOMMON_LOGGER_ENDL;
				ERR_print_errors_fp(stderr);
				throw ibrcommon::Exception("Error loading RSA private key file: " + file.getPath());
			}
			fclose(rsa_pkey_file);
			return rsa;
		}

		RSA* SecurityKey::getPublicRSA() const
		{
			RSA *rsa = RSA_new();

			FILE * rsa_pkey_file = fopen(file.getPath().c_str(), "r");
			if (!rsa_pkey_file) {
				IBRCOMMON_LOGGER_ex(critical) << "Failed to open " << file.getPath() << IBRCOMMON_LOGGER_ENDL;
				throw ibrcommon::Exception("Failed to open " + file.getPath());
			}
			if (!PEM_read_RSA_PUBKEY(rsa_pkey_file, &rsa, NULL, NULL)) {
				IBRCOMMON_LOGGER_ex(critical) << "Error loading RSA public key file: " << file.getPath() << IBRCOMMON_LOGGER_ENDL;
				ERR_print_errors_fp(stderr);
				throw ibrcommon::Exception("Error loading RSA public key file: " + file.getPath());
			}
			fclose(rsa_pkey_file);
			return rsa;
		}
	}
}

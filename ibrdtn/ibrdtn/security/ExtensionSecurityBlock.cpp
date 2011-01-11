#include "ibrdtn/security/ExtensionSecurityBlock.h"
#include <ibrcommon/Logger.h>
#include "ibrdtn/data/Serializer.h"
#include "ibrdtn/data/Bundle.h"
#include <openssl/err.h>
#include <openssl/rsa.h>

#ifdef __DEVELOPMENT_ASSERTIONS__
#include <cassert>
#endif

namespace dtn
{
	namespace security
	{
		dtn::data::Block* ExtensionSecurityBlock::Factory::create()
		{
			return new ExtensionSecurityBlock();
		}

		ExtensionSecurityBlock::ExtensionSecurityBlock()
		 : SecurityBlock(EXTENSION_SECURITY_BLOCK, ESB_RSA_AES128_EXT)
		{
		}

		ExtensionSecurityBlock::~ExtensionSecurityBlock()
		{
		}

		void ExtensionSecurityBlock::encrypt(dtn::data::Bundle& bundle, const SecurityKey &key, const dtn::data::Block &block, const dtn::data::EID& source, const dtn::data::EID& destination)
		{
			u_int32_t salt = 0;

			// load the rsa key
			RSA *rsa_key = key.getRSA();

			// key used for encrypting the block. the key will be encrypted using RSA
			unsigned char ephemeral_key[ibrcommon::AES128Stream::key_size_in_bytes];
			createSaltAndKey(salt, ephemeral_key, ibrcommon::AES128Stream::key_size_in_bytes);

			dtn::security::ExtensionSecurityBlock& esb = SecurityBlock::encryptBlock<ExtensionSecurityBlock>(bundle, block, salt, ephemeral_key);

			// set the source and destination address of the new block
			if (source != bundle._source) esb.setSecuritySource( source );
			if (destination != bundle._destination) esb.setSecurityDestination( destination );

			// encrypt the ephemeral key and place it in _ciphersuite_params
			addSalt(esb._ciphersuite_params, salt);
			addKey(esb._ciphersuite_params, ephemeral_key, ibrcommon::AES128Stream::key_size_in_bytes, rsa_key);
			esb._ciphersuite_flags |= CONTAINS_CIPHERSUITE_PARAMS;

			// free the rsa key
			key.free(rsa_key);
		}

		void ExtensionSecurityBlock::decrypt(dtn::data::Bundle& bundle, const SecurityKey &key, const dtn::security::ExtensionSecurityBlock& block)
		{
			// load the rsa key
			RSA *rsa_key = key.getRSA();

			// get key, convert with reinterpret_cast
			unsigned char keydata[ibrcommon::AES128Stream::key_size_in_bytes];

			if (!getKey(block._ciphersuite_params, keydata, ibrcommon::AES128Stream::key_size_in_bytes, rsa_key))
			{
				IBRCOMMON_LOGGER_ex(critical) << "could not get symmetric key decrypted" << IBRCOMMON_LOGGER_ENDL;
				throw ibrcommon::Exception("could not extract the key");
			}

			// get salt, convert with stringstream
			u_int32_t salt = getSalt(block._ciphersuite_params);

			SecurityBlock::decryptBlock(bundle, block, salt, keydata);
		}

		void ExtensionSecurityBlock::decrypt(dtn::data::Bundle& bundle, const SecurityKey &key, u_int64_t correlator)
		{
			const std::list<const dtn::security::ExtensionSecurityBlock*> blocks = bundle.getBlocks<ExtensionSecurityBlock>();

			for (std::list<const dtn::security::ExtensionSecurityBlock*>::const_iterator it = blocks.begin(); it != blocks.end(); it++)
			{
				const dtn::security::ExtensionSecurityBlock &esb = (**it);

				if ((correlator == 0) || (correlator == esb._correlator))
				{
					decrypt(bundle, key, esb);
				}
			}
		}
	}
}

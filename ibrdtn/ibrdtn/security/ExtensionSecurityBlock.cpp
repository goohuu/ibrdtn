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
		 : SecurityBlock(EXTENSION_SECURITY_BLOCK, ESB_RSA_AES128_EXT), _rsa(0)
		{
		}

		ExtensionSecurityBlock::ExtensionSecurityBlock(const dtn::data::Bundle& bundle)
		 : SecurityBlock(EXTENSION_SECURITY_BLOCK, ESB_RSA_AES128_EXT), _rsa(0)
		{
		}

		ExtensionSecurityBlock::ExtensionSecurityBlock(RSA* long_key)
		 : SecurityBlock(EXTENSION_SECURITY_BLOCK), _rsa(long_key)
		{
		}

		ExtensionSecurityBlock::~ExtensionSecurityBlock()
		{
		}

		void ExtensionSecurityBlock::encryptBlock(dtn::data::Bundle& bundle, const dtn::data::Block*const block, const dtn::data::EID& source, const dtn::data::EID& destination) const
		{
			if (block == NULL)
				return;

			u_int32_t salt;

			// key used for encrypting the block. the key will be encrypted using RSA
			unsigned char ephemeral_key[ibrcommon::AES128Stream::key_size_in_bytes];
			createSaltAndKey(salt, ephemeral_key, ibrcommon::AES128Stream::key_size_in_bytes);
			dtn::security::ExtensionSecurityBlock& esb = SecurityBlock::encryptBlock<ExtensionSecurityBlock>(bundle, block, salt, ephemeral_key);

			// set the source and destination address of the new block
			if (source != bundle._source) esb.setSecuritySource( source );
			if (destination != bundle._destination) esb.setSecurityDestination( destination );

			// encrypt the ephemeral key and place it in _ciphersuite_params
			addSalt(esb._ciphersuite_params, salt);
			addKey(esb._ciphersuite_params, ephemeral_key, ibrcommon::AES128Stream::key_size_in_bytes, _rsa);
			esb._ciphersuite_flags |= CONTAINS_CIPHERSUITE_PARAMS;
		}

		void ExtensionSecurityBlock::encryptBlock(dtn::data::Bundle& bundle, const std::list<dtn::data::Block const *>& blocks, const dtn::data::EID& source, const dtn::data::EID& destination) const
		{
			if (blocks.size() < 2)
			{
				IBRCOMMON_LOGGER_ex(critical) << "need at least two blocks for group encryption" << IBRCOMMON_LOGGER_ENDL;
				return;
			}

			u_int32_t salt;
			// key used for encrypting the block. the key will be encrypted using RSA
			unsigned char ephemeral_key[ibrcommon::AES128Stream::key_size_in_bytes];
			createSaltAndKey(salt, ephemeral_key, ibrcommon::AES128Stream::key_size_in_bytes);

			u_int64_t correlator = createCorrelatorValue(bundle);

			std::list<dtn::data::Block const *>::const_iterator it = blocks.begin();
			if (*it != NULL)
			{
				dtn::security::ExtensionSecurityBlock& esb = SecurityBlock::encryptBlock<ExtensionSecurityBlock>(bundle, *it, salt, ephemeral_key);
				esb.setCorrelator(correlator);

				// set the source and destination address of the new block
				if (source != bundle._source) esb.setSecuritySource( source );
				if (destination != bundle._destination) esb.setSecurityDestination( destination );

				// encrypt the ephemeral key and place it in _ciphersuite_params
				addSalt(esb._ciphersuite_params, salt);
				addKey(esb._ciphersuite_params, ephemeral_key, ibrcommon::AES128Stream::key_size_in_bytes, _rsa);
				esb._ciphersuite_flags |= CONTAINS_CIPHERSUITE_PARAMS;
			}
			it++;

			for (; it != blocks.end(); it++)
				if (*it != NULL)
				{
					dtn::security::ExtensionSecurityBlock& esb = SecurityBlock::encryptBlock<ExtensionSecurityBlock>(bundle, *it, salt, ephemeral_key);
					esb.setCorrelator(correlator);
				}
		}

		bool ExtensionSecurityBlock::decryptBlock(dtn::data::Bundle& bundle) const
		{
//			std::list <const dtn::security::ExtensionSecurityBlock* > our_blocks;
//			std::list <const dtn::security::ExtensionSecurityBlock* > esbs = bundle.getBlocks<ExtensionSecurityBlock>();
//			// get our blocks
//			for (std::list<const dtn::security::ExtensionSecurityBlock* >::const_iterator it = esbs.begin(); it != esbs.end(); it++)
//				if ((**it).isSecurityDestination(bundle, _our_id) && (*it)->_ciphersuite_id == SecurityBlock::ESB_RSA_AES128_EXT)
//					our_blocks.push_back(*it);
//
//			bool success = true;
//			for (std::list<const dtn::security::ExtensionSecurityBlock* >::const_iterator it = our_blocks.begin(); it != our_blocks.end();)
//			{
//#ifdef __DEVELOPMENT_ASSERTIONS__
//				assert(getTLVs((**it)._ciphersuite_params, SecurityBlock::salt).size() > 0);
//#endif
//				if ((**it)._ciphersuite_flags & SecurityBlock::CONTAINS_CORRELATOR)
//					success &= decryptBlocks(bundle, (**it)._correlator);
//				else
//					success &= decryptBlock(bundle, *it);
//
//				// recollect all remaining blocks
//				our_blocks.clear();
//				esbs = bundle.getBlocks<ExtensionSecurityBlock>();
//				for (std::list<const dtn::security::ExtensionSecurityBlock* >::const_iterator esbit = esbs.begin(); esbit != esbs.end(); esbit++)
//					if ((**it).SecurityBlock::isSecurityDestination(bundle, _our_id) && (*it)->_ciphersuite_id == SecurityBlock::ESB_RSA_AES128_EXT)
//						our_blocks.push_back(*esbit);
//				it = our_blocks.begin();
//			}
//			return success;
		}

		bool ExtensionSecurityBlock::decryptBlock(dtn::data::Bundle& bundle, const dtn::security::ExtensionSecurityBlock* block) const
		{
			if (block == NULL)
				return false;

			// get key, convert with reinterpret_cast
			unsigned char key[ibrcommon::AES128Stream::key_size_in_bytes];
			if (!getKey(block->_ciphersuite_params, key, ibrcommon::AES128Stream::key_size_in_bytes, _rsa))
			{
				IBRCOMMON_LOGGER_ex(critical) << "could not get symmetric key decrypted" << IBRCOMMON_LOGGER_ENDL;
				return false;
			}

			// get salt, convert with stringstream
			u_int32_t salt = getSalt(block->_ciphersuite_params);

			try {
				SecurityBlock::decryptBlock(bundle, block, salt, key);
			} catch (const ibrcommon::Exception&) {
				return false;
			}

			return true;
		}

		bool ExtensionSecurityBlock::decryptBlock(dtn::data::Bundle& bundle, std::list<dtn::security::ExtensionSecurityBlock const *>& blocks) const
		{
			if (blocks.size() < 2)
			{
				IBRCOMMON_LOGGER_ex(critical) << "need at least two blocks for group decryption" << IBRCOMMON_LOGGER_ENDL;
				return false;
			}

			u_int32_t salt;
			unsigned char ephemeral_key[ibrcommon::AES128Stream::key_size_in_bytes];

			// get salt and key from first block
			std::list<dtn::security::ExtensionSecurityBlock const *>::iterator it = blocks.begin();
			if (*it != NULL)
			{
				salt = getSalt((*it)->_ciphersuite_params);
				if (!getKey((*it)->_ciphersuite_params, ephemeral_key, ibrcommon::AES128Stream::key_size_in_bytes, _rsa))
				{
					IBRCOMMON_LOGGER_ex(critical) << "could not get symmetric key decrypted" << IBRCOMMON_LOGGER_ENDL;
					return false;
				}
			}

			// decrypt all blocks
			for (; it != blocks.end(); it++)
				if (*it != NULL)
				{
					try {
						SecurityBlock::decryptBlock(bundle, *it, salt, ephemeral_key);
					} catch (const ibrcommon::Exception&) {
						return false;
					}
				}

			return true;
		}

		bool ExtensionSecurityBlock::decryptBlocks(dtn::data::Bundle& bundle, u_int64_t correlator) const
		{
			std::list<dtn::security::ExtensionSecurityBlock const *>blocks;
			std::list<dtn::security::ExtensionSecurityBlock const *>all_blocks = bundle.getBlocks<ExtensionSecurityBlock>();
			for (std::list<dtn::security::ExtensionSecurityBlock const *>::iterator it = all_blocks.begin(); it != all_blocks.end(); it++)
				if (*it != NULL && (*it)->_correlator == correlator)
					blocks.push_back(*it);

			return decryptBlock(bundle, blocks);
		}
	}
}

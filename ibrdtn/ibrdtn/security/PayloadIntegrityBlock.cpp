#include "ibrdtn/security/PayloadIntegrityBlock.h"
#include "ibrdtn/security/MutualSerializer.h"
#include "ibrdtn/data/Bundle.h"

#include <ibrcommon/ssl/RSASHA256Stream.h>
#include <ibrcommon/Logger.h>
#include <openssl/err.h>
#include <openssl/rsa.h>

#ifdef __DEVELOPMENT_ASSERTIONS__
#include <cassert>
#endif

namespace dtn
{
	namespace security
	{
		dtn::data::Block* PayloadIntegrityBlock::Factory::create()
		{
			return new PayloadIntegrityBlock();
		}

		PayloadIntegrityBlock::PayloadIntegrityBlock()
		 : SecurityBlock(PAYLOAD_INTEGRITY_BLOCK, PIB_RSA_SHA256), pkey(0), key_size(-1)
		{
		}

		PayloadIntegrityBlock::PayloadIntegrityBlock(RSA * hkey)
		 : SecurityBlock(PAYLOAD_INTEGRITY_BLOCK), key_size(-1)
		{
			pkey = EVP_PKEY_new();
			if (pkey == NULL)
			{
				IBRCOMMON_LOGGER_ex(critical) << "failed to create the key object" << IBRCOMMON_LOGGER_ENDL;
				ERR_print_errors_fp(stderr);
#ifdef __DEVELOPMENT_ASSERTIONS__
				assert(pkey != NULL);
#endif
			}
			// statt assign set1 benutzen, damit hkey nicht gelöscht wird
			// http://www.openssl.org/docs/crypto/EVP_PKEY_set1_RSA.html
			if (!EVP_PKEY_set1_RSA(pkey, hkey))
			{
				IBRCOMMON_LOGGER_ex(critical) << "failed to set RSA key" << IBRCOMMON_LOGGER_ENDL;
				ERR_print_errors_fp(stderr);
				EVP_PKEY_free(pkey);
				pkey = 0;
			}
		}

		PayloadIntegrityBlock::~PayloadIntegrityBlock()
		{
			EVP_PKEY_free(pkey);
		}

		size_t PayloadIntegrityBlock::getSecurityResultSize() const
		{
			size_t size = 0;
#ifdef __DEVELOPMENT_ASSERTIONS__
			assert(EVP_PKEY_size(pkey) > 0 || key_size > 0 || _security_result.size() > 0);
#endif

			if (EVP_PKEY_size(pkey) <= 0 && key_size <= 0)
				key_size = _security_result.size();

			// size of integrity_signature
			if (EVP_PKEY_size(pkey) > 0)
			{
				size += EVP_PKEY_size(pkey);
				// size of integrity_signature length
				size += dtn::data::SDNV::getLength(EVP_PKEY_size(pkey));
				// TLV type
				size++;
			}
			else
				size = key_size;

			return size;
		}

		void PayloadIntegrityBlock::addHash(dtn::data::Bundle &bundle, const dtn::data::EID& source, const dtn::data::EID& destination) const
		{
			IBRCOMMON_LOGGER_DEBUG_ex(ibrcommon::Logger::LOGGER_DEBUG) << "entering ..." << IBRCOMMON_LOGGER_ENDL;
			// if key is invalid do nothing
			if (pkey == NULL)
			{
				IBRCOMMON_LOGGER_ex(critical) << "trying to sign without a key, aborting" << IBRCOMMON_LOGGER_ENDL;
				return;
			}

			PayloadIntegrityBlock& pib = bundle.push_front<PayloadIntegrityBlock>();
			pib.set(REPLICATE_IN_EVERY_FRAGMENT, true);

			// check if this is a fragment
			if (bundle.get(dtn::data::PrimaryBlock::FRAGMENT))
			{
				dtn::data::PayloadBlock& plb = bundle.getBlock<dtn::data::PayloadBlock>();
				ibrcommon::BLOB::Reference blobref = plb.getBLOB();
				
				ibrcommon::MutexLock ml(blobref);
				addFragmentRange(pib._ciphersuite_params, bundle._fragmentoffset, blobref.operator*());
			}

			// set the source and destination address of the new block
			if (source != bundle._source) pib.setSecuritySource( source );
			if (destination != bundle._destination) pib.setSecurityDestination( destination );

			pib.setKeySize(getSecurityResultSize());
//			pib.setCiphersuiteId(SecurityBlock::PIB_RSA_SHA256);
			pib._ciphersuite_flags |= CONTAINS_SECURITY_RESULT;
			std::string sign = calcHash(bundle, pib);
			pib._security_result.add(SecurityBlock::integrity_signature, sign);
		}

		const std::string PayloadIntegrityBlock::calcHash(const dtn::data::Bundle &bundle, PayloadIntegrityBlock& ignore) const
		{
			IBRCOMMON_LOGGER_DEBUG_ex(ibrcommon::Logger::LOGGER_DEBUG) <<  "entering ..." << IBRCOMMON_LOGGER_ENDL;

			ibrcommon::RSASHA256Stream rs2s(pkey);
			dtn::security::MutualSerializer(rs2s).serialize_mutable(bundle, &ignore);
			int return_code = rs2s.getSign().first;
			std::string sign_string = rs2s.getSign().second;

			if (return_code)
				return sign_string;
			else
			{
				IBRCOMMON_LOGGER_ex(critical) << "an error occured at the creation of the hash and it is invalid" << IBRCOMMON_LOGGER_ENDL;
				ERR_print_errors_fp(stderr);
				return std::string("");
			}
		}

		int PayloadIntegrityBlock::testBlock(const dtn::data::Bundle& bundle, dtn::security::PayloadIntegrityBlock const * sb, const bool use_eid) const
		{
			IBRCOMMON_LOGGER_DEBUG_ex(ibrcommon::Logger::LOGGER_DEBUG) << "checking block" << IBRCOMMON_LOGGER_ENDL;

//			// check if we have the public key of the security source, or this bundle is adressed to us
//			if (use_eid && !((*sb).isSecuritySource(bundle, _partner_node) || (*sb).isSecurityDestination(bundle, _our_id)))
//				return 0;
//			// check the correct algorithm
//			if (sb->_ciphersuite_id != SecurityBlock::PIB_RSA_SHA256)
//				return 0;
//
//			ibrcommon::RSASHA256Stream rs2s(pkey, true);
//			dtn::security::MutualSerializer(rs2s).serialize_mutable(bundle, sb);
//			return rs2s.getVerification(SecurityBlock::getTLVs(sb->_security_result, SecurityBlock::integrity_signature).begin().operator*());
		}

		signed char PayloadIntegrityBlock::verify(const dtn::data::Bundle &bundle) const
		{
			IBRCOMMON_LOGGER_DEBUG_ex(ibrcommon::Logger::LOGGER_DEBUG) << "entering ..." << IBRCOMMON_LOGGER_ENDL;
			IBRCOMMON_LOGGER_DEBUG_ex(ibrcommon::Logger::LOGGER_DEBUG) << "searching our PIB" << IBRCOMMON_LOGGER_ENDL;
			// iterate over all PIBs to find the right one
			std::list<const PayloadIntegrityBlock *> pibs = bundle.getBlocks<PayloadIntegrityBlock>();

			for (std::list<const PayloadIntegrityBlock *>::const_iterator it = pibs.begin(); it!=pibs.end(); it++)
			{
				int return_code = testBlock(bundle, *it);
				// return if we got an error or are successful
				if (return_code)
					return return_code;
			}
			return 0;
		}

		void PayloadIntegrityBlock::setKeySize(int new_size) const
		{
			key_size = new_size;
		}

		bool PayloadIntegrityBlock::verifyAndRemoveTopBlock(dtn::data::Bundle& bundle) const
		{
			PayloadIntegrityBlock& pib = bundle.getBlock<PayloadIntegrityBlock>();
			if (testBlock(bundle, &pib) == 1)
			{
				bundle.remove(pib);
				return true;
			}
			else
				return false;
		}

		int PayloadIntegrityBlock::verifyAndRemoveMatchingBlock(dtn::data::Bundle& bundle) const
		{
			std::list<const PayloadIntegrityBlock *> pibs = bundle.getBlocks<PayloadIntegrityBlock>();
			int deleted_blocks = 0;
			const PayloadIntegrityBlock * valid = NULL;
			// search for valid PIB
			for (std::list<const PayloadIntegrityBlock *>::const_iterator it = pibs.begin(); it != pibs.end() && !valid; it++)
				// return if we got an error or are successful
				if (testBlock(bundle, *it) == 1)
				{
					valid = *it;
					deleted_blocks++;
				}

			// delete all pibs up to and including valid
			for (std::list<const PayloadIntegrityBlock *>::const_iterator it = pibs.begin(); valid && it != pibs.end() && *it != valid; it++)
			{
				deleted_blocks++;
				bundle.remove(*(*it));
			}
			if (valid)
				bundle.remove(*valid);
			return deleted_blocks;
		}

		void PayloadIntegrityBlock::removeAllPayloadIntegrityBlocks(dtn::data::Bundle& bundle) const
		{
			std::list<const PayloadIntegrityBlock *> pibs = bundle.getBlocks<PayloadIntegrityBlock>();
			for (std::list<const PayloadIntegrityBlock *>::const_iterator it = pibs.begin(); it != pibs.end(); it++)
				bundle.remove(*(*it));
		}
	}
}

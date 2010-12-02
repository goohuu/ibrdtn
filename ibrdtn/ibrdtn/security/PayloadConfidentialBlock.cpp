#include "ibrdtn/security/PayloadConfidentialBlock.h"
#include "ibrdtn/security/PayloadIntegrityBlock.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/data/SDNV.h"

#include <openssl/err.h>
#include <openssl/rsa.h>
#include <ibrcommon/thread/MutexLock.h>
#include <ibrcommon/ssl/AES128Stream.h>
#include <ibrcommon/Logger.h>

#ifdef __DEVELOPMENT_ASSERTIONS__
#include <cassert>
#endif

namespace dtn
{
	namespace security
	{
		dtn::data::Block* PayloadConfidentialBlock::Factory::create()
		{
			return new PayloadConfidentialBlock();
		}

		PayloadConfidentialBlock::PayloadConfidentialBlock()
		: SecurityBlock(PAYLOAD_CONFIDENTIAL_BLOCK, PCB_RSA_AES128_PAYLOAD_PIB_PCB)
		{
		}

		PayloadConfidentialBlock::PayloadConfidentialBlock(dtn::data::Bundle& bundle)
		: SecurityBlock(PAYLOAD_CONFIDENTIAL_BLOCK, PCB_RSA_AES128_PAYLOAD_PIB_PCB)
		{
		}

		PayloadConfidentialBlock::PayloadConfidentialBlock(RSA* long_key, const dtn::data::EID& we, const dtn::data::EID& partner)
		: SecurityBlock(PAYLOAD_CONFIDENTIAL_BLOCK, we, partner)
		{
			addDestination(long_key, partner);
		}

		PayloadConfidentialBlock::~PayloadConfidentialBlock()
		{
		}

		void PayloadConfidentialBlock::addDestination(RSA* long_key, const dtn::data::EID& partner)
		{
			_node_keys.insert(std::pair<const dtn::data::EID, RSA*>(partner.getNodeEID(), long_key));
		}

		void PayloadConfidentialBlock::encrypt(dtn::data::Bundle& bundle) const
		{
			// get all PCBs
			const std::list<PayloadConfidentialBlock const *> pcbs = bundle.getBlocks<PayloadConfidentialBlock>();

			// get all PIBs
			const std::list<PayloadIntegrityBlock const *> pibs = bundle.getBlocks<PayloadIntegrityBlock>();

			// generate key and salt
			u_int32_t salt;
			unsigned char ephemeral_key[ibrcommon::AES128Stream::key_size_in_bytes];
			createSaltAndKey(salt, ephemeral_key, ibrcommon::AES128Stream::key_size_in_bytes);

			// encrypt payload
			encryptPayload(bundle, ephemeral_key, salt);
			
			// this only works if we have one key for encryption and not more, because
			// with mutiple destinations the blocks cannot be correlated
			if (_node_keys.size() == 1)
			{
				dtn::security::PayloadConfidentialBlock& pcb = bundle.getBlock<dtn::security::PayloadConfidentialBlock>();

				// create correlator
				u_int64_t correlator = createCorrelatorValue(bundle);
			
				if (pcbs.size() > 0 || pibs.size() > 0)
					pcb.setCorrelator(correlator);

				// encrypt PCBs and PIBs
				for (std::list<PayloadConfidentialBlock const *>::const_iterator it = pcbs.begin(); it != pcbs.end(); it++)
					SecurityBlock::encryptBlock<PayloadConfidentialBlock>(bundle, *it, salt, ephemeral_key).setCorrelator(correlator);

				for (std::list<PayloadIntegrityBlock const *>::const_iterator it = pibs.begin(); it != pibs.end(); it++)
					SecurityBlock::encryptBlock<PayloadConfidentialBlock>(bundle, *it, salt, ephemeral_key).setCorrelator(correlator);
			}
		}

		bool PayloadConfidentialBlock::decrypt(dtn::data::Bundle& bundle) const
		{
			// get all pibs from the bundle, they will be invalid, if the decryption 
			// is successfull. so they need to be removed
			std::list<PayloadIntegrityBlock const *> pibs = bundle.getBlocks<PayloadIntegrityBlock>();
			
			// get all PCBs
			std::list<PayloadConfidentialBlock const *> pcbs = bundle.getBlocks<PayloadConfidentialBlock>();

			// look after the first PCB for us
			std::list<PayloadConfidentialBlock const *>::iterator it = pcbs.begin();
			for (; it != pcbs.end(); it++) {
				bool isdest = SecurityBlock::isSecurityDestination(bundle, **it, _our_id);
				bool isciphid = (*it)->_ciphersuite_id == SecurityBlock::PCB_RSA_AES128_PAYLOAD_PIB_PCB;
				if (isdest && isciphid)
					break;
			}

			if (it == pcbs.end())
			{
				IBRCOMMON_LOGGER_ex(critical) << "no PayloadConfidentialBlock for us found" << IBRCOMMON_LOGGER_ENDL;
				return false;
			}

			// get salt and key
			u_int32_t salt = getSalt((*it)->_ciphersuite_params);
			unsigned char key[ibrcommon::AES128Stream::key_size_in_bytes];
			if (!getKey((*it)->_ciphersuite_params, key, ibrcommon::AES128Stream::key_size_in_bytes, _node_keys.begin()->second))
			{
				IBRCOMMON_LOGGER_ex(critical) << "could not get symmetric key decrypted" << IBRCOMMON_LOGGER_ENDL;
				return false;
			}

			if (!decryptPayload(bundle, key, salt))
			{
				// reverse decryption
				IBRCOMMON_LOGGER_ex(critical) << "tag verfication failed, reversing decryption..." << IBRCOMMON_LOGGER_ENDL;
				decryptPayload(bundle, key, salt);
				return false;
			}

			PayloadConfidentialBlock const * first_pcb = *it;
			// check if first PCB has a correlator and decrypt all correlated blocks
			if (first_pcb->_ciphersuite_flags & CONTAINS_CORRELATOR)
			{
				it++;
				// get correlated blocks
				for (; it != pcbs.end(); it++)
					if ((**it)._ciphersuite_flags & CONTAINS_CORRELATOR && (**it)._correlator == first_pcb->_correlator && !decryptBlock(bundle, *it, salt, key))
							IBRCOMMON_LOGGER_ex(critical) << "tag verfication failed, when no matching key is found the block will be deleted" << IBRCOMMON_LOGGER_ENDL;
				bundle.remove(*first_pcb);
			}
			else
			{
				// delete all pcbs which might carry key information for other destinations
				std::list<PayloadConfidentialBlock const *> pcbs = bundle.getBlocks<PayloadConfidentialBlock>();
				for (it = pcbs.begin(); it != pcbs.end(); it++)
					bundle.remove(**it);
			}
			
			// delete all now invalid pibs
			for (std::list <const dtn::security::PayloadIntegrityBlock* >::iterator pit = pibs.begin(); pit != pibs.end(); pit++)
				bundle.remove(**pit);
			
			return true;
		}

		void PayloadConfidentialBlock::encryptPayload(dtn::data::Bundle& bundle, const unsigned char ephemeral_key[ibrcommon::AES128Stream::key_size_in_bytes], const u_int32_t salt) const
		{
			dtn::data::PayloadBlock& plb = bundle.getBlock<dtn::data::PayloadBlock>();
			ibrcommon::BLOB::Reference blobref = plb.getBLOB();
			ibrcommon::MutexLock ml(blobref);

			// encrypt in place
			// no clear() or pointer resetting needed, it is done in MutexLock
// 			blobref.operator*().clear(); blobref.operator*().seekg(0); blobref.operator*().seekp(0);
			ibrcommon::AES128Stream encrypt(ibrcommon::AES128Stream::ENCRYPT, blobref.operator*(), ephemeral_key, salt);
			encrypt << blobref.operator*().rdbuf() << std::flush;
			
			for (std::map<dtn::data::EID, RSA *>::const_iterator it = _node_keys.begin(); it != _node_keys.end(); it++)
			{
				PayloadConfidentialBlock& pcb = bundle.push_front<PayloadConfidentialBlock>();
				setSourceAndDestination(bundle, pcb, &(it->first));

				pcb.set(REPLICATE_IN_EVERY_FRAGMENT, true);
				// check if this is a fragment
				if (bundle.get(dtn::data::PrimaryBlock::FRAGMENT))
					addFragmentRange(pcb._ciphersuite_params, bundle._fragmentoffset, blobref.operator*());

				// store encypted key, tag, iv and salt
				addSalt(pcb._ciphersuite_params, salt);
				addKey(pcb._ciphersuite_params, ephemeral_key, ibrcommon::AES128Stream::key_size_in_bytes, it->second);
				SecurityBlock::addTLV(pcb._ciphersuite_params, SecurityBlock::initialization_vector, ibrcommon::AES128Stream::iv_len, reinterpret_cast<char const *>(encrypt.getIV()));
				SecurityBlock::addTLV(pcb._security_result, SecurityBlock::integrity_signature, ibrcommon::AES128Stream::tag_len, reinterpret_cast<char const *>(encrypt.getTag()));
				pcb._ciphersuite_flags |= SecurityBlock::CONTAINS_CIPHERSUITE_PARAMS;
				pcb._ciphersuite_flags |= SecurityBlock::CONTAINS_SECURITY_RESULT;
			}
		}

		bool PayloadConfidentialBlock::decryptPayload(dtn::data::Bundle& bundle, const unsigned char ephemeral_key[ibrcommon::AES128Stream::key_size_in_bytes], const u_int32_t salt) const
		{
			// TODO handle fragmentation
			PayloadConfidentialBlock& pcb = bundle.getBlock<PayloadConfidentialBlock>();
			dtn::data::PayloadBlock& plb = bundle.getBlock<dtn::data::PayloadBlock>();

			std::string iv_string = SecurityBlock::getTLVs(pcb._ciphersuite_params, SecurityBlock::initialization_vector).begin().operator*();
#ifdef __DEVELOPMENT_ASSERTIONS__
			assert(iv_string.size() == ibrcommon::AES128Stream::iv_len);
#endif
			unsigned char const * iv = reinterpret_cast<unsigned char const *>(iv_string.c_str());

			std::string tag_string = SecurityBlock::getTLVs(pcb._security_result, SecurityBlock::integrity_signature).begin().operator*();
#ifdef __DEVELOPMENT_ASSERTIONS__
			assert(tag_string.size() == ibrcommon::AES128Stream::tag_len);
#endif
			unsigned char const * tag = reinterpret_cast<unsigned char const *>(tag_string.c_str());

			ibrcommon::BLOB::Reference blobref = plb.getBLOB();
			ibrcommon::MutexLock ml(blobref);

			ibrcommon::AES128Stream decrypt(ibrcommon::AES128Stream::DECRYPT, *blobref, ephemeral_key, salt, iv, tag);
			decrypt << (*blobref).rdbuf() << std::flush;

			return decrypt.isTagGood();
		}
	}
}

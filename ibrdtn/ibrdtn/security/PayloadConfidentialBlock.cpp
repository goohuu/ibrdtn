#include "ibrdtn/security/PayloadConfidentialBlock.h"
#include "ibrdtn/security/PayloadIntegrityBlock.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/data/SDNV.h"

#include <openssl/err.h>
#include <openssl/rsa.h>
#include <ibrcommon/thread/MutexLock.h>
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

		PayloadConfidentialBlock::~PayloadConfidentialBlock()
		{
		}

		void PayloadConfidentialBlock::encrypt(dtn::data::Bundle& bundle, const dtn::security::SecurityKey &long_key, const dtn::data::EID& source)
		{
			// contains the random salt
			u_int32_t salt;

			// contains the random key
			unsigned char ephemeral_key[ibrcommon::AES128Stream::key_size_in_bytes];

			unsigned char iv[ibrcommon::AES128Stream::iv_len];
			unsigned char tag[ibrcommon::AES128Stream::tag_len];

			// get all PCBs
			const std::list<const PayloadConfidentialBlock* > pcbs = bundle.getBlocks<PayloadConfidentialBlock>();

			// get all PIBs
			const std::list<const PayloadIntegrityBlock* > pibs = bundle.getBlocks<PayloadIntegrityBlock>();

			// create a new payload confidential block
			PayloadConfidentialBlock& pcb = bundle.push_front<PayloadConfidentialBlock>();

			// create a random salt and key
			createSaltAndKey(salt, ephemeral_key, ibrcommon::AES128Stream::key_size_in_bytes);

			// encrypt payload - BEGIN
			dtn::data::PayloadBlock& plb = bundle.getBlock<dtn::data::PayloadBlock>();
			ibrcommon::BLOB::Reference blobref = plb.getBLOB();

			{
				ibrcommon::BLOB::iostream stream = blobref.iostream();
				ibrcommon::AES128Stream aes_stream(ibrcommon::CipherStream::CIPHER_ENCRYPT, *stream, ephemeral_key, salt);

				aes_stream.getIV(iv);
				aes_stream.getTag(tag);

				// encrypt in place
				((ibrcommon::CipherStream&)aes_stream).encrypt(*stream);

				// check if this is a fragment
				if (bundle.get(dtn::data::PrimaryBlock::FRAGMENT))
				{
					// ... and set the corresponding cipher suit params
					addFragmentRange(pcb._ciphersuite_params, bundle._fragmentoffset, stream.size());
				}
			}
			// encrypt payload - END

			// set the source and destination address of the new block
			if (source != bundle._source.getNodeEID()) pcb.setSecuritySource( source );
			if (long_key.reference != bundle._destination.getNodeEID()) pcb.setSecurityDestination( long_key.reference );

			// set replicate in every fragment to true
			pcb.set(REPLICATE_IN_EVERY_FRAGMENT, true);

			// store encypted key, tag, iv and salt
			addSalt(pcb._ciphersuite_params, salt);

			// get the RSA key
			RSA *rsa_key = long_key.getRSA();

			// encrypt the random key and add it to the ciphersuite params
			addKey(pcb._ciphersuite_params, ephemeral_key, ibrcommon::AES128Stream::key_size_in_bytes, rsa_key);

			// free the RSA key
			long_key.free(rsa_key);

			pcb._ciphersuite_params.set(SecurityBlock::initialization_vector, std::string((const char*)&iv, ibrcommon::AES128Stream::iv_len));
			pcb._ciphersuite_flags |= SecurityBlock::CONTAINS_CIPHERSUITE_PARAMS;

			pcb._security_result.set(SecurityBlock::integrity_signature, std::string((const char*)&tag, ibrcommon::AES128Stream::tag_len));
			pcb._ciphersuite_flags |= SecurityBlock::CONTAINS_SECURITY_RESULT;

			// create correlator
			u_int64_t correlator = createCorrelatorValue(bundle);

			if (pcbs.size() > 0 || pibs.size() > 0)
				pcb.setCorrelator(correlator);

			// encrypt PCBs and PIBs
			for (std::list<const PayloadConfidentialBlock*>::const_iterator it = pcbs.begin(); it != pcbs.end(); it++)
				SecurityBlock::encryptBlock<PayloadConfidentialBlock>(bundle, (dtn::data::Block&)**it, salt, ephemeral_key).setCorrelator(correlator);

			for (std::list<const PayloadIntegrityBlock*>::const_iterator it = pibs.begin(); it != pibs.end(); it++)
				SecurityBlock::encryptBlock<PayloadConfidentialBlock>(bundle, (dtn::data::Block&)**it, salt, ephemeral_key).setCorrelator(correlator);
		}

		void PayloadConfidentialBlock::decrypt(dtn::data::Bundle& bundle, const dtn::security::SecurityKey &long_key)
		{
			// get all pibs from the bundle, they will be invalid, if the decryption 
			// is successfull. so they need to be removed
			const std::list<const PayloadIntegrityBlock* > pibs = bundle.getBlocks<PayloadIntegrityBlock>();
			
			// get the payload confidential blocks
			const std::list<const PayloadConfidentialBlock* > pcbs = bundle.getBlocks<PayloadConfidentialBlock>();

			// iterate through all pcb
			for (std::list<const PayloadConfidentialBlock* >::const_iterator it = pcbs.begin(); it != pcbs.end(); it++)
			{
				const PayloadConfidentialBlock &pcb = dynamic_cast<const PayloadConfidentialBlock&>(**it);

				// if security destination does not match local, then fail
				if (pcb.isSecurityDestination(bundle, long_key.reference) &&
					(pcb._ciphersuite_id == SecurityBlock::PCB_RSA_AES128_PAYLOAD_PIB_PCB))
				{
					// decrypt the block

					// get salt and key
					u_int32_t salt = getSalt(pcb._ciphersuite_params);
					unsigned char key[ibrcommon::AES128Stream::key_size_in_bytes];

					RSA *rsa_key = long_key.getRSA();
					if (!getKey(pcb._ciphersuite_params, key, ibrcommon::AES128Stream::key_size_in_bytes, rsa_key))
					{
						long_key.free(rsa_key);
						IBRCOMMON_LOGGER(critical) << "could not get symmetric key decrypted" << IBRCOMMON_LOGGER_ENDL;
						throw ibrcommon::Exception("decrypt failed - could not get symmetric key decrypted");
					}
					long_key.free(rsa_key);

					if (!decryptPayload(bundle, key, salt))
					{
						// reverse decryption
						IBRCOMMON_LOGGER(critical) << "tag verfication failed, reversing decryption..." << IBRCOMMON_LOGGER_ENDL;
						decryptPayload(bundle, key, salt);
						throw ibrcommon::Exception("decrypt reversed - tag verfication failed");
					}

					// check if first PCB has a correlator and decrypt all correlated blocks
					if (pcb._ciphersuite_flags & CONTAINS_CORRELATOR)
					{
						// move the iterator to the next pcb
						it++;

						// get correlated blocks
						for (; it != pcbs.end(); it++)
						{
							// if this block is correlated to the origin pcb
							if ((**it)._ciphersuite_flags & CONTAINS_CORRELATOR && (**it)._correlator == pcb._correlator)
							{
								// try to decrypt the block
								try {
									decryptBlock(bundle, (dtn::security::SecurityBlock&)**it, salt, key);
								} catch (const ibrcommon::Exception&) {
									IBRCOMMON_LOGGER_ex(critical) << "tag verfication failed, when no matching key is found the block will be deleted" << IBRCOMMON_LOGGER_ENDL;

									// abort the decryption and discard the bundle?
								}
							}
						}

						// remove the block
						bundle.remove(pcb);
					}
					else
					{
						// delete all pcbs which might carry key information for other destinations
						std::list<PayloadConfidentialBlock const *> pcbs = bundle.getBlocks<PayloadConfidentialBlock>();
						for (it = pcbs.begin(); it != pcbs.end(); it++)
							bundle.remove(**it);
					}

					// delete all now invalid pibs
					for (std::list <const dtn::security::PayloadIntegrityBlock* >::const_iterator pit = pibs.begin(); pit != pibs.end(); pit++)
					{
						bundle.remove(**pit);
					}
				}
				else
				{
					// abort here - we've done all we can do
					return;
				}
			}
		}

		bool PayloadConfidentialBlock::decryptPayload(dtn::data::Bundle& bundle, const unsigned char ephemeral_key[ibrcommon::AES128Stream::key_size_in_bytes], const u_int32_t salt)
		{
			// TODO handle fragmentation
			PayloadConfidentialBlock& pcb = bundle.getBlock<PayloadConfidentialBlock>();
			dtn::data::PayloadBlock& plb = bundle.getBlock<dtn::data::PayloadBlock>();

			std::string iv_string = pcb._ciphersuite_params.get(SecurityBlock::initialization_vector);

			if (iv_string.size() == ibrcommon::AES128Stream::iv_len)
			{
				IBRCOMMON_LOGGER(error) << "initialization vector has the wrong size" << IBRCOMMON_LOGGER_ENDL;
				return false;
			}

			unsigned char const * iv = reinterpret_cast<unsigned char const *>(iv_string.c_str());

			std::string tag_string = pcb._security_result.get(SecurityBlock::integrity_signature);

			if (tag_string.size() == ibrcommon::AES128Stream::tag_len)
			{
				IBRCOMMON_LOGGER(error) << "integrity signature has the wrong size" << IBRCOMMON_LOGGER_ENDL;
				return false;
			}

			unsigned char const * tag = reinterpret_cast<unsigned char const *>(tag_string.c_str());

			// array for the integrity signature (tag)
			unsigned char decrypt_tag[ibrcommon::AES128Stream::tag_len];

			// get the reference to the corresponding BLOB object
			ibrcommon::BLOB::Reference blobref = plb.getBLOB();

			// decrypt the payload and get the integrity signature (tag)
			{
				ibrcommon::BLOB::iostream stream = blobref.iostream();
				ibrcommon::AES128Stream decrypt(ibrcommon::CipherStream::CIPHER_DECRYPT, *stream, ephemeral_key, salt, iv);
				((ibrcommon::CipherStream&)decrypt).decrypt(*stream);

				// get the decrypt tag
				decrypt.getTag(decrypt_tag);
			}

			if (memcmp(decrypt_tag, tag, ibrcommon::AES128Stream::tag_len) != 0)
			{
				IBRCOMMON_LOGGER(error) << "integrity signature of the decrypted payload is invalid" << IBRCOMMON_LOGGER_ENDL;
				return false;
			}

			return true;
		}
	}
}

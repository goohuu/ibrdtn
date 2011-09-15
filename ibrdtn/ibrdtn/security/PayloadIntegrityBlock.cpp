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
		 : SecurityBlock(PAYLOAD_INTEGRITY_BLOCK, PIB_RSA_SHA256), result_size(0)
		{
		}

		PayloadIntegrityBlock::~PayloadIntegrityBlock()
		{
		}

		size_t PayloadIntegrityBlock::getSecurityResultSize() const
		{
			if (result_size > 0)
			{
				return result_size;
			}

			return SecurityBlock::getSecurityResultSize();
		}

		void PayloadIntegrityBlock::sign(dtn::data::Bundle &bundle, const SecurityKey &key, const dtn::data::EID& destination)
		{
			PayloadIntegrityBlock& pib = bundle.push_front<PayloadIntegrityBlock>();
			pib.set(REPLICATE_IN_EVERY_FRAGMENT, true);

			// check if this is a fragment
			if (bundle.get(dtn::data::PrimaryBlock::FRAGMENT))
			{
				dtn::data::PayloadBlock& plb = bundle.getBlock<dtn::data::PayloadBlock>();
				ibrcommon::BLOB::Reference blobref = plb.getBLOB();
				ibrcommon::BLOB::iostream stream = blobref.iostream();
				addFragmentRange(pib._ciphersuite_params, bundle._fragmentoffset, stream.size());
			}

			// set the source and destination address of the new block
			if (key.reference != bundle._source.getNode()) pib.setSecuritySource( key.reference );
			if (destination != bundle._destination.getNode()) pib.setSecurityDestination( destination );

			pib.setResultSize(key);
			pib.setCiphersuiteId(SecurityBlock::PIB_RSA_SHA256);
			pib._ciphersuite_flags |= CONTAINS_SECURITY_RESULT;
			std::string sign = calcHash(bundle, key, pib);
			pib._security_result.set(SecurityBlock::integrity_signature, sign);
		}

		const std::string PayloadIntegrityBlock::calcHash(const dtn::data::Bundle &bundle, const SecurityKey &key, PayloadIntegrityBlock& ignore)
		{
			EVP_PKEY *pkey = key.getEVP();
			ibrcommon::RSASHA256Stream rs2s(pkey);

			// serialize the bundle in the mutable form
			dtn::security::MutualSerializer ms(rs2s, &ignore);
			(dtn::data::DefaultSerializer&)ms << bundle; rs2s << std::flush;

			int return_code = rs2s.getSign().first;
			std::string sign_string = rs2s.getSign().second;
			SecurityKey::free(pkey);

			if (return_code)
				return sign_string;
			else
			{
				IBRCOMMON_LOGGER_ex(critical) << "an error occured at the creation of the hash and it is invalid" << IBRCOMMON_LOGGER_ENDL;
				ERR_print_errors_fp(stderr);
				return std::string("");
			}
		}

		void PayloadIntegrityBlock::verify(const dtn::data::Bundle& bundle, const SecurityKey &key, const PayloadIntegrityBlock &sb, const bool use_eid)
		{
			// check if we have the public key of the security source
			if (use_eid)
			{
				if (!sb.isSecuritySource(bundle, key.reference))
				{
					throw ibrcommon::Exception("key not match the security source");
				}
			}

			// check the correct algorithm
			if (sb._ciphersuite_id != SecurityBlock::PIB_RSA_SHA256)
			{
				throw ibrcommon::Exception("can not verify the PIB because of an invalid algorithm");
			}

			EVP_PKEY *pkey = key.getEVP();
			if (pkey == NULL) throw ibrcommon::Exception("verification error");

			ibrcommon::RSASHA256Stream rs2s(pkey, true);

			// serialize the bundle in the mutable form
			dtn::security::MutualSerializer ms(rs2s, &sb);
			(dtn::data::DefaultSerializer&)ms << bundle; rs2s << std::flush;

			int ret = rs2s.getVerification(sb._security_result.get(SecurityBlock::integrity_signature));
			SecurityKey::free(pkey);

			if (ret == 0)
			{
				throw ibrcommon::Exception("verification failed");
			}
			else if (ret < 0)
			{
				throw ibrcommon::Exception("verification error");
			}
		}

		void PayloadIntegrityBlock::verify(const dtn::data::Bundle &bundle, const SecurityKey &key)
		{
			// iterate over all PIBs to find the right one
			std::list<const PayloadIntegrityBlock *> pibs = bundle.getBlocks<PayloadIntegrityBlock>();

			for (std::list<const PayloadIntegrityBlock *>::const_iterator it = pibs.begin(); it!=pibs.end(); it++)
			{
				verify(bundle, key, **it);
			}
		}

		void PayloadIntegrityBlock::setResultSize(const SecurityKey &key)
		{
			EVP_PKEY *pkey = key.getEVP();

			// size of integrity_signature
			if ((result_size = EVP_PKEY_size(pkey)) > 0)
			{
				// sdnv length
				result_size += dtn::data::SDNV(result_size).getLength();

				// type
				result_size++;
			}
			else
			{
				result_size = _security_result.getLength();
			}

			SecurityKey::free(pkey);
		}

		void PayloadIntegrityBlock::strip(dtn::data::Bundle& bundle, const SecurityKey &key, const bool all)
		{
			std::list<const PayloadIntegrityBlock *> pibs = bundle.getBlocks<PayloadIntegrityBlock>();
			const PayloadIntegrityBlock * valid = NULL;

			// search for valid PIB
			for (std::list<const PayloadIntegrityBlock *>::const_iterator it = pibs.begin(); it != pibs.end() && !valid; it++)
			{
				const PayloadIntegrityBlock &pib = (**it);

				// check if the PIB is valid
				try {
					verify(bundle, key, pib);

					// found an valid PIB, remove it
					bundle.remove(pib);

					// remove all previous pibs if all = true
					if (all && (it != pibs.begin()))
					{
						// move the iterator one backward
						for (it--; it != pibs.begin(); it--)
						{
							bundle.remove(**it);
						}

						// remove the first PIB too
						bundle.remove(**it);
					}

					return;
				} catch (const ibrcommon::Exception&) { };
			}
		}

		void PayloadIntegrityBlock::strip(dtn::data::Bundle& bundle)
		{
			std::list<const PayloadIntegrityBlock *> pibs = bundle.getBlocks<PayloadIntegrityBlock>();
			for (std::list<const PayloadIntegrityBlock *>::const_iterator it = pibs.begin(); it != pibs.end(); it++)
			{
				bundle.remove(*(*it));
			}
		}

		std::istream& PayloadIntegrityBlock::deserialize(std::istream &stream, const size_t length)
		{
			// deserialize the SecurityBlock
			SecurityBlock::deserialize(stream, length);

			// set the key size locally
			result_size = _security_result.getLength();

			return stream;
		}
	}
}

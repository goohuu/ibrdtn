#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/security/BundleAuthenticationBlock.h"
#include "ibrdtn/security/StrictSerializer.h"
#include "ibrcommon/ssl/HMacStream.h"
#include <ibrcommon/Logger.h>
#include <cstring>

#ifdef __DEVELOPMENT_ASSERTIONS__
#include <cassert>
#endif

namespace dtn
{
	namespace security
	{
		dtn::data::Block* BundleAuthenticationBlock::Factory::create()
		{
			return new BundleAuthenticationBlock();
		}

		BundleAuthenticationBlock::BundleAuthenticationBlock()
		 : SecurityBlock(BUNDLE_AUTHENTICATION_BLOCK, BAB_HMAC), _keys(), _partners()
		{
		}

		BundleAuthenticationBlock::BundleAuthenticationBlock(const unsigned char * const hkey, const size_t size)
		 : SecurityBlock(BUNDLE_AUTHENTICATION_BLOCK), _keys(), _partners()
		{
//			addKey(hkey, size, partner);
		}

		BundleAuthenticationBlock::BundleAuthenticationBlock(const list< std::pair<const unsigned char * const, const size_t> > keys)
		 : SecurityBlock(BUNDLE_AUTHENTICATION_BLOCK), _keys(), _partners()
		{
#ifdef __DEVELOPMENT_ASSERTIONS__
			assert(keys.size() == partners.size());
#endif
//			std::list<dtn::data::EID>::const_iterator e_it = partners.begin();
//			for (std::list< std::pair<const unsigned char * const, const size_t> >::const_iterator it = keys.begin(); it != keys.end(); it++, e_it++)
//				addKey(it->first, it->second, e_it->getNodeEID());
		}

		BundleAuthenticationBlock::~BundleAuthenticationBlock()
		{
			for (std::list< std::pair<const unsigned char * const, const size_t> >::const_iterator it = _keys.begin(); it != _keys.end(); it++)
				delete [] it->first;
		}

		void BundleAuthenticationBlock::addKey(const unsigned char * const hkey, const size_t size, const dtn::data::EID partner)
		{
			unsigned char * key = new unsigned char[size];
			for (unsigned int i = 0; i < size; i++)
				key[i] = hkey[i];
// 			std::pair<const unsigned char *, const size_t> key_size(key, size);
// 			_keys.push_back(key_size);
				_keys.push_back(std::pair<const unsigned char *, const size_t>(key, size));

// 			dtn::data::EID * partner_ptr = new dtn::data::EID(partner);
// 			_partners.push_back(*partner_ptr);
			_partners.push_back(partner.getNodeEID());
		}

		void BundleAuthenticationBlock::addMAC(dtn::data::Bundle& bundle) const
		{
			// adding all bab pairs to the bundle
			std::list<BundleAuthenticationBlock *> bab_ends;
			for (std::list<dtn::data::EID>::const_iterator it = _partners.begin(); it != _partners.end(); it++)
			{
				BundleAuthenticationBlock& bab_begin = bundle.push_front<BundleAuthenticationBlock>();
				BundleAuthenticationBlock& bab_end = bundle.push_back<BundleAuthenticationBlock>();

				// set source and destination
//				setSourceAndDestination(bundle, bab_begin, &(*it));

				u_int64_t correlator = createCorrelatorValue(bundle);
				bab_ends.push_back(&bab_end);
				bab_begin.setCorrelator(correlator);
// 				bab_begin.setCiphersuiteId(BAB_HMAC);
				bab_end.setCorrelator(correlator);
				bab_end._ciphersuite_flags |= CONTAINS_SECURITY_RESULT;
			}

			// calculating each hash
			std::list<std::pair<const unsigned char * const, const size_t> >::const_iterator it_keys = _keys.begin();
			for (std::list<BundleAuthenticationBlock *>::iterator it = bab_ends.begin(); it != bab_ends.end(); it++, it_keys++)
			{
				std::string sizehash_hash = calcMAC(bundle, it_keys->first, it_keys->second);
				SecurityBlock::addTLV((*it)->_security_result, SecurityBlock::integrity_signature, sizehash_hash.size(), sizehash_hash.c_str());
			}
		}

		signed char BundleAuthenticationBlock::verify(const dtn::data::Bundle& bundle) const
		{
			std::list<dtn::data::EID>::const_iterator ids = _partners.begin();
			for (std::list<std::pair<const unsigned char * const, const size_t> >::const_iterator it_keys = _keys.begin(); it_keys != _keys.end(); it_keys++, ids++)
				if (verify(bundle, *it_keys, *ids).first)
					return 1;

			return 0;
		}

		int BundleAuthenticationBlock::verifyAndRemoveMatchingBlocks(dtn::data::Bundle& bundle) const
		{
			// collect matching correlators, needs to be done first, because removing
			// the blocks would invalidate other babs
			std::list<u_int64_t> correlators;
			std::list<dtn::data::EID>::const_iterator it_partner = _partners.begin();
			for (std::list<std::pair<const unsigned char * const, const size_t> >::const_iterator it_keys = _keys.begin(); it_keys != _keys.end(); it_keys++, it_partner++)
			{
				std::pair<bool, u_int64_t> result = verify(bundle, *it_keys, *it_partner);
				if (result.first)
					correlators.push_back(result.second);
			}

			int unremoved_blocks = 0;
			std::list<const BundleAuthenticationBlock *> babs = bundle.getBlocks<BundleAuthenticationBlock>();
			for (std::list<const BundleAuthenticationBlock *>::const_iterator it = babs.begin(); it != babs.end(); it++)
			{
				bool removed = false;
				for (std::list<u_int64_t>::iterator cor_it = correlators.begin(); cor_it != correlators.end(); cor_it++)
					if ((*it)->_ciphersuite_flags & SecurityBlock::CONTAINS_CORRELATOR && (*it)->_correlator == *cor_it)
					{
						removed = true;
						bundle.remove(*(*it));
					}
				if (!removed)
					unremoved_blocks++;
			}
			return unremoved_blocks;
		}

		void BundleAuthenticationBlock::removeAllBundleAuthenticationBlocks(dtn::data::Bundle& bundle)
		{
			// blocks of a certain type
			std::list<const BundleAuthenticationBlock *> babs = bundle.getBlocks<BundleAuthenticationBlock>();
			for (std::list<const BundleAuthenticationBlock *>::const_iterator it = babs.begin(); it != babs.end(); it++)
				bundle.remove(*(*it));
		}

		std::pair<bool,u_int64_t> BundleAuthenticationBlock::verify(const dtn::data::Bundle& bundle, std::pair<const unsigned char * const, const size_t> key, const dtn::data::EID& partner) const
		{
			std::list<const BundleAuthenticationBlock *> babs = bundle.getBlocks<BundleAuthenticationBlock>();

			// get the blocks, with which the key should match
			std::list<u_int64_t> good_correlators;
			for (std::list<const BundleAuthenticationBlock *>::const_iterator it = babs.begin(); it != babs.end() && !((*it)->_ciphersuite_flags & CONTAINS_SECURITY_RESULT); it++)
				if ((**it).isSecuritySource(bundle, partner) && (*it)->_ciphersuite_flags & CONTAINS_CORRELATOR && (*it)->_ciphersuite_id == SecurityBlock::BAB_HMAC)
					good_correlators.push_back((**it)._correlator);

			std::string our_hash_string = calcMAC(bundle, key.first, key.second);
			for (std::list<const BundleAuthenticationBlock *>::const_iterator it = babs.begin(); it != babs.end(); it++)
			{
				if (!((*it)->_ciphersuite_flags & CONTAINS_SECURITY_RESULT) || !((*it)->_ciphersuite_flags & CONTAINS_CORRELATOR))
					continue;

				bool result = false;
				for (std::list<u_int64_t>::const_iterator cit = good_correlators.begin(); cit != good_correlators.end() && !result; cit++)
					result = *cit == (*it)->_correlator;
				if (!result)
					continue;

				std::string their_hash = SecurityBlock::getTLVs( (*it)->_security_result, SecurityBlock::integrity_signature).begin().operator*();
				if (our_hash_string.compare(their_hash) == 0) {
					// hash matched
					return std::pair<bool, u_int64_t>(true, (*it)->_correlator);
				}
			}
			return std::pair<bool, u_int64_t>(false, 0);
		}

		std::string BundleAuthenticationBlock::calcMAC(const dtn::data::Bundle& bundle, const unsigned char * const key, const size_t key_size, const bool with_correlator, const u_int64_t correlator) const
		{
			IBRCOMMON_LOGGER_DEBUG_ex(ibrcommon::Logger::LOGGER_DEBUG) << "Generating hash" << IBRCOMMON_LOGGER_ENDL;

			ibrcommon::HMacStream hms(key, key_size);
			dtn::security::StrictSerializer(hms).serialize_strict(bundle, BUNDLE_AUTHENTICATION_BLOCK, with_correlator, correlator);

			return ibrcommon::HashStream::extract(hms);
		}

		size_t BundleAuthenticationBlock::getSecurityResultSize() const
		{
			// TLV type
			size_t size = 1;
			// length of value length
			size += dtn::data::SDNV::getLength(EVP_MD_size(EVP_sha1()));
			// length of value
			size += EVP_MD_size(EVP_sha1());
			return size;
		}

	}
}

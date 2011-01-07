/*
 * SecurityBlock.cpp
 *
 *  Created on: 08.03.2010
 *      Author: morgenro
 */

#include "ibrdtn/security/SecurityBlock.h"
#include "ibrdtn/security/MutualSerializer.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/data/PayloadBlock.h"
#include "ibrdtn/data/ExtensionBlock.h"

#include <ibrcommon/Logger.h>
#include <cstdlib>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/rsa.h>

#ifdef __DEVELOPMENT_ASSERTIONS__
#include <cassert>
#endif

namespace dtn
{
	namespace security
	{
		SecurityBlock::SecurityBlock(const dtn::security::SecurityBlock::BLOCK_TYPES type, const dtn::security::SecurityBlock::CIPHERSUITE_IDS id)
		: Block(type), _ciphersuite_id(id), _ciphersuite_flags(0), _correlator(0), ignore_security_result(false)
		{

		}

		SecurityBlock::SecurityBlock(const dtn::security::SecurityBlock::BLOCK_TYPES type)
		: Block(type), _ciphersuite_flags(0), _correlator(0), ignore_security_result(false)
		{

		}

		SecurityBlock::~SecurityBlock()
		{
		}

		const dtn::data::EID SecurityBlock::getSecuritySource() const
		{
			if (_ciphersuite_flags & SecurityBlock::CONTAINS_SECURITY_SOURCE)
			{
				return _eids.front();
			}
			else
			{
				return dtn::data::EID();
			}
		}

		const dtn::data::EID SecurityBlock::getSecurityDestination() const
		{
			if (_ciphersuite_flags & SecurityBlock::CONTAINS_SECURITY_DESTINATION)
			{
				if (_ciphersuite_flags & SecurityBlock::CONTAINS_SECURITY_SOURCE)
				{
					size_t eidcount = _eids.size();
					return *(++_eids.begin());
				}
				else
				{
					return _eids.front();
				}
			}
			else
			{
				return dtn::data::EID();
			}
		}

		void SecurityBlock::setSecuritySource(const dtn::data::EID &source)
		{
			// the first EID must be the source and the second the destination
			if (_ciphersuite_flags & SecurityBlock::CONTAINS_SECURITY_SOURCE)
			{
				_eids.pop_front();
			}

			_eids.push_front(source.getNodeEID());

			set(Block::BLOCK_CONTAINS_EIDS, true);
			this->_ciphersuite_flags |= SecurityBlock::CONTAINS_SECURITY_SOURCE;

#ifdef __DEVELOPMENT_ASSERTIONS__
			assert(getSecuritySource() == source.getNodeEID());
#endif
		}

		void SecurityBlock::setSecurityDestination(const dtn::data::EID &destination)
		{
			std::list<dtn::data::EID>::iterator it = _eids.begin();

			if (_ciphersuite_flags & SecurityBlock::CONTAINS_SECURITY_SOURCE)
			{
				it++;
			}

			if (_ciphersuite_flags & SecurityBlock::CONTAINS_SECURITY_DESTINATION)
			{
				(*it) = destination.getNodeEID();
			}
			else
			{
				_eids.insert(it, destination.getNodeEID());
				set(Block::BLOCK_CONTAINS_EIDS, true);
				this->_ciphersuite_flags |= SecurityBlock::CONTAINS_SECURITY_DESTINATION;
			}

#ifdef __DEVELOPMENT_ASSERTIONS__
			assert(getSecurityDestination() == destination.getNodeEID());
#endif
		}

//		void SecurityBlock::setSourceAndDestination(const dtn::data::Bundle& bundle, SecurityBlock& sb, const dtn::data::EID& dest) const
//		{
//			// set source and destination
//			if (bundle._source != _our_id.getNodeEID() && _our_id != dtn::data::EID())
//				sb.setSecuritySource(_our_id.getNodeEID());
//
//			if (bundle._destination != dest.getNodeEID() && dest != dtn::data::EID())
//				sb.setSecurityDestination(dest.getNodeEID());
//		}

		void SecurityBlock::setCiphersuiteId(const CIPHERSUITE_IDS id)
		{
			_ciphersuite_id = static_cast<u_int64_t>(id);
		}

		void SecurityBlock::setCorrelator(const u_int64_t corr)
		{
			_correlator = corr;
			_ciphersuite_flags |= SecurityBlock::CONTAINS_CORRELATOR;
		}

		bool SecurityBlock::isCorrelatorPresent(const dtn::data::Bundle& bundle, const u_int64_t correlator)
		{
			std::list<const dtn::data::Block *> blocks = bundle.getBlocks();
			bool return_val = false;
			for (std::list<const dtn::data::Block *>::const_iterator it = blocks.begin(); it != blocks.end() && !return_val; it++)
			{
				char type = (*it)->getType();
				if (type == BUNDLE_AUTHENTICATION_BLOCK
					|| type == PAYLOAD_INTEGRITY_BLOCK
					|| type == PAYLOAD_CONFIDENTIAL_BLOCK
					|| type == EXTENSION_SECURITY_BLOCK)
				return_val = static_cast<const SecurityBlock*>(*it)->_correlator == correlator;
			}
			return return_val;
		}

		u_int64_t SecurityBlock::createCorrelatorValue(const dtn::data::Bundle& bundle)
		{
			u_int64_t corr = random();
			while (isCorrelatorPresent(bundle, corr))
				corr = random();
			return corr;
		}

		size_t SecurityBlock::getLength() const
		{
			size_t length = dtn::data::SDNV::getLength(_ciphersuite_id)
				+ dtn::data::SDNV::getLength(_ciphersuite_flags);

			if (_ciphersuite_flags & CONTAINS_CORRELATOR)
				length += dtn::data::SDNV::getLength(_correlator);
			if (_ciphersuite_flags & CONTAINS_CIPHERSUITE_PARAMS)
				length += _ciphersuite_params.getLength();
			if (_ciphersuite_flags & CONTAINS_SECURITY_RESULT)
				length += dtn::data::SDNV::getLength(getSecurityResultSize()) + getSecurityResultSize();

			return length;
		}

		size_t SecurityBlock::getLength_mutable() const
		{
			// ciphersuite_id
			size_t length = MutualSerializer::sdnv_size;
			// ciphersuite_flags
			length += MutualSerializer::sdnv_size;
			// correlator
			if (_ciphersuite_flags & CONTAINS_CORRELATOR)
				length += MutualSerializer::sdnv_size;
			// ciphersuite parameters
			if (_ciphersuite_flags & CONTAINS_CIPHERSUITE_PARAMS)
			{
				length += MutualSerializer::sdnv_size;
				length += _ciphersuite_params.size();
			}
			// security result
			if (_ciphersuite_flags & CONTAINS_SECURITY_RESULT)
				length += MutualSerializer::sdnv_size + getSecurityResultSize();

			return length;
		}

		std::ostream& SecurityBlock::serialize(std::ostream &stream) const
		{
			stream << dtn::data::SDNV(_ciphersuite_id) << dtn::data::SDNV(_ciphersuite_flags);

			if (_ciphersuite_flags & CONTAINS_CORRELATOR)
				stream << dtn::data::SDNV(_correlator);
			if (_ciphersuite_flags & CONTAINS_CIPHERSUITE_PARAMS)
				stream << _ciphersuite_params;

			if (_ciphersuite_flags & CONTAINS_SECURITY_RESULT)
			{
				if (ignore_security_result)
					stream << dtn::data::SDNV(getSecurityResultSize());
				else
					stream << _security_result;
			}

			return stream;
		}

		ostream& SecurityBlock::serialize(ostream& stream, const bool mutual) const
		{
			if (mutual)
				return serialize_mutual(stream);
			else
				return serialize(stream);
		}

		std::istream& SecurityBlock::deserialize(std::istream &stream)
		{
#ifdef __DEVELOPMENT_ASSERTIONS__
			// recheck blocktype. if blocktype is set wrong, this will be a huge fail
			assert(_blocktype == BUNDLE_AUTHENTICATION_BLOCK || _blocktype == PAYLOAD_INTEGRITY_BLOCK || _blocktype == PAYLOAD_CONFIDENTIAL_BLOCK || _blocktype == EXTENSION_SECURITY_BLOCK);
#endif

			dtn::data::SDNV ciphersuite_id, ciphersuite_flags;
			stream >> ciphersuite_id >> ciphersuite_flags;
			_ciphersuite_id = ciphersuite_id.getValue();
			_ciphersuite_flags = ciphersuite_flags.getValue();

#ifdef __DEVELOPMENT_ASSERTIONS__
			// recheck ciphersuite_id
			assert(_ciphersuite_id == BAB_HMAC || _ciphersuite_id == PIB_RSA_SHA256 || _ciphersuite_id == PCB_RSA_AES128_PAYLOAD_PIB_PCB || _ciphersuite_id == ESB_RSA_AES128_EXT);
			// recheck ciphersuite_flags, could be more exhaustive
			assert(_ciphersuite_flags < 32);
#endif

			if (_ciphersuite_flags & CONTAINS_CORRELATOR)
			{
				dtn::data::SDNV correlator;
				stream >> correlator;
				_correlator = correlator.getValue();
			}
			if (_ciphersuite_flags & CONTAINS_CIPHERSUITE_PARAMS)
			{
				stream >> _ciphersuite_params;
#ifdef __DEVELOPMENT_ASSERTIONS__
				assert(_ciphersuite_params.size() > 0);
#endif
			}

 			if (_ciphersuite_flags & CONTAINS_SECURITY_RESULT)
			{
 				stream >> _security_result;
#ifdef __DEVELOPMENT_ASSERTIONS__
				assert(_security_result.size() > 0);
#endif
			}

			return stream;
		}

		void SecurityBlock::set_ignore_security_result(const bool value) const
		{
			ignore_security_result = value;
		}

		std::ostream& SecurityBlock::serialize_mutual(std::ostream &stream) const
		{
			MutualSerializer::write_mutable(stream, dtn::data::SDNV(_ciphersuite_id));
			MutualSerializer::write_mutable(stream, dtn::data::SDNV(_ciphersuite_flags));

			if (_ciphersuite_flags & CONTAINS_CORRELATOR)
				MutualSerializer::write_mutable(stream, dtn::data::SDNV(_ciphersuite_flags));

			if (_ciphersuite_flags & CONTAINS_CIPHERSUITE_PARAMS)
			{
				MutualSerializer::write_mutable(stream, dtn::data::SDNV(_ciphersuite_params.length()));
				stream.write(static_cast<std::string>(_ciphersuite_params).c_str(),_ciphersuite_params.length());
			}

			if (_ciphersuite_flags & CONTAINS_SECURITY_RESULT)
			{
				if (ignore_security_result)
					MutualSerializer::write_mutable(stream, dtn::data::SDNV(getSecurityResultSize()));
				else
				{
					MutualSerializer::write_mutable(stream, dtn::data::SDNV(getSecurityResultSize()));
					stream.write(_security_result.c_str(), getSecurityResultSize());
				}
			}

			return stream;
		}

		size_t SecurityBlock::getSecurityResultSize() const
		{
#ifdef __DEVELOPMENT_ASSERTIONS__
			assert(_security_result.size() != 0);
#endif
			return _security_result.size();
		}

		size_t SecurityBlock::addTLV(std::string& string, const char type, const size_t length, const char * const value)
		{
			char sdnv[dtn::data::SDNV::MAX_LENGTH];
			size_t copied = dtn::data::SDNV(length).encode(sdnv, dtn::data::SDNV::MAX_LENGTH);

			string.append(&type, sizeof(char)).append(sdnv, copied).append(value, length);

			return sizeof(char)+copied+length;
		}

		size_t SecurityBlock::addTLV(std::string& string, const char type, const std::string& len_val)
		{
			return addTLV(string, type, len_val.size(), len_val.c_str());
		}

		inline signed char getTLVType(const std::string& string, size_t& pos)
		{
			return string[pos++];
		}

		inline u_int64_t getTLVLength(const std::string& string, size_t& pos)
		{
			char sdnv_raw[dtn::data::SDNV::MAX_LENGTH];
			size_t copied = 0;
			while ((sdnv_raw[copied++] = string[pos++]) & 0x80) ;
			dtn::data::SDNV sdnv;
			sdnv.decode(sdnv_raw, copied);
			return sdnv.getValue();
		}

		inline std::string getTLVValue(const std::string& string, size_t& pos, u_int64_t length)
		{
			return string.substr(pos, length);
		}

		size_t SecurityBlock::removeTLV(std::string& string, const char type)
		{
			std::list<std::pair<const size_t, const char * const> > tlvs;
			for (size_t i = 0; i < string.size();)
			{
				size_t blockstart = i;
				// get type
				signed char tlvtype = getTLVType(string, i);

				// get length
				u_int64_t length = getTLVLength(string, i);

				i += length;

#ifdef __DEVELOPMENT_ASSERTIONS__
 				assert(i <= string.size());
#endif

				// see if it is the TLV to be removed
				if (type == tlvtype)
				{
					string.erase(blockstart, i-blockstart);
					return i - blockstart;
				}
			}
			return 0;
		}

		const std::list<std::string> SecurityBlock::getTLVs(const std::string& string, const char type)
		{
			std::list<std::string> tlvs;
			for (size_t i = 0; i < string.size();)
			{
				// get type
				signed char tlvtype = getTLVType(string, i);

				// get length
				u_int64_t length = getTLVLength(string, i);

				// see if type is ok
				if (type == tlvtype)
					// read TLV
					tlvs.push_back(getTLVValue(string, i, length));

				i += length;

#ifdef __DEVELOPMENT_ASSERTIONS__
				assert(i <= string.size());
#endif
			}
			return tlvs;
		}

		void SecurityBlock::createSaltAndKey(u_int32_t& salt, unsigned char* key, size_t key_size)
		{

			if (!RAND_bytes(reinterpret_cast<unsigned char *>(&salt), sizeof(u_int32_t)))
			{
				IBRCOMMON_LOGGER_ex(critical) << "failed to generate salt. maybe /dev/urandom is missing for seeding the PRNG" << IBRCOMMON_LOGGER_ENDL;
				ERR_print_errors_fp(stderr);
			}
			if (!RAND_bytes(key, key_size))
			{
				IBRCOMMON_LOGGER_ex(critical) << "failed to generate key. maybe /dev/urandom is missing for seeding the PRNG" << IBRCOMMON_LOGGER_ENDL;
				ERR_print_errors_fp(stderr);
			}
		}

		void SecurityBlock::addKey(std::string& security_parameter, unsigned char const * const key, size_t key_size, RSA * rsa)
		{
			// encrypt the ephemeral key and place it in _ciphersuite_params
#ifdef __DEVELOPMENT_ASSERTIONS__
			assert(key_size < RSA_size(rsa)-41);
#endif
			unsigned char encrypted_key[RSA_size(rsa)];
			int encrypted_key_len = RSA_public_encrypt(key_size, key, encrypted_key, rsa, RSA_PKCS1_OAEP_PADDING);
			if (encrypted_key_len == -1)
			{
				IBRCOMMON_LOGGER_ex(critical) << "failed to encrypt the symmetric AES key" << IBRCOMMON_LOGGER_ENDL;
				ERR_print_errors_fp(stderr);
			}
			SecurityBlock::addTLV(security_parameter, SecurityBlock::key_information, std::string(reinterpret_cast<char *>(encrypted_key), encrypted_key_len));
		}

		bool SecurityBlock::getKey(const std::string& security_parameter, unsigned char * key, size_t key_size, RSA * rsa)
		{
			std::string key_string(SecurityBlock::getTLVs(security_parameter, SecurityBlock::key_information).begin().operator*());
			// get key, convert with reinterpret_cast
			unsigned char const * encrypted_key = reinterpret_cast<unsigned char const *>(key_string.c_str());
			unsigned char the_key[RSA_size(rsa)];
			RSA_blinding_on(rsa, NULL);
			int plaintext_key_len = RSA_private_decrypt(key_string.size(), encrypted_key, the_key, rsa, RSA_PKCS1_OAEP_PADDING);
			RSA_blinding_off(rsa);
			if (plaintext_key_len == -1)
			{
				IBRCOMMON_LOGGER_ex(critical) << "failed to decrypt the symmetric AES key" << IBRCOMMON_LOGGER_ENDL;
				ERR_print_errors_fp(stderr);
				return false;
			}
#ifdef __DEVELOPMENT_ASSERTIONS__
			assert(plaintext_key_len == key_size);
#endif
			std::copy(the_key, the_key+key_size, key);
			return true;
		}

		void SecurityBlock::copyEID(const Block& from, Block& to, size_t skip)
		{
			// take eid list, getEIDList() is broken
			std::list<dtn::data::EID> their_eids = from.getEIDList();
			std::list<dtn::data::EID>::iterator it = their_eids.begin();

			while (it != their_eids.end() && skip > 0)
			{
				skip--;
				it++;
			}

			for (; it != their_eids.end(); it++)
				to.addEID(*it);
		}

		void SecurityBlock::addSalt(string& security_parameters, u_int32_t salt)
		{
			std::stringstream salt_stream;
			salt_stream << salt;
			SecurityBlock::addTLV(security_parameters, SecurityBlock::salt, salt_stream.str().size(), salt_stream.str().c_str());
		}

		u_int32_t SecurityBlock::getSalt(const std::string& security_parameters)
		{
			// get salt, convert with stringstream
			std::string salt_string(SecurityBlock::getTLVs(security_parameters, SecurityBlock::salt).begin().operator*());
			std::stringstream salt_stream;
			salt_stream << salt_string;
			u_int32_t salt;
			salt_stream >> salt;
			return salt;
		}

		void SecurityBlock::decryptBlock(dtn::data::Bundle& bundle, dtn::security::SecurityBlock const * block, u_int32_t salt, const unsigned char key[ibrcommon::AES128Stream::key_size_in_bytes])
		{
			// get iv, convert with reinterpret_cast
			std::string iv_string(SecurityBlock::getTLVs(block->_ciphersuite_params, SecurityBlock::initialization_vector).begin().operator*());
			unsigned char const * iv = reinterpret_cast<unsigned char const *>(iv_string.c_str());

			// get data and tag, the last tag_len bytes are the tag. cut them of and reinterpret_cast
			std::string data_tag_string(SecurityBlock::getTLVs(block->_security_result, SecurityBlock::encapsulated_block).begin().operator*());
			std::string tag_string(data_tag_string.substr(data_tag_string.size() - ibrcommon::AES128Stream::tag_len, ibrcommon::AES128Stream::tag_len));
			unsigned char const * tag = reinterpret_cast<unsigned char const *>(tag_string.c_str());
			data_tag_string.resize(data_tag_string.size() - ibrcommon::AES128Stream::tag_len);

			// decrypt block
			std::stringstream plaintext;
			ibrcommon::AES128Stream decrypt(ibrcommon::AES128Stream::DECRYPT, plaintext, key, salt, iv, tag);
			decrypt << data_tag_string << std::flush;

			if (!decrypt.isTagGood())
				throw ibrcommon::Exception("decryption of block failed - tag is bad");

			// deserialize block
			dtn::data::DefaultDeserializer ddser(plaintext);

			// BLOCK_TYPE
			char block_type = plaintext.peek();
			if (dtn::data::PayloadBlock::BLOCK_TYPE)
			{
				dtn::data::PayloadBlock &plaintext_block = bundle.insert<dtn::data::PayloadBlock>(*block);
				ddser >> plaintext_block;
			}
			else
			{
				try {
					dtn::data::ExtensionBlock::Factory &f = dtn::data::ExtensionBlock::Factory::get(block_type);
					dtn::data::Block &plaintext_block = bundle.insert(f, *block);
					ddser >> plaintext_block;

					plaintext_block.getEIDList().clear();

					// copy eids
					// remove security_source and destination
					size_t skip = 0;
					if (block->_ciphersuite_flags & SecurityBlock::CONTAINS_SECURITY_DESTINATION)
						skip++;
					if (block->_ciphersuite_flags & SecurityBlock::CONTAINS_SECURITY_SOURCE)
						skip++;
					copyEID(plaintext_block, plaintext_block, skip);

				} catch (const ibrcommon::Exception &ex) {
					dtn::data::ExtensionBlock &plaintext_block = bundle.insert<dtn::data::ExtensionBlock>(*block);
					ddser >> plaintext_block;

					plaintext_block.getEIDList().clear();

					// copy eids
					// remove security_source and destination
					size_t skip = 0;
					if (block->_ciphersuite_flags & SecurityBlock::CONTAINS_SECURITY_DESTINATION)
						skip++;
					if (block->_ciphersuite_flags & SecurityBlock::CONTAINS_SECURITY_SOURCE)
						skip++;
					copyEID(plaintext_block, plaintext_block, skip);
				}
			}

			bundle.remove(*block);
		}

		void SecurityBlock::addFragmentRange(string& ciphersuite_params, size_t fragmentoffset, std::istream& stream)
		{
			stream.seekg(0, ios_base::end);
			std::streampos end = stream.tellg();
			stream.seekg(0, ios_base::beg);
			std::streampos begin = stream.tellg();
			std::streampos range = end - begin + 1;

			dtn::data::SDNV offset(fragmentoffset);
			dtn::data::SDNV range_sdnv(range);

			char off_range[2*dtn::data::SDNV::MAX_LENGTH];
			size_t used = offset.encode(off_range, 2*dtn::data::SDNV::MAX_LENGTH);
			used += range_sdnv.encode(off_range + used, 2*dtn::data::SDNV::MAX_LENGTH - used);

			SecurityBlock::addTLV(ciphersuite_params, SecurityBlock::fragment_range, used, off_range);
		}

		bool SecurityBlock::isSecuritySource(const dtn::data::Bundle& bundle, const dtn::data::EID& eid) const
		{
			return getSecuritySource(bundle) == eid.getNodeEID();
		}

		bool SecurityBlock::isSecurityDestination(const dtn::data::Bundle& bundle, const dtn::data::EID& eid) const
		{
			return getSecurityDestination(bundle) == eid.getNodeEID();
		}
		
		const dtn::data::EID SecurityBlock::getSecuritySource(const dtn::data::Bundle& bundle) const
		{
			dtn::data::EID source = getSecuritySource();
			if (source == dtn::data::EID())
				source = bundle._source.getNodeEID();
			return source;
		}

		const dtn::data::EID SecurityBlock::getSecurityDestination(const dtn::data::Bundle& bundle) const
		{
			dtn::data::EID destination = getSecurityDestination();
			if (destination == dtn::data::EID())
				destination = bundle._destination.getNodeEID();
			return destination;
		}
	}
}

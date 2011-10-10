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
#include <netinet/in.h>

#ifdef __DEVELOPMENT_ASSERTIONS__
#include <cassert>
#endif

namespace dtn
{
	namespace security
	{
		const std::string SecurityBlock::TLVList::toString() const
		{
			std::stringstream ss;

			for (std::set<TLV>::const_iterator iter = begin(); iter != end(); iter++)
			{
				ss << (*iter);
			}

			return ss.str();
		}

		size_t SecurityBlock::TLVList::getLength() const
		{
			size_t len = getPayloadLength();
			return len;
		}

		size_t SecurityBlock::TLVList::getPayloadLength() const
		{
			size_t len = 0;

			for (std::set<SecurityBlock::TLV>::const_iterator iter = begin(); iter != end(); iter++)
			{
				len += (*iter).getLength();
			}

			return len;
		}

		const std::string SecurityBlock::TLVList::get(SecurityBlock::TLV_TYPES type) const
		{
			for (std::set<SecurityBlock::TLV>::const_iterator iter = begin(); iter != end(); iter++)
			{
				if ((*iter).getType() == type)
				{
					return (*iter).getValue();
				}
			}

			throw ibrcommon::Exception("element not found");
		}

		void SecurityBlock::TLVList::get(TLV_TYPES type, unsigned char *value, size_t length) const
		{
			const std::string data = get(type);

			if (length < data.size())
			{
				::memcpy(value, data.c_str(), length);
			}
			else
			{
				::memcpy(value, data.c_str(), data.size());
			}
		}

		void SecurityBlock::TLVList::set(SecurityBlock::TLV_TYPES type, std::string value)
		{
			SecurityBlock::TLV tlv(type, value);

			erase(tlv);
			insert(tlv);
		}

		void SecurityBlock::TLVList::set(TLV_TYPES type, const unsigned char *value, size_t length)
		{
			const std::string data(reinterpret_cast<const char *>(value), length);
			set(type, data);
		}

		void SecurityBlock::TLVList::remove(SecurityBlock::TLV_TYPES type)
		{
			erase(SecurityBlock::TLV(type, ""));
		}

		const std::string SecurityBlock::TLV::getValue() const
		{
			return _value;
		}

		SecurityBlock::TLV_TYPES SecurityBlock::TLV::getType() const
		{
			return _type;
		}

		size_t SecurityBlock::TLV::getLength() const
		{
			return _value.getLength() + sizeof(char);
		}

		std::ostream& operator<<(std::ostream &stream, const SecurityBlock::TLVList &tlvlist)
		{
			dtn::data::SDNV length(tlvlist.getPayloadLength());
			stream << length;

			for (std::set<SecurityBlock::TLV>::const_iterator iter = tlvlist.begin(); iter != tlvlist.end(); iter++)
			{
				stream << (*iter);
			}
			return stream;
		}

		std::istream& operator>>(std::istream &stream, SecurityBlock::TLVList &tlvlist)
		{
			dtn::data::SDNV length;
			stream >> length;
			size_t read_length = 0;

			while (read_length < length.getValue())
			{
				SecurityBlock::TLV tlv;
				stream >> tlv;
				tlvlist.insert(tlv);
				read_length += tlv.getLength();
			}

			return stream;
		}

		bool SecurityBlock::TLV::operator<(const SecurityBlock::TLV &tlv) const
		{
			return (_type < tlv._type);
		}

		bool SecurityBlock::TLV::operator==(const SecurityBlock::TLV &tlv) const
		{
			return (_type == tlv._type);
		}

		std::ostream& operator<<(std::ostream &stream, const SecurityBlock::TLV &tlv)
		{
			stream.put((char)tlv._type);
			stream << tlv._value;
			return stream;
		}

		std::istream& operator>>(std::istream &stream, SecurityBlock::TLV &tlv)
		{
			char tlv_type;
			stream.get(tlv_type); tlv._type = SecurityBlock::TLV_TYPES(tlv_type);
			stream >> tlv._value;
			return stream;
		}

		SecurityBlock::SecurityBlock(const dtn::security::SecurityBlock::BLOCK_TYPES type, const dtn::security::SecurityBlock::CIPHERSUITE_IDS id)
		: Block(type), _ciphersuite_id(id), _ciphersuite_flags(0), _correlator(0)
		{

		}

		SecurityBlock::SecurityBlock(const dtn::security::SecurityBlock::BLOCK_TYPES type)
		: Block(type), _ciphersuite_flags(0), _correlator(0)
		{

		}

		SecurityBlock::~SecurityBlock()
		{
		}

		void SecurityBlock::store_security_references()
		{
			// clear the EID list
			_eids.clear();

			// first the security source
			if (_security_source == dtn::data::EID())
			{
				_ciphersuite_flags &= ~(SecurityBlock::CONTAINS_SECURITY_SOURCE);
			}
			else
			{
				_ciphersuite_flags |= SecurityBlock::CONTAINS_SECURITY_SOURCE;
				_eids.push_back(_security_source);
			}

			// then the destination
			if (_security_destination == dtn::data::EID())
			{
				_ciphersuite_flags &= ~(SecurityBlock::CONTAINS_SECURITY_DESTINATION);
			}
			else
			{
				_ciphersuite_flags |= SecurityBlock::CONTAINS_SECURITY_DESTINATION;
				_eids.push_back(_security_destination);
			}

			if (_eids.size() > 0)
			{
				set(Block::BLOCK_CONTAINS_EIDS, true);
			}
			else
			{
				set(Block::BLOCK_CONTAINS_EIDS, false);
			}
		}

		const dtn::data::EID SecurityBlock::getSecuritySource() const
		{
			return _security_source;
		}

		const dtn::data::EID SecurityBlock::getSecurityDestination() const
		{
			return _security_destination;
		}

		void SecurityBlock::setSecuritySource(const dtn::data::EID &source)
		{
			_security_source = source;
			store_security_references();
		}

		void SecurityBlock::setSecurityDestination(const dtn::data::EID &destination)
		{
			_security_destination = destination;
			store_security_references();
		}

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
			{
				length += dtn::data::SDNV::getLength(_correlator);
			}

			if (_ciphersuite_flags & CONTAINS_CIPHERSUITE_PARAMS)
			{
				const dtn::data::SDNV size(_ciphersuite_params.getLength());
				length += size.getLength() + size.getValue();
			}

			if (_ciphersuite_flags & CONTAINS_SECURITY_RESULT)
			{
				const dtn::data::SDNV size(getSecurityResultSize());
				length += size.getLength() + size.getValue();
			}

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
			{
				length += MutualSerializer::sdnv_size;
			}

			// ciphersuite parameters
			if (_ciphersuite_flags & CONTAINS_CIPHERSUITE_PARAMS)
			{
				length += MutualSerializer::sdnv_size;
				length += _ciphersuite_params.getLength();
			}
			// security result
			if (_ciphersuite_flags & CONTAINS_SECURITY_RESULT)
			{
				length += MutualSerializer::sdnv_size + getSecurityResultSize();
			}

			return length;
		}

		std::ostream& SecurityBlock::serialize(std::ostream &stream, size_t &length) const
		{
			stream << dtn::data::SDNV(_ciphersuite_id) << dtn::data::SDNV(_ciphersuite_flags);

			if (_ciphersuite_flags & CONTAINS_CORRELATOR)
			{
				stream << dtn::data::SDNV(_correlator);
			}

			if (_ciphersuite_flags & CONTAINS_CIPHERSUITE_PARAMS)
			{
				stream << _ciphersuite_params;
			}

			if (_ciphersuite_flags & CONTAINS_SECURITY_RESULT)
			{
				stream << _security_result;
			}

			return stream;
		}

		std::ostream& SecurityBlock::serialize_strict(std::ostream &stream, size_t &length) const
		{
			stream << dtn::data::SDNV(_ciphersuite_id) << dtn::data::SDNV(_ciphersuite_flags);

			if (_ciphersuite_flags & CONTAINS_CORRELATOR)
			{
				stream << dtn::data::SDNV(_correlator);
			}

			if (_ciphersuite_flags & CONTAINS_CIPHERSUITE_PARAMS)
			{
				stream << _ciphersuite_params;
			}

			if (_ciphersuite_flags & CONTAINS_SECURITY_RESULT)
			{
				stream << dtn::data::SDNV(getSecurityResultSize());
			}

			return stream;
		}

		std::istream& SecurityBlock::deserialize(std::istream &stream, const size_t length)
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

			// copy security source and destination
			if (_ciphersuite_flags & SecurityBlock::CONTAINS_SECURITY_SOURCE)
			{
				if (_eids.size() == 0)
					throw dtn::SerializationFailedException("ciphersuite flags indicate a security source, but it is not present");

				_security_source = _eids.front();
			}

			if (_ciphersuite_flags & SecurityBlock::CONTAINS_SECURITY_DESTINATION)
			{
				if (_ciphersuite_flags & SecurityBlock::CONTAINS_SECURITY_SOURCE)
				{
					if (_eids.size() < 2)
						throw dtn::SerializationFailedException("ciphersuite flags indicate a security destination, but it is not present");

					_security_destination = (*(_eids.begin())++);
				}
				else
				{
					if (_eids.size() == 0)
						throw dtn::SerializationFailedException("ciphersuite flags indicate a security destination, but it is not present");

					_security_destination = _eids.front();
				}
			}

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
				assert(_ciphersuite_params.getLength() > 0);
#endif
			}

 			if (_ciphersuite_flags & CONTAINS_SECURITY_RESULT)
			{
 				stream >> _security_result;
#ifdef __DEVELOPMENT_ASSERTIONS__
				assert(_security_result.getLength() > 0);
#endif
			}

			return stream;
		}

		dtn::security::MutualSerializer& SecurityBlock::serialize_mutable(dtn::security::MutualSerializer &serializer) const
		{
			serializer << dtn::data::SDNV(_ciphersuite_id);
			serializer << dtn::data::SDNV(_ciphersuite_flags);

			if (_ciphersuite_flags & CONTAINS_CORRELATOR)
				serializer << dtn::data::SDNV(_ciphersuite_flags);

			if (_ciphersuite_flags & CONTAINS_CIPHERSUITE_PARAMS)
			{
				serializer << _ciphersuite_params;
			}

			if (_ciphersuite_flags & CONTAINS_SECURITY_RESULT)
			{
				serializer << _security_result;
			}

			return serializer;
		}

		dtn::security::MutualSerializer& SecurityBlock::serialize_mutable_without_security_result(dtn::security::MutualSerializer &serializer) const
		{
			serializer << dtn::data::SDNV(_ciphersuite_id);
			serializer << dtn::data::SDNV(_ciphersuite_flags);

			if (_ciphersuite_flags & CONTAINS_CORRELATOR)
				serializer << dtn::data::SDNV(_ciphersuite_flags);

			if (_ciphersuite_flags & CONTAINS_CIPHERSUITE_PARAMS)
			{
				serializer << _ciphersuite_params;
			}

			if (_ciphersuite_flags & CONTAINS_SECURITY_RESULT)
			{
				serializer << dtn::data::SDNV(getSecurityResultSize());
			}

			return serializer;
		}

		size_t SecurityBlock::getSecurityResultSize() const
		{
#ifdef __DEVELOPMENT_ASSERTIONS__
			assert(_security_result.getLength() != 0);
#endif
			return _security_result.getLength();
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

		void SecurityBlock::addKey(TLVList& security_parameter, unsigned char const * const key, size_t key_size, RSA * rsa)
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
			security_parameter.set(SecurityBlock::key_information, std::string(reinterpret_cast<char *>(encrypted_key), encrypted_key_len));
		}

		bool SecurityBlock::getKey(const TLVList& security_parameter, unsigned char * key, size_t key_size, RSA * rsa)
		{
			std::string key_string = security_parameter.get(SecurityBlock::key_information);
			// get key, convert with reinterpret_cast
			unsigned char const * encrypted_key = reinterpret_cast<const unsigned char*>(key_string.c_str());
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

		void SecurityBlock::addSalt(TLVList& security_parameters, const u_int32_t &salt)
		{
			u_int32_t nsalt = htonl(salt);
			security_parameters.set(SecurityBlock::salt, (const unsigned char*)&nsalt, sizeof(nsalt));
		}

		u_int32_t SecurityBlock::getSalt(const TLVList& security_parameters)
		{
			u_int32_t nsalt = 0;
			security_parameters.get(SecurityBlock::salt, (unsigned char*)&nsalt, sizeof(nsalt));
			return ntohl(nsalt);
		}

		void SecurityBlock::decryptBlock(dtn::data::Bundle& bundle, const dtn::security::SecurityBlock &block, u_int32_t salt, const unsigned char key[ibrcommon::AES128Stream::key_size_in_bytes])
		{
			// the array for the extracted tag
			unsigned char tag[ibrcommon::AES128Stream::tag_len];

			// the array for the extracted iv
			unsigned char iv[ibrcommon::AES128Stream::iv_len];

			// get iv, convert with reinterpret_cast
			block._ciphersuite_params.get(SecurityBlock::initialization_vector, iv, ibrcommon::AES128Stream::iv_len);

			// get data and tag, the last tag_len bytes are the tag. cut them of and reinterpret_cast
			std::string block_data = block._security_result.get(SecurityBlock::encapsulated_block);

			// create a pointer to the tag begin
			const char *tag_p = block_data.c_str() + (block_data.size() - ibrcommon::AES128Stream::tag_len);

			// copy the tag
			::memcpy(tag, tag_p, ibrcommon::AES128Stream::tag_len);

			// strip off the tag from block data
			block_data.resize(block_data.size() - ibrcommon::AES128Stream::tag_len);

			// decrypt block
			std::stringstream plaintext;
			ibrcommon::AES128Stream decrypt(ibrcommon::CipherStream::CIPHER_DECRYPT, plaintext, key, salt, iv);
			decrypt << block_data << std::flush;

			// verify the decrypt tag
			if (!decrypt.verify(tag))
			{
				throw ibrcommon::Exception("decryption of block failed - tag is bad");
			}

			// deserialize block
			dtn::data::DefaultDeserializer ddser(plaintext);

			// peek the block type
			char block_type = plaintext.peek();

			if (block_type == dtn::data::PayloadBlock::BLOCK_TYPE)
			{
				dtn::data::PayloadBlock &plaintext_block = bundle.insert<dtn::data::PayloadBlock>(block);
				ddser >> plaintext_block;
			}
			else
			{
				try {
					dtn::data::ExtensionBlock::Factory &f = dtn::data::ExtensionBlock::Factory::get(block_type);
					dtn::data::Block &plaintext_block = bundle.insert(f, block);
					ddser >> plaintext_block;

					plaintext_block.getEIDList().clear();

					// copy eids
					// remove security_source and destination
					size_t skip = 0;
					if (block._ciphersuite_flags & SecurityBlock::CONTAINS_SECURITY_DESTINATION)
						skip++;
					if (block._ciphersuite_flags & SecurityBlock::CONTAINS_SECURITY_SOURCE)
						skip++;
					copyEID(plaintext_block, plaintext_block, skip);

				} catch (const ibrcommon::Exception &ex) {
					dtn::data::ExtensionBlock &plaintext_block = bundle.insert<dtn::data::ExtensionBlock>(block);
					ddser >> plaintext_block;

					plaintext_block.getEIDList().clear();

					// copy eids
					// remove security_source and destination
					size_t skip = 0;
					if (block._ciphersuite_flags & SecurityBlock::CONTAINS_SECURITY_DESTINATION)
						skip++;
					if (block._ciphersuite_flags & SecurityBlock::CONTAINS_SECURITY_SOURCE)
						skip++;
					copyEID(plaintext_block, plaintext_block, skip);
				}
			}

			bundle.remove(block);
		}

		void SecurityBlock::addFragmentRange(TLVList& ciphersuite_params, size_t fragmentoffset, size_t payload_length)
		{
			dtn::data::SDNV offset(fragmentoffset);
			dtn::data::SDNV range_sdnv(payload_length);

			std::stringstream ss;
			ss << offset << range_sdnv;

			ciphersuite_params.set(SecurityBlock::fragment_range, ss.str());
		}

		bool SecurityBlock::isSecuritySource(const dtn::data::Bundle& bundle, const dtn::data::EID& eid) const
		{
			IBRCOMMON_LOGGER_DEBUG(30) << "check security source: " << getSecuritySource(bundle).getString() << " == " << eid.getNode().getString() << IBRCOMMON_LOGGER_ENDL;
			return getSecuritySource(bundle) == eid.getNode();
		}

		bool SecurityBlock::isSecurityDestination(const dtn::data::Bundle& bundle, const dtn::data::EID& eid) const
		{
			IBRCOMMON_LOGGER_DEBUG(30) << "check security destination: " << getSecurityDestination(bundle).getString() << " == " << eid.getNode().getString() << IBRCOMMON_LOGGER_ENDL;
			return getSecurityDestination(bundle) == eid.getNode();
		}
		
		const dtn::data::EID SecurityBlock::getSecuritySource(const dtn::data::Bundle& bundle) const
		{
			dtn::data::EID source = getSecuritySource();
			if (source == dtn::data::EID())
				source = bundle._source.getNode();
			return source;
		}

		const dtn::data::EID SecurityBlock::getSecurityDestination(const dtn::data::Bundle& bundle) const
		{
			dtn::data::EID destination = getSecurityDestination();
			if (destination == dtn::data::EID())
				destination = bundle._destination.getNode();
			return destination;
		}
	}
}

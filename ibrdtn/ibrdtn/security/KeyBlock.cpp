#include "ibrdtn/security/KeyBlock.h"
#include <ibrcommon/Logger.h>
#include <openssl/err.h>
#ifdef __DEVELOPMENT_ASSERTIONS__
#include <cassert>
#endif

namespace dtn
{
	namespace security
	{
		dtn::data::Block* KeyBlock::Factory::create()
		{
			return new KeyBlock();
		}

		KeyBlock::KeyBlock()
		: Block(BLOCK_TYPE), _key(""), _sendable_times(10), _refresh_counter(false)
		{

		}

		KeyBlock::~KeyBlock()
		{
		}

		void KeyBlock::setTarget(const dtn::data::EID& target)
		{
			// only save one EID
			_eids.clear();
			addEID(target.getNodeEID());
		}

		const dtn::data::EID KeyBlock::getTarget() const
		{
			size_t size = getEIDList().size();
#ifdef __DEVELOPMENT_ASSERTIONS__
			assert(size == 1);
#endif
			return getEIDList().front();
		}

		void KeyBlock::setSecurityBlockType(SecurityBlock::BLOCK_TYPES type)
		{
			_type = type;
		}

		SecurityBlock::BLOCK_TYPES KeyBlock::getSecurityBlockType() const
		{
			return _type;
		}

		bool KeyBlock::iskeyPresent() const
		{
			return _key.size() != 0;
		}

		void KeyBlock::addKey(RSA * rsa)
		{
			int len = i2d_RSAPublicKey(rsa, NULL);
#ifdef __DEVELOPMENT_ASSERTIONS__
			assert(len != 0);
#endif
			unsigned char buffer[len];
			unsigned char * next = buffer;
			len = i2d_RSAPublicKey(rsa, &next);
			if (len == 0)
			{
				IBRCOMMON_LOGGER_ex(critical) << "Failed to serialize the public key for " << getEIDList().front().getString() << IBRCOMMON_LOGGER_ENDL;
				ERR_print_errors_fp(stderr);
				return;
			}

			_key = dtn::data::BundleString(std::string(reinterpret_cast<char *>(buffer), len));
		}

		RSA * KeyBlock::getKey_rsa() const
		{
			RSA * rsa = createRSA(_key);

			if (rsa == NULL)
				IBRCOMMON_LOGGER_ex(critical) << "Failed to deserialize the public key for " << getEIDList().front().getString() << IBRCOMMON_LOGGER_ENDL;

			return rsa;
		}

		RSA* KeyBlock::createRSA(const std::string& key)
		{
			unsigned char const * key_char = reinterpret_cast<unsigned char const *>(key.c_str());
			RSA * rsa = d2i_RSAPublicKey(NULL, &key_char, key.size());

			if (rsa == NULL)
			{
				IBRCOMMON_LOGGER_ex(critical) << "Failed to deserialize public key" << IBRCOMMON_LOGGER_ENDL;
				ERR_print_errors_fp(stderr);
			}
			return rsa;
		}

		const std::string& KeyBlock::getKey() const
		{
			return _key;
		}

		bool KeyBlock::operator==(KeyBlock& kb) const
		{
			return (getSecurityBlockType() == kb.getSecurityBlockType()) && (getTarget() == kb.getTarget());
		}

		bool KeyBlock::operator==(const dtn::security::KeyBlock& kb) const
		{
			return (getSecurityBlockType() == kb.getSecurityBlockType()) && (getTarget() == kb.getTarget());
		}

		void KeyBlock::setDistance(unsigned int times)
		{
			_sendable_times = times;
		}

		unsigned int KeyBlock::getDistance() const
		{
			return _sendable_times;
		}

		bool KeyBlock::isSendable() const
		{
			return _sendable_times != 0;
		}

		bool KeyBlock::isNeedingRefreshing() const
		{
			return _refresh_counter;
		}

		void KeyBlock::setRefresh(bool status)
		{
			_refresh_counter = status;
		}

		size_t KeyBlock::getLength() const
		{
			size_t length = dtn::data::BundleString(_key).getLength();
			length += dtn::data::SDNV::getLength(_type);
			length += dtn::data::SDNV::getLength(_sendable_times);
			length += dtn::data::SDNV::getLength(_refresh_counter);

			return length;
		}

		std::ostream& KeyBlock::serialize(std::ostream &stream) const
		{
			stream << dtn::data::BundleString(_key) << dtn::data::SDNV(_type) << dtn::data::SDNV(_sendable_times) << dtn::data::SDNV(_refresh_counter);
			return stream;
		}

		std::istream& KeyBlock::deserialize(std::istream &stream)
		{
			dtn::data::BundleString key_string;
			dtn::data::SDNV type_sdnv, times_sdnv, refresh_sdnv;

			stream >> key_string >> type_sdnv >> times_sdnv >> refresh_sdnv;

			_key = key_string;
			_type = static_cast<SecurityBlock::BLOCK_TYPES>(type_sdnv.getValue());
			_sendable_times = static_cast<unsigned int>(times_sdnv.getValue());
			_refresh_counter = static_cast<bool>(refresh_sdnv.getValue());

			if (_sendable_times > 0)
				_sendable_times--;
			return stream;
		}
	}
}

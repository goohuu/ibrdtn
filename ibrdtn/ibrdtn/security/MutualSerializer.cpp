#include "ibrdtn/security/MutualSerializer.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/data/Block.h"
#include "ibrdtn/data/EID.h"
#include "ibrdtn/security/SecurityBlock.h"
#include <ibrcommon/Logger.h>

#include <arpa/inet.h>
#include <endian.h>

#ifdef __DEVELOPMENT_ASSERTIONS__
#include <cassert>
#endif

// needed for Debian Lenny, whichs older clibrary does not provide htobe64(x)
#include <bits/byteswap.h>
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define _ibrdtn_htobe64(x)  __bswap_64(x)
#else
#define _ibrdtn_htobe64(x) (x)
#endif

namespace dtn
{
	namespace security
	{
		MutualSerializer::MutualSerializer(std::ostream& stream, const dtn::data::Block *ignore)
		 : dtn::data::DefaultSerializer(stream), _ignore(ignore), _ignore_previous_bundles(ignore != NULL)
		 {
		 }

		MutualSerializer::~MutualSerializer()
		{
		}

		dtn::data::Serializer& MutualSerializer::operator<<(const dtn::data::PrimaryBlock &obj)
		{
			// we want to ignore all block before "ignore"
			if (_ignore != NULL) _ignore_previous_bundles = true;

			// write unpacked primary block
			// bundle version
			_stream << dtn::data::BUNDLE_VERSION;

			// processing flags
			(*this) << dtn::data::SDNV(obj._procflags & 0x0000000007C1BE);

			// length of header
			(*this) << (u_int32_t)getLength(obj);

			// dest, source, report to id
			(*this) << obj._destination;
			(*this) << obj._source;
			(*this) << obj._reportto;

			// timestamp
			(*this) << dtn::data::SDNV(obj._timestamp);
			(*this) << dtn::data::SDNV(obj._sequencenumber);

			// lifetime
			(*this) << dtn::data::SDNV(obj._lifetime);

			return *this;
		}

		dtn::data::Serializer& MutualSerializer::operator<<(const dtn::data::Block &obj)
		{
			// do we ignore the current block?
			if (_ignore_previous_bundles && (&obj != _ignore))
			{
				return *this;
			}
			else
			{
				// process all following bundles
				_ignore_previous_bundles = false;
			}

			// only take payload related blocks
			if (obj.getType() != dtn::data::PayloadBlock::BLOCK_TYPE
				&& obj.getType() != SecurityBlock::PAYLOAD_INTEGRITY_BLOCK
				&& obj.getType() != SecurityBlock::PAYLOAD_CONFIDENTIAL_BLOCK)
			{
				return *this;
			}

			_stream << obj._blocktype;
			(*this) << dtn::data::SDNV(obj._procflags & 0x0000000000000077);

#ifdef __DEVELOPMENT_ASSERTIONS__
			// test: BLOCK_CONTAINS_EIDS => (_eids.size() > 0)
			assert(!obj.get(dtn::data::Block::BLOCK_CONTAINS_EIDS) || (obj._eids.size() > 0));
#endif

			if (obj.get(dtn::data::Block::BLOCK_CONTAINS_EIDS))
				for (std::list<dtn::data::EID>::const_iterator it = obj._eids.begin(); it != obj._eids.end(); it++)
					(*this) << (*it);

			try {
				const dtn::security::SecurityBlock &sb = dynamic_cast<const dtn::security::SecurityBlock&>(obj);
				
				if ( (sb.getType() == SecurityBlock::PAYLOAD_INTEGRITY_BLOCK) || (sb.getType() == SecurityBlock::PAYLOAD_CONFIDENTIAL_BLOCK) )
				{
					// write size of the payload in the block
					(*this) << dtn::data::SDNV(sb.getLength_mutable());

					sb.serialize_mutable_without_security_result(*this);
				}
			} catch (const std::bad_cast&) {
				// write size of the payload in the block
				(*this) << dtn::data::SDNV(obj.getLength());

				// write the payload of the block
				size_t slength = 0;
				obj.serialize(_stream, slength);
			};

			return (*this);
		}

		size_t MutualSerializer::getLength(const dtn::data::Bundle&)
		{
#ifdef __DEVELOPMENT_ASSERTIONS__
			assert(false);
#endif
			return 0;
		}

		size_t MutualSerializer::getLength(const dtn::data::PrimaryBlock &obj) const
		{
			// predict the block length
			// length in bytes
			// starting with the fields after the length field

			// dest id length
			u_int32_t length = 4;
			// dest id
			length += obj._destination.getString().size();
			// source id length
			length += 4;
			// source id
			length += obj._source.getString().size();
			// report to id length
			length += 4;
			// report to id
			length += obj._reportto.getString().size();
			// creation time: 2*SDNV
			length += 2*sdnv_size;
			// lifetime: SDNV
			length += sdnv_size;

			IBRCOMMON_LOGGER_DEBUG_ex(ibrcommon::Logger::LOGGER_DEBUG) << "length: " << length << IBRCOMMON_LOGGER_ENDL;

			return length;
		}

		size_t MutualSerializer::getLength(const dtn::data::Block &obj) const
		{
			size_t len = 0;

			len += sizeof(obj._blocktype);
			// proc flags
			len += sdnv_size;

#ifdef __DEVELOPMENT_ASSERTIONS__
			// test: BLOCK_CONTAINS_EIDS => (_eids.size() > 0)
			assert(!obj.get(dtn::data::Block::BLOCK_CONTAINS_EIDS) || (obj._eids.size() > 0));
#endif

			if (obj.get(dtn::data::Block::BLOCK_CONTAINS_EIDS))
				for (std::list<dtn::data::EID>::const_iterator it = obj._eids.begin(); it != obj._eids.end(); it++)
					len += it->getString().size();

			// size-field of the size of the payload in the block
			len += sdnv_size;

			try {
				const dtn::security::SecurityBlock &sb = dynamic_cast<const dtn::security::SecurityBlock&>(obj);

				// add size of the payload in the block
				len += sb.getLength_mutable();
			} catch (const std::bad_cast&) {
				// add size of the payload in the block
				len += obj.getLength();
			};

			return len;
		}


		dtn::data::Serializer& MutualSerializer::operator<<(const u_int32_t value)
		{
			u_int32_t be = htonl(value);
			_stream.write(reinterpret_cast<char*>(&be), sizeof(u_int32_t));
			return *this;
		}

		dtn::data::Serializer& MutualSerializer::operator<<(const dtn::data::EID& value)
		{
			u_int32_t length = value.getString().length();
			(*this) << length;
			_stream << value.getString();

			return *this;
		}

		dtn::data::Serializer& MutualSerializer::operator<<(const dtn::data::SDNV& value)
		{
			// endianess muahahaha ...
			// and now we are gcc centric, even older versions work
			u_int64_t be = _ibrdtn_htobe64(value.getValue());
			_stream.write(reinterpret_cast<char*>(&be), sizeof(u_int64_t));
			return *this;
		}

		dtn::data::Serializer& MutualSerializer::operator<<(const dtn::security::SecurityBlock::TLVList& list)
		{
			(*this) << dtn::data::SDNV(list.getLength());
			_stream << list.toString();
			return *this;
		}
	}
}

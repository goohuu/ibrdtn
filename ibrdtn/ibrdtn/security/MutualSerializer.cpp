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
		MutualSerializer::MutualSerializer(std::ostream& stream)
		 : _stream(stream)
		 {
		 }

		MutualSerializer::~MutualSerializer()
		{
		}

		MutualSerializer& MutualSerializer::serialize_mutable(const dtn::data::Bundle& obj, const SecurityBlock * ignore)
		{
			// serialize the primary block
			(*this) << (dtn::data::PrimaryBlock&)obj;

			// serialize all secondary blocks
			std::list<refcnt_ptr<dtn::data::Block> > list = obj._blocks._blocks;

			std::list<refcnt_ptr<dtn::data::Block> >::const_iterator iter = list.begin();

			if (ignore)
			{
				IBRCOMMON_LOGGER_DEBUG_ex(ibrcommon::Logger::LOGGER_DEBUG) << "ignored security result block type: " << static_cast<int>(ignore->getType()) << IBRCOMMON_LOGGER_ENDL;
				ignore->set_ignore_security_result(true);
				// skip all blocks until ignore is reached
				while (&(*(*iter)) != ignore && iter != list.end())
					iter++;
			}

			for (; iter != list.end(); iter++)
			{
				const dtn::data::Block &b = (*(*iter));

				// only take payload related blocks
				if (b.getType() == dtn::data::PayloadBlock::BLOCK_TYPE
					|| b.getType() == SecurityBlock::PAYLOAD_INTEGRITY_BLOCK
					|| b.getType() == SecurityBlock::PAYLOAD_CONFIDENTIAL_BLOCK)
					(*this) << b;
			}

			if (ignore)
				ignore->set_ignore_security_result(false);

			return (*this);
		}

		MutualSerializer& MutualSerializer::operator<<(const dtn::data::Bundle &obj)
		{
			return serialize_mutable(obj, 0);
		}

		MutualSerializer& MutualSerializer::operator<<(const dtn::data::PrimaryBlock &obj)
		{
			// write unpacked primary block
			// bundle version
			_stream << dtn::data::BUNDLE_VERSION;
			// processing flags
			MutualSerializer::write_mutable(_stream, dtn::data::SDNV(obj._procflags & 0x0000000007C1BE));

			// length of header
			MutualSerializer::write_mutable(_stream, dtn::data::SDNV(getLength(obj)));

			// dest id
			MutualSerializer::write_mutable(_stream, obj._destination);
			// source id
			MutualSerializer::write_mutable(_stream, obj._source);
			// report to id
			MutualSerializer::write_mutable(_stream, obj._reportto);

			// timestamp
			MutualSerializer::write_mutable(_stream, dtn::data::SDNV(obj._timestamp));
			MutualSerializer::write_mutable(_stream, dtn::data::SDNV(obj._sequencenumber));

			// lifetime
			MutualSerializer::write_mutable(_stream, dtn::data::SDNV(obj._lifetime));

			return *this;
		}

		MutualSerializer& MutualSerializer::operator<<(const dtn::data::Block &obj)
		{
			_stream << obj._blocktype;
			MutualSerializer::write_mutable(_stream, dtn::data::SDNV(obj._procflags & 0x0000000000000077));

#ifdef __DEVELOPMENT_ASSERTIONS__
			// test: BLOCK_CONTAINS_EIDS => (_eids.size() > 0)
			assert(!(obj._procflags & Block::BLOCK_CONTAINS_EIDS) || (obj._eids.size() > 0));
#endif

			if (obj._procflags & dtn::data::Block::BLOCK_CONTAINS_EIDS)
				for (std::list<dtn::data::EID>::const_iterator it = obj._eids.begin(); it != obj._eids.end(); it++)
					MutualSerializer::write_mutable(_stream, *it);

			// write size of the payload in the block
			MutualSerializer::write_mutable(_stream, dtn::data::SDNV(obj.getLength_mutable()));

			// write the payload of the block
			obj.serialize(_stream, true);

			return (*this);
		}

		size_t MutualSerializer::getLength(const dtn::data::Bundle &obj)
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
			assert(!(obj._procflags & Block::BLOCK_CONTAINS_EIDS) || (obj._eids.size() > 0));
#endif

			if (obj._procflags & dtn::data::Block::BLOCK_CONTAINS_EIDS)
				for (std::list<dtn::data::EID>::const_iterator it = obj._eids.begin(); it != obj._eids.end(); it++)
					len += it->getString().size();

			// size-field of the size of the payload in the block
			len += sdnv_size;
			// size of the payload
			len += obj.getLength_mutable();

			return len;
		}


		std::ostream& MutualSerializer::write_mutable(std::ostream& stream, const u_int32_t value)
		{
			u_int32_t be = htonl(value);
			if (!stream)
				std::cerr << "Could not write to Stream\n";
			else
				stream.write(reinterpret_cast<char*>(&be), sizeof(u_int32_t));
			return stream;
		}

		std::ostream& MutualSerializer::write_mutable(std::ostream& stream, const dtn::data::EID& value)
		{
			if (!stream)
				std::cerr << "Could not write to Stream\n";
			else
			{
				MutualSerializer::write_mutable(stream, value.getString().size());
				stream.write(value.getString().c_str(), value.getString().size());
			}
			return stream;
		}

		std::ostream& MutualSerializer::write_mutable(std::ostream& stream, const dtn::data::SDNV& value)
		{
			// endianess muahahaha ...
			// and now we are gcc centric, even older versions work
			u_int64_t be = _ibrdtn_htobe64(value.getValue());
			if (!stream)
				std::cerr << "Could not write to Stream\n";
			else
			{
				stream.write(reinterpret_cast<char*>(&be), sizeof(u_int64_t));
			}
			return stream;
		}
	}
}

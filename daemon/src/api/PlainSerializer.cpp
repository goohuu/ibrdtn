/*
 * PlainSerializer.cpp
 *
 *  Created on: 16.06.2011
 *      Author: morgenro
 */

#include "config.h"
#include "api/PlainSerializer.h"
#include <ibrcommon/refcnt_ptr.h>
#include <ibrcommon/data/Base64Stream.h>
#include <list>

namespace dtn
{
	namespace api
	{
		PlainSerializer::PlainSerializer(std::ostream &stream)
		 : _stream(stream)
		{
		}

		PlainSerializer::~PlainSerializer()
		{
		}

		dtn::data::Serializer& PlainSerializer::operator<<(const dtn::data::Bundle &obj)
		{
			// serialize the primary block
			(*this) << (dtn::data::PrimaryBlock&)obj;

			// serialize all secondary blocks
			const std::list<const dtn::data::Block*> list = obj.getBlocks();

			// block count
			_stream << "Blocks: " << list.size() << std::endl;

			for (std::list<const dtn::data::Block*>::const_iterator iter = list.begin(); iter != list.end(); iter++)
			{
				const dtn::data::Block &b = (*(*iter));
				_stream << std::endl;
				(*this) << b;
			}

			_stream << std::endl;

			return (*this);
		}

		dtn::data::Serializer& PlainSerializer::operator<<(const dtn::data::PrimaryBlock &obj)
		{
			_stream << "Processing flags: " << obj._procflags << std::endl;
			_stream << "Timestamp: " << obj._timestamp << std::endl;
			_stream << "Sequencenumber: " << obj._sequencenumber << std::endl;
			_stream << "Source: " << obj._source.getString() << std::endl;
			_stream << "Destination: " << obj._destination.getString() << std::endl;
			_stream << "Reportto: " << obj._reportto.getString() << std::endl;
			_stream << "Custodian: " << obj._custodian.getString() << std::endl;
			_stream << "Lifetime: " << obj._lifetime << std::endl;

			if (obj._procflags & dtn::data::PrimaryBlock::FRAGMENT)
			{
				_stream << "Fragment offset: " << obj._fragmentoffset << std::endl;
				_stream << "Application data length: " << obj._appdatalength << std::endl;
			}

			return (*this);
		}

		dtn::data::Serializer& PlainSerializer::operator<<(const dtn::data::Block &obj)
		{
			_stream << "Block: " << (int)obj.getType() << std::endl;

			std::stringstream flags;

			if (obj.get(dtn::data::Block::REPLICATE_IN_EVERY_FRAGMENT))
			{
				flags << " REPLICATE_IN_EVERY_FRAGMENT";
			}

			if (obj.get(dtn::data::Block::TRANSMIT_STATUSREPORT_IF_NOT_PROCESSED))
			{
				flags << " TRANSMIT_STATUSREPORT_IF_NOT_PROCESSED";
			}

			if (obj.get(dtn::data::Block::DELETE_BUNDLE_IF_NOT_PROCESSED))
			{
				flags << " DELETE_BUNDLE_IF_NOT_PROCESSED";
			}

			if (obj.get(dtn::data::Block::DISCARD_IF_NOT_PROCESSED))
			{
				flags << " DISCARD_IF_NOT_PROCESSED";
			}

			if (obj.get(dtn::data::Block::FORWARDED_WITHOUT_PROCESSED))
			{
				flags << " FORWARDED_WITHOUT_PROCESSED";
			}

			if (flags.str().length() > 0)
			{
				_stream << "Flags:" << flags.str() << std::endl;
			}

			if (obj.get(dtn::data::Block::BLOCK_CONTAINS_EIDS))
			{
				std::list<dtn::data::EID> eid_list = obj.getEIDList();

				for (std::list<dtn::data::EID>::const_iterator iter = eid_list.begin(); iter != eid_list.end(); iter++)
				{
					_stream << "EID: " << (*iter).getString() << std::endl;
				}
			}

			try {
				_stream << std::endl;

				// put data here
				ibrcommon::Base64Stream b64(_stream, false, 80);
				obj.serialize(b64);
				b64 << std::flush;
			} catch (const std::exception &ex) {
				std::cerr << ex.what() << std::endl;
			}

			_stream << std::endl;

			return (*this);
		}

		size_t PlainSerializer::getLength(const dtn::data::Bundle &obj)
		{
			return 0;
		}

		size_t PlainSerializer::getLength(const dtn::data::PrimaryBlock &obj) const
		{
			return 0;
		}

		size_t PlainSerializer::getLength(const dtn::data::Block &obj) const
		{
			return 0;
		}
	}
}

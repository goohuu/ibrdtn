/*
 * StatusReportBlock.cpp
 *
 *  Created on: 05.06.2009
 *      Author: morgenro
 */

#include "ibrdtn/data/StatusReportBlock.h"
#include "ibrdtn/data/SDNV.h"
#include "ibrdtn/data/BundleString.h"
#include "ibrdtn/data/PayloadBlock.h"
#include <stdlib.h>
#include <sstream>

namespace dtn
{
	namespace data
	{
		StatusReportBlock::StatusReportBlock()
		 : Block(dtn::data::PayloadBlock::BLOCK_TYPE), _admfield(16), _status(0), _reasoncode(0),
		 _fragment_offset(0), _fragment_length(0), _timeof_receipt(),
		 _timeof_custodyaccept(), _timeof_forwarding(), _timeof_delivery(),
		 _timeof_deletion(), _bundle_timestamp(0), _bundle_sequence(0)
		{
		}

		StatusReportBlock::~StatusReportBlock()
		{
		}

		const size_t StatusReportBlock::getLength() const
		{
			// determine the block size
			size_t len = 0;
			len += sizeof(_admfield);
			len += sizeof(_status);

			if ( _admfield & 0x01 )
			{
				len += _fragment_offset.getLength();
				len += _fragment_length.getLength();
			}

			len += _timeof_receipt.getLength();
			len += _timeof_custodyaccept.getLength();
			len += _timeof_forwarding.getLength();

			len += _timeof_delivery.getLength();
			len += _timeof_deletion.getLength();
			len += _bundle_timestamp.getLength();
			len += _bundle_sequence.getLength();

			BundleString sourceid(_source.getString());
			len += sourceid.getLength();

			return len;
		}

		std::ostream& StatusReportBlock::serialize(std::ostream &stream) const
		{
			stream << _admfield;
			stream << _status;

			if ( _admfield & 0x01 )
			{
				stream << _fragment_offset;
				stream << _fragment_length;
			}

			stream << _timeof_receipt;
			stream << _timeof_custodyaccept;
			stream << _timeof_forwarding;
			stream << _timeof_delivery;
			stream << _timeof_deletion;
			stream << _bundle_timestamp;
			stream << _bundle_sequence;
			stream << BundleString(_source.getString());

			return stream;
		}

		std::istream& StatusReportBlock::deserialize(std::istream &stream)
		{
			stream >> _admfield;
			stream >> _status;

			if ( _admfield & 0x01 )
			{
				stream >> _fragment_offset;
				stream >> _fragment_length;
			}

			stream >> _timeof_receipt;
			stream >> _timeof_custodyaccept;
			stream >> _timeof_forwarding;
			stream >> _timeof_delivery;
			stream >> _timeof_deletion;
			stream >> _bundle_timestamp;
			stream >> _bundle_sequence;

			BundleString source;
			stream >> source;
			_source = EID(source);

			// unset block not processed bit
			set(dtn::data::Block::FORWARDED_WITHOUT_PROCESSED, false);

			return stream;
		}

	}
}

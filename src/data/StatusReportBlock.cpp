/*
 * StatusReportBlock.cpp
 *
 *  Created on: 05.06.2009
 *      Author: morgenro
 */

#include "ibrdtn/data/StatusReportBlock.h"
#include "ibrdtn/streams/BundleStreamWriter.h"
#include "ibrdtn/data/SDNV.h"
#include "ibrdtn/data/BundleString.h"
#include <stdlib.h>
#include <sstream>

namespace dtn
{
	namespace data
	{
		StatusReportBlock::StatusReportBlock()
		 : PayloadBlock(ibrcommon::StringBLOB::create()), _admfield(16), _status(0), _reasoncode(0),
		 _fragment_offset(0), _fragment_length(0), _timeof_receipt(),
		 _timeof_custodyaccept(), _timeof_forwarding(), _timeof_delivery(),
		 _timeof_deletion(), _bundle_timestamp(0), _bundle_sequence(0)
		{
		}

		StatusReportBlock::StatusReportBlock(Block *block)
		 : PayloadBlock(block->getBLOB()), _admfield(16), _status(0), _reasoncode(0),
		 _fragment_offset(0), _fragment_length(0), _timeof_receipt(),
		 _timeof_custodyaccept(), _timeof_forwarding(), _timeof_delivery(),
		 _timeof_deletion(), _bundle_timestamp(0), _bundle_sequence(0)
		{
			read();
		}

		StatusReportBlock::StatusReportBlock(ibrcommon::BLOB::Reference ref)
		 : PayloadBlock(ref), _admfield(16), _status(0), _reasoncode(0),
		 _fragment_offset(0), _fragment_length(0), _timeof_receipt(),
		 _timeof_custodyaccept(), _timeof_forwarding(), _timeof_delivery(),
		 _timeof_deletion(), _bundle_timestamp(0), _bundle_sequence(0)
		{
		}

		StatusReportBlock::~StatusReportBlock()
		{
		}

		void StatusReportBlock::read()
		{
			// read the attributes out of the BLOB
			ibrcommon::BLOB::Reference ref = Block::getBLOB();
			ibrcommon::MutexLock l(ref);
			(*ref).seekp(0);

			(*ref) >> _admfield;
			(*ref) >> _status;

			if ( _admfield & 0x01 )
			{
				(*ref) >> _fragment_offset;
				(*ref) >> _fragment_length;
			}

			(*ref) >> _timeof_receipt;
			(*ref) >> _timeof_custodyaccept;
			(*ref) >> _timeof_forwarding;
			(*ref) >> _timeof_delivery;
			(*ref) >> _timeof_deletion;
			(*ref) >> _bundle_timestamp;
			(*ref) >> _bundle_sequence;

			BundleString source;
			(*ref) >> source;
			_source = EID(source);
		}

		void StatusReportBlock::commit()
		{
			// read the attributes out of the BLOB
			ibrcommon::BLOB::Reference ref = Block::getBLOB();
			ibrcommon::MutexLock l(ref);
			ref.clear();

			(*ref).seekg(0);

			(*ref) << _admfield;
			(*ref) << _status;

			if ( _admfield & 0x01 )
			{
				(*ref) << _fragment_offset;
				(*ref) << _fragment_length;
			}

			(*ref) << _timeof_receipt;
			(*ref) << _timeof_custodyaccept;
			(*ref) << _timeof_forwarding;
			(*ref) << _timeof_delivery;
			(*ref) << _timeof_deletion;
			(*ref) << _bundle_timestamp;
			(*ref) << _bundle_sequence;
			(*ref) << BundleString(_source.getString());
		}
	}
}

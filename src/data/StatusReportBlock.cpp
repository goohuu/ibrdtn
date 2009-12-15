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
		 : PayloadBlock(ibrcommon::BLOBManager::BLOB_MEMORY), _admfield(16), _status(0), _reasoncode(0),
		 _fragment_offset(0), _fragment_length(0), _timeof_receipt(),
		 _timeof_custodyaccept(), _timeof_forwarding(), _timeof_delivery(),
		 _timeof_deletion(), _bundle_timestamp(0), _bundle_sequence(0)
		{
		}

		StatusReportBlock::StatusReportBlock(Block *block)
		 : PayloadBlock(block->getBLOBReference()), _admfield(16), _status(0), _reasoncode(0),
		 _fragment_offset(0), _fragment_length(0), _timeof_receipt(),
		 _timeof_custodyaccept(), _timeof_forwarding(), _timeof_delivery(),
		 _timeof_deletion(), _bundle_timestamp(0), _bundle_sequence(0)
		{
			read();
		}

		StatusReportBlock::StatusReportBlock(ibrcommon::BLOBReference ref)
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
			// read the attributes out of the BLOBReference
			ibrcommon::BLOBReference ref = Block::getBLOBReference();
			size_t remain = ref.getSize();
			char buffer[remain];

			// read the data into the buffer
			ref.read(buffer, 0, remain);

			// make a useable pointer
			char *data = buffer;

			SDNV value = 0;
			size_t len = 0;

			_admfield = data[0]; 	remain--; data++;
			_status = data[0]; 		remain--; data++;

			if ( _admfield & 0x01 )
			{
				len = _fragment_offset.decode(data, remain); remain -= len; data += len;
				len = _fragment_length.decode(data, remain); remain -= len; data += len;
			}

			len = _timeof_receipt.decode(data, remain); remain -= len; data += len;
			len = _timeof_custodyaccept.decode(data, remain); remain -= len; data += len;
			len = _timeof_forwarding.decode(data, remain); remain -= len; data += len;
			len = _timeof_delivery.decode(data, remain); remain -= len; data += len;
			len = _timeof_deletion.decode(data, remain); remain -= len; data += len;
			len = _bundle_timestamp.decode(data, remain); remain -= len; data += len;
			len = _bundle_sequence.decode(data, remain); remain -= len; data += len;
			len = value.decode(data, remain); remain -= len; data += len;

			string source;
			source.assign(data, remain);
			_source = EID(source);
		}

		void StatusReportBlock::commit()
		{
			stringstream ss;

			ss << _admfield;
			ss << _status;

			if ( _admfield & 0x01 )
			{
				ss << _fragment_offset;
				ss << _fragment_length;
			}

			ss << _timeof_receipt;
			ss << _timeof_custodyaccept;
			ss << _timeof_forwarding;
			ss << _timeof_delivery;
			ss << _timeof_deletion;
			ss << _bundle_timestamp;
			ss << _bundle_sequence;
			ss << BundleString(_source.getString());

			// clear the blob
			ibrcommon::BLOBReference ref = Block::getBLOBReference();
			ref.clear();

			// copy the content of ss to the blob
			string data = ss.str();
			ref.append(data.c_str(), data.length());
		}
	}
}

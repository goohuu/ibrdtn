/*
 * Block.cpp
 *
 *  Created on: 04.06.2009
 *      Author: morgenro
 */

#include "ibrdtn/data/Block.h"
#include "ibrdtn/streams/BundleStreamWriter.h"
#include <iostream>

using namespace std;

namespace dtn
{
	namespace data
	{
		Block::Block(char blocktype)
		 : _procflags(0), _blocktype(blocktype), _blobref(ibrcommon::StringBLOB::create()), _payload_offset(0)
		{
		}

		Block::Block(char blocktype, ibrcommon::BLOB::Reference blob)
		 : _procflags(0), _blocktype(blocktype), _blobref(blob), _payload_offset(0)
		{
		}

		Block::Block(Block *block)
		 : _procflags(block->_procflags), _blocktype(block->_blocktype), _blobref(block->_blobref), _payload_offset(block->_payload_offset)
		{
		}

		Block::~Block()
		{
		}

		void Block::addEID(EID eid)
		{
			_eids.push_back(eid);

			// add proc flag if not set
			if (!(_procflags & Block::BLOCK_CONTAINS_EIDS)) _procflags += Block::BLOCK_CONTAINS_EIDS;
		}

		list<EID> Block::getEIDList() const
		{
			return _eids;
		}

		ibrcommon::BLOB::Reference Block::getBLOB()
		{
			return _blobref;
		}

		size_t Block::writeHeader( dtn::streams::BundleWriter &writer ) const
		{
			size_t len = 0;
			len += writer.write(_blocktype);
			len += writer.write(_procflags);
			len += writer.write(_blobref.getSize());
			return len;
		}

		size_t Block::getHeaderSize(dtn::streams::BundleWriter &writer) const
		{
			size_t len = 0;

			len += writer.getSizeOf(_blocktype);
			len += writer.getSizeOf(_procflags);

//			for (std::list<dtn::data::EID>::const_iterator iter = _eids.begin(); iter != _eids.end(); iter++)
//			{
//				const dtn::data::EID &eid = (*iter);
//			}

			return len;
		}

		size_t Block::getSize() const
		{
			dtn::streams::BundleStreamWriter writer(cout);
			size_t len = 0;

			// add header size
			len += getHeaderSize(writer);

			// get the estimated size of the payload
			size_t psize = _blobref.getSize() - _payload_offset;

			len += writer.getSizeOf(psize);
			len += psize;

			return len;
		}

		size_t Block::write( dtn::streams::BundleWriter &writer )
		{
			size_t len = 0;
			len += writeHeader( writer );

			ibrcommon::MutexLock l(_blobref);

			// jump to the offset position of the blob
			(*_blobref).seekg(_payload_offset);

			// write the payload to the stream
			len += writer.write( (*_blobref) );

			return len;
		}

		void Block::setOffset(size_t offset) throw (PayloadTooSmallException)
		{
			if (offset > _blobref.getSize()) throw PayloadTooSmallException();

			_payload_offset = offset;
		}
	}
}

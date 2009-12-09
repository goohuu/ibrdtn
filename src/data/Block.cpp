/*
 * Block.cpp
 *
 *  Created on: 04.06.2009
 *      Author: morgenro
 */

#include "ibrdtn/data/Block.h"
#include "ibrdtn/streams/BundleStreamWriter.h"
#include "ibrcommon/data/BLOBManager.h"
#include <iostream>

using namespace std;

namespace dtn
{
	namespace data
	{
		Block::Block(char blocktype)
		 : _procflags(0), _blocktype(blocktype), _blobref(ibrcommon::BLOBManager::_instance.create())
		{
		}

		Block::Block(char blocktype, ibrcommon::BLOBManager::BLOB_TYPE type)
		 : _procflags(0), _blocktype(blocktype), _blobref(ibrcommon::BLOBManager::_instance.create(type))
		{
		}

		Block::Block(char blocktype, ibrcommon::BLOBReference ref)
		: _procflags(0), _blocktype(blocktype), _blobref(ref)
		{
		}

		Block::~Block()
		{
		}

		void Block::addEID(EID eid)
		{
			_eids.push_back(eid);
		}

		list<EID> Block::getEIDList() const
		{
			return _eids;
		}

		ibrcommon::BLOBReference Block::getBLOBReference()
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

		size_t Block::getSize() const
		{
			dtn::streams::BundleStreamWriter writer(cout);
			size_t len = 0;

			len += writer.getSizeOf(_blocktype);
			len += writer.getSizeOf(_procflags);
			len += writer.getSizeOf(_blobref.getSize());
			len += _blobref.getSize();

			return len;
		}

		size_t Block::write( dtn::streams::BundleWriter &writer )
		{
			size_t len = 0;
			len += writeHeader( writer );
			len += writer.write( _blobref );
			return len;
		}
	}
}

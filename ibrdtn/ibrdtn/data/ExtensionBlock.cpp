/*
 * ExtensionBlock.cpp
 *
 *  Created on: 26.05.2010
 *      Author: morgenro
 */

#include "ibrdtn/data/ExtensionBlock.h"
#include "ibrdtn/data/Exceptions.h"
#include <ibrcommon/thread/MutexLock.h>

namespace dtn
{
	namespace data
	{
		ExtensionBlock::ExtensionBlock()
		 : Block(0), _blobref(ibrcommon::TmpFileBLOB::create())
		{
		}

		ExtensionBlock::ExtensionBlock(ibrcommon::BLOB::Reference ref)
		 : Block(0), _blobref(ref)
		{
		}

		ExtensionBlock::~ExtensionBlock()
		{
		}

		ibrcommon::BLOB::Reference ExtensionBlock::getBLOB()
		{
			return _blobref;
		}

		size_t ExtensionBlock::getLength() const
		{
			ibrcommon::BLOB::Reference blobref = _blobref;
			ibrcommon::BLOB::iostream io = blobref.iostream();
			return blobref.getSize();
		}

		std::ostream& ExtensionBlock::serialize(std::ostream &stream) const
		{
			ibrcommon::BLOB::Reference blobref = _blobref;
			ibrcommon::BLOB::iostream io = blobref.iostream();

			try {
				ibrcommon::BLOB::copy(stream, *io, blobref.getSize());
			} catch (const ibrcommon::IOException &ex) {
				throw dtn::SerializationFailedException(ex.what());
			}

			return stream;
		}

		std::istream& ExtensionBlock::deserialize(std::istream &stream)
		{
			// lock the BLOB
			ibrcommon::BLOB::iostream io = _blobref.iostream();

			// clear the blob
			_blobref.clear();

			// check if the blob is ready
			if (!(*io).good()) throw dtn::SerializationFailedException("could not open BLOB for payload");

			try {
				ibrcommon::BLOB::copy(*io, stream, _blocksize);
			} catch (const ibrcommon::IOException &ex) {
				throw dtn::SerializationFailedException(ex.what());
			}

			// set block not processed bit
			set(dtn::data::Block::FORWARDED_WITHOUT_PROCESSED, true);

			return stream;
		}
	}
}

/*
 * PayloadBlock.cpp
 *
 *  Created on: 29.05.2009
 *      Author: morgenro
 */

#include "ibrdtn/data/PayloadBlock.h"
#include "ibrdtn/data/Exceptions.h"
#include <ibrcommon/thread/MutexLock.h>

namespace dtn
{
	namespace data
	{
		PayloadBlock::PayloadBlock()
		 : Block(PayloadBlock::BLOCK_TYPE), _blobref(ibrcommon::TmpFileBLOB::create())
		{
		}

		PayloadBlock::PayloadBlock(ibrcommon::BLOB::Reference ref)
		 : Block(PayloadBlock::BLOCK_TYPE), _blobref(ref)
		{
		}

		PayloadBlock::~PayloadBlock()
		{
		}

		ibrcommon::BLOB::Reference PayloadBlock::getBLOB() const
		{
			return _blobref;
		}

		const size_t PayloadBlock::getLength() const
		{
			return _blobref.getSize();
		}

		std::ostream& PayloadBlock::serialize(std::ostream &stream) const
		{
			ibrcommon::BLOB::Reference blobref = _blobref;
			ibrcommon::MutexLock l(blobref);

			// write payload
			stream << (*blobref).rdbuf();

			return stream;
		}

		std::istream& PayloadBlock::deserialize(std::istream &stream)
		{
			// clear the blob
			_blobref.clear();

			ibrcommon::MutexLock l(_blobref);

			// read payload
			const int buffer_size = 0x1000;
			char buffer[buffer_size];
			size_t ret = 1;
			ssize_t remain = _blocksize;

			while (remain > 0)
			{
				if (remain > buffer_size)
				{
					stream.read(buffer, buffer_size);
				}
				else
				{
					stream.read(buffer, remain);
				}

				(*_blobref).write(buffer, stream.gcount());

				remain -= stream.gcount();

				if (stream.eof()) throw dtn::exceptions::IOException("block not complete");
			}

			// unset block not processed bit
			set(dtn::data::Block::FORWARDED_WITHOUT_PROCESSED, false);

			return stream;
		}
	}
}

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

		const size_t ExtensionBlock::getLength() const
		{
			return _blobref.getSize();
		}

		std::ostream& ExtensionBlock::serialize(std::ostream &stream) const
		{
			ibrcommon::BLOB::Reference blobref = _blobref;
			ibrcommon::MutexLock l(blobref);

			// write payload
			(*blobref).seekg(0);
			stream << (*blobref).rdbuf();

			return stream;
		}

		std::istream& ExtensionBlock::deserialize(std::istream &stream)
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

			// set block not processed bit
			set(dtn::data::Block::FORWARDED_WITHOUT_PROCESSED, true);

			return stream;
		}
	}
}

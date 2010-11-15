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
			ibrcommon::MutexLock l(blobref);
			return blobref.getSize();
		}

		std::ostream& ExtensionBlock::serialize(std::ostream &stream) const
		{
			ibrcommon::BLOB::Reference blobref = _blobref;
			ibrcommon::MutexLock l(blobref);

			// remember the old exceptions state
			std::ios::iostate oldstate = (*blobref).exceptions();

			// activate exceptions for this method
			(*blobref).exceptions(std::ios::badbit | std::ios::eofbit);

			try {
				// write payload
				stream << (*blobref).rdbuf();
			} catch (std::exception &ex) {
				// restore the old state
				(*blobref).exceptions(oldstate);

				throw dtn::SerializationFailedException(ex.what());
			}

			// restore the old state
			(*blobref).exceptions(oldstate);

			return stream;
		}

		std::istream& ExtensionBlock::deserialize(std::istream &stream)
		{
			// lock the BLOB
			ibrcommon::MutexLock l(_blobref);

			// clear the blob
			_blobref.clear();

			// check if the blob is ready
			if (!(*_blobref).good())
			{
				throw dtn::SerializationFailedException("could not open BLOB for payload");
			}

			// read payload
			const int buffer_size = 0x1000;
			char buffer[buffer_size];
			ssize_t remain = _blocksize;

			while (remain > 0)
			{
				// check if the reading stream is ok
				if (stream.eof()) throw dtn::SerializationFailedException("stream reached EOF while reading payload");

				// read the full buffer size of less?
				if (remain > buffer_size)
				{
					stream.read(buffer, buffer_size);
				}
				else
				{
					stream.read(buffer, remain);
				}

				// write the bytes to the BLOB
				(*_blobref).write(buffer, stream.gcount());

				// shrink the remaining bytes by the red bytes
				remain -= stream.gcount();
			}

			// set block not processed bit
			set(dtn::data::Block::FORWARDED_WITHOUT_PROCESSED, true);

			return stream;
		}
	}
}

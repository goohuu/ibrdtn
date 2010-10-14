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

		size_t PayloadBlock::getLength() const
		{
			ibrcommon::BLOB::Reference blobref = _blobref;
			ibrcommon::MutexLock l(blobref);
			return blobref.getSize();
		}

		std::ostream& PayloadBlock::serialize(std::ostream &stream) const
		{
			ibrcommon::BLOB::Reference blobref = _blobref;
			ibrcommon::MutexLock l(blobref);

			// remember the old exceptions state
			std::ios::iostate oldstate = (*blobref).exceptions();

			// activate exceptions for this method
			(*blobref).exceptions(std::ios::badbit | std::ios::failbit | std::ios::eofbit);

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

		std::istream& PayloadBlock::deserialize(std::istream &stream)
		{
			ibrcommon::MutexLock l(_blobref);

			// clear the blob
			_blobref.clear();

			// remember the old exceptions state
			std::ios::iostate oldstate = (*_blobref).exceptions();

			// activate exceptions for this method
			(*_blobref).exceptions(std::ios::badbit | std::ios::failbit | std::ios::eofbit);

			try {
				// read payload
				const int buffer_size = 0x1000;
				char buffer[buffer_size];
				ssize_t remain = _blocksize;

				while (remain > 0 && stream.good())
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
				}
			} catch (std::exception &ex) {
				// restore the old state
				(*_blobref).exceptions(oldstate);

				throw dtn::SerializationFailedException(ex.what());
			}

			// restore the old state
			(*_blobref).exceptions(oldstate);

			// unset block not processed bit
			set(dtn::data::Block::FORWARDED_WITHOUT_PROCESSED, false);

			return stream;
		}
	}
}

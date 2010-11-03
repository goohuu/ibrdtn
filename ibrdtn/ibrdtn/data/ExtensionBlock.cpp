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

			// lock the BLOB
			ibrcommon::MutexLock l(_blobref);

			// remember the old exceptions state
			std::ios::iostate oldstate = (*_blobref).exceptions();

			// activate exceptions for this method
			(*_blobref).exceptions(std::ios::badbit | std::ios::eofbit);

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
			} catch (std::ios_base::failure ex) {
				throw dtn::SerializationFailedException();
			} catch (...) {
				// restore the old state
				(*_blobref).exceptions(oldstate);

				throw;
			}

			// restore the old state
			(*_blobref).exceptions(oldstate);

			// set block not processed bit
			set(dtn::data::Block::FORWARDED_WITHOUT_PROCESSED, true);

			return stream;
		}
	}
}

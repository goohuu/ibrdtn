/*
 * PayloadBlock.cpp
 *
 *  Created on: 29.05.2009
 *      Author: morgenro
 */

#include "ibrdtn/data/PayloadBlock.h"
#include "ibrdtn/data/Exceptions.h"

namespace dtn
{
	namespace data
	{
		PayloadBlock::PayloadBlock()
		 : Block(PayloadBlock::BLOCK_TYPE)
		{
		}

		PayloadBlock::PayloadBlock(ibrcommon::BLOB::Reference ref)
		 : Block(PayloadBlock::BLOCK_TYPE, ref)
		{
		}

		PayloadBlock::~PayloadBlock()
		{
		}

//		std::pair<PayloadBlock*, PayloadBlock*> PayloadBlock::split(size_t position)
//		{
//			throw dtn::exceptions::NotImplementedException("Fragmentation is not supported.");
//		}

		void PayloadBlock::setFragment(size_t offset) throw (dtn::exceptions::FragmentationException)
		{
			dtn::streams::BundleStreamWriter writer(cout);
			size_t hsize = getHeaderSize(writer);

			if (hsize <= offset) throw dtn::exceptions::FragmentationException("Fragmentation at this position is not possible.");

			try {
				setOffset(offset - hsize);
			} catch (Block::PayloadTooSmallException ex) {
				throw dtn::exceptions::FragmentationException("Offset outside of the payload area");
			}
		}
	}
}

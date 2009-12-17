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

		std::pair<PayloadBlock*, PayloadBlock*> PayloadBlock::split(size_t position)
		{
			throw dtn::exceptions::NotImplementedException("Fragmentation is not supported.");
		}
	}
}

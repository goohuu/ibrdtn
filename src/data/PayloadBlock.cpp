/*
 * PayloadBlock.cpp
 *
 *  Created on: 29.05.2009
 *      Author: morgenro
 */

#include "ibrdtn/data/PayloadBlock.h"

namespace dtn {
namespace data
{
	PayloadBlock::PayloadBlock()
	 : Block(PayloadBlock::BLOCK_TYPE)
	{
	}

	PayloadBlock::PayloadBlock(blob::BLOBManager::BLOB_TYPE type)
	 : Block(PayloadBlock::BLOCK_TYPE, type)
	{
	}

	PayloadBlock::PayloadBlock(BLOBReference ref)
	 : Block(PayloadBlock::BLOCK_TYPE, ref)
	{
	}

	PayloadBlock::~PayloadBlock()
	{
	}
}
}

/*
 * DiscoverBlockFactory.h
 *
 *  Created on: 20.02.2009
 *      Author: morgenro
 */

#ifndef DISCOVERBLOCKFACTORY_H_
#define DISCOVERBLOCKFACTORY_H_

#include "data/Block.h"
#include "data/BlockFactory.h"
#include "emma/DiscoverBlock.h"

using namespace dtn::data;

namespace emma
{
	class DiscoverBlockFactory : public BlockFactory
	{
	public:
		DiscoverBlockFactory();

		/**
		 * Parse existing data for fields of this block.
		 * @param[in] data Data-array to parse.
		 * @param[in] size The length of the data-array.
		 * @return The consumed bytes of the data-array.
		 */
		Block* parse(const unsigned char *data, unsigned int size);

		/**
		 * Copy a existing block to a new block
		 * @param[in] block The block to copy.
		 * @return A copy of the given block.
		 */
		Block* copy(const Block &block);

		static DiscoverBlock* newDiscoverBlock();

		char getBlockType() const;
	};
}

#endif /* DISCOVERBLOCKFACTORY_H_ */

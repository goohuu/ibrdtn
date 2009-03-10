/*
 * BlockFactory.h
 *
 *  Created on: 19.02.2009
 *      Author: morgenro
 */

#include "Block.h"

#ifndef BLOCKFACTORY_H_
#define BLOCKFACTORY_H_

namespace dtn
{
	namespace data
	{
		/**
		 * This is a factory for Block objects.
		 */
		class BlockFactory
		{
		public:
			/**
			 * Constuctor for this BlockFactory
			 */
			BlockFactory();

			/**
			 * Parse existing data for fields of this block.
			 * @param[in] data Data-array to parse.
			 * @param[in] size The length of the data-array.
			 * @return The consumed bytes of the data-array.
			 */
			virtual Block* parse(const unsigned char *data, unsigned int size);

			/**
			 * Copy a existing block to a new block
			 * @param[in] block The block to copy.
			 * @return A copy of the given block.
			 */
			virtual Block* copy(const Block &block);

			/**
			 * Create a new empty block.
			 * @return A empty Block object.
			 */
			static Block* newBlock();

			virtual char getBlockType() const;
		};
	}
}


#endif /* BLOCKFACTORY_H_ */

#ifndef PAYLOADBLOCK_H_
#define PAYLOADBLOCK_H_

#include <stdio.h>

#include "data/Block.h"
#include "data/BlockFlags.h"
#include "data/Exceptions.h"

namespace dtn
{
	namespace data
	{
		/**
		 * A PayloadBlock can be attached to a bundle and carry application payload.
		 * Additional this class is used as base for custody signals and status reports.
		 */
		class PayloadBlock : public data::Block
		{
		public:
			static const unsigned char BLOCK_TYPE = 1;

			PayloadBlock(NetworkFrame *frame);
			PayloadBlock(Block *block);

			/**
			 * destructor
			 */
			virtual ~PayloadBlock();

			/**
			 * Return a pointer to the payload of this block.
			 * @return A pointer to the payload.
			 */
			unsigned char* getPayload() const;

			/**
			 * Set the payload of this PayloadBlock. It copy the given data to the existing data array of the bundle.
			 */
			void setPayload(const unsigned char *data, unsigned int size);

			/**
			 * Get the range of the payload in the data array.
			 * e.g. if the payload data begins at 15th byte and has a size of 64 byte, then the pair <15, 79> is returned.
			 * @return a pair of two unsigned integer
			 */
			pair<unsigned int, unsigned int> getPayloadRange() const;

			/**
			 * Return the length of the payload.
			 * @return The length of the payload.
			 */
			unsigned int getLength() const;
		};
	}
}

#endif /*PAYLOADBLOCK_H_*/

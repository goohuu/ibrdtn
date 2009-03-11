/*
 * BlockFactory.cpp
 *
 *  Created on: 19.02.2009
 *      Author: morgenro
 */

#include "data/BlockFactory.h"
#include "data/SDNV.h"
#include "data/Exceptions.h"

using namespace dtn::exceptions;

namespace dtn
{
namespace data
	{
		BlockFactory::BlockFactory()
		{
		}

		char BlockFactory::getBlockType() const
		{
			return Block::BLOCK_TYPE;
		}

		Block* BlockFactory::newBlock()
		{
			NetworkFrame *frame = new NetworkFrame();

			// the block type
			frame->append(0);

			// processing flags
			frame->append(0);

			// block length
			frame->append(0);

			return new Block(frame);
		}

		Block* BlockFactory::parse(const unsigned char *data, unsigned int size)
		{
			// copy the data pointer
			const unsigned char *datap = data;

			// block length
			u_int64_t block_length = 0;

			// create a new mapping for the fields
			map<unsigned int, unsigned int> mapping;

			// we start with position zero
			unsigned int position = 0;

			// the first field is the block type
			mapping[position] = 1;
			position++;

			// move the data pointer forward
			data++;
			block_length++;

			// sdnv value for decoding
			unsigned int value = 0;

			// decode block flags
			int ret = SDNV::decode(data, SDNV::len(data), &value);

			// check for error
			if (ret == -1) throw InvalidBundleData("Error while decoding a SDNV.");
			if (ret > (size - block_length)) throw InvalidBundleData("Decoded SDNV is too long.");

			// remind the length of this SDNV
			mapping[1] = ret;
			position++;

			// create a BlockFlags object
			BlockFlags blockflags = BlockFlags(value);

			// set the data pointer
			data += ret;
			block_length += ret;

			// check for eid references
			if ( blockflags.getFlag(CONTAINS_EID_FIELD) )
			{
				// read the count of eid references
				ret = SDNV::decode(data, SDNV::len(data), &value);

				// check for error
				if (ret == -1) throw InvalidBundleData("Error while decoding a SDNV.");
				if (ret > (size - block_length)) throw InvalidBundleData("Decoded SDNV is too long.");

				// remind the length of this SDNV
				mapping[position] = ret;
				position++;

				for (uint64_t i = 0; i < (value * 2); i++)
				{
					int len = SDNV::len(data);

					mapping[position] = len;
					position++;

					data += len;
					block_length += len;
				}
			}

			// Decodiere die Länge der block-spezifischen Daten
			ret = SDNV::decode(data, SDNV::len(data), &value);

			// check for error
			if (ret == -1) throw InvalidBundleData("Error while decoding a SDNV.");
			if (ret > (size - block_length)) throw InvalidBundleData("Decoded SDNV is too long.");

			// Merke die Länge des Payloadlängenfeld
			mapping[position] = ret;
			position++;

			data += ret;	// Springe zu den block-spezifischen Daten
			block_length += ret;

			if (value > (size - block_length)) throw InvalidBundleData("Decoded block length is too long.");

			// put the length of the last field in the mapping
			mapping[position] = value;

			// add the size to the block length
			block_length += value;

			// create a new NetworkFrame.
			NetworkFrame *frame = new NetworkFrame(datap, block_length);

			// set the mapping to the frame
			frame->setFieldSizeMap(mapping);

			return new Block(frame);
		}

		Block* BlockFactory::copy(const Block &block)
		{
			NetworkFrame *frame = new NetworkFrame(block.getFrame());
			return new Block(frame);
		}
	}
}

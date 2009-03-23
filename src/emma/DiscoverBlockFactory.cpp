/*
 * DiscoverBlockFactory.cpp
 *
 *  Created on: 20.02.2009
 *      Author: morgenro
 */

#include "emma/DiscoverBlockFactory.h"
#include "emma/DiscoverBlock.h"
#include "data/SDNV.h"

namespace emma
{
	DiscoverBlockFactory::DiscoverBlockFactory()
	{

	}

	char DiscoverBlockFactory::getBlockType() const
	{
		return DiscoverBlock::BLOCK_TYPE;
	}

	Block* DiscoverBlockFactory::parse(const unsigned char *data, unsigned int size)
	{
		// initially parse the base block
		Block *block = BlockFactory().parse(data, size);

		// create a payload block out of this block
		DiscoverBlock *discoverblock = new DiscoverBlock(block);

		NetworkFrame &frame = discoverblock->getFrame();

		// get the field mapping of the frame
		map<unsigned int, unsigned int> &mapping = frame.getFieldSizeMap();

		// field index of the body
		unsigned int position = mapping.size() - 1;

		// get payload for parsing
		unsigned char *datap = frame.get(position);

		// current field size
		unsigned int len = 0;

		// port
		len = SDNV::len(datap);
		mapping[position] = len;
		position++;
		datap += len;

		// length of address
		len = SDNV::len(datap);
		mapping[position] = len;
		position++;
		datap += len;

		// address
		mapping[position] = frame.getSDNV(position - 1);
		datap += frame.getSDNV(position - 1);
		position++;

		// optionals
		len = SDNV::len(datap);
		mapping[position] = len;
		position++;
		datap += len;

		if ( frame.getSDNV(position - 1) > 1 )
		{
			// latitude (double)
			mapping[position] = sizeof(double);
			position++;
			datap += sizeof(double);

			// longitude (double)
			mapping[position] = sizeof(double);
			position++;
			datap += sizeof(double);
		}

		// update the size of the frame
		frame.updateSize();

		return discoverblock;
	}

	Block* DiscoverBlockFactory::copy(const Block &block)
	{
		NetworkFrame *frame = new NetworkFrame(block.getFrame());
		return new DiscoverBlock(frame);
	}

	DiscoverBlock* DiscoverBlockFactory::newDiscoverBlock()
	{
		// create a empty DiscoverBlock
		DiscoverBlock *block = new DiscoverBlock( BlockFactory::newBlock() );

		// get the NetworkFrame object of the block
		NetworkFrame &frame = block->getFrame();

		// set block type
		frame.set(0, (unsigned char)DiscoverBlock::BLOCK_TYPE);

		// set DISCARD_IF_CANT_PROCESSED flag
		block->getBlockFlags().setFlag(DISCARD_IF_CANT_PROCESSED, true);

		// append port number
		frame.append( 4556 );

		// address placeholder
		string address = "!dtn";

		// length of the address placeholder
		frame.append( address.length() );
		frame.append((unsigned char*)address.c_str(), address.length());

		// currently no optional fields
		frame.append( (unsigned int)0 );

		// update block size
		block->updateBlockSize();

		return block;
	}
}

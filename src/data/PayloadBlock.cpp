#include "data/PayloadBlock.h"
#include "data/SDNV.h"
#include "data/BundleFactory.h"

namespace dtn
{
namespace data
{
	PayloadBlock::PayloadBlock(NetworkFrame *frame) : Block(frame)
	{
		this->m_processed = true;
	}

	PayloadBlock::PayloadBlock(Block *block) : Block(block)
	{
		this->m_processed = true;
	}

	PayloadBlock::~PayloadBlock()
	{
	}

	void PayloadBlock::setPayload(const unsigned char *data, unsigned int size)
	{
		unsigned int position = Block::getBodyIndex();

		NetworkFrame &frame = Block::getFrame();

		// Setze neue Bodylänge
		frame.set(position - 1, size);

		// Setze neuen Bodyinhalt
		frame.set(position, data, size);
	}

	unsigned char* PayloadBlock::getPayload() const
	{
		NetworkFrame &frame = Block::getFrame();
		return frame.get( Block::getBodyIndex() );
	}

	unsigned int PayloadBlock::getLength() const
	{
		NetworkFrame &frame = Block::getFrame();
		return frame.getSDNV( Block::getBodyIndex() - 1 );
	}

	pair<unsigned int, unsigned int> PayloadBlock::getPayloadRange() const
	{
		NetworkFrame &frame = Block::getFrame();
		unsigned int start_field = Block::getBodyIndex();
		unsigned int start = 0;

		for (unsigned int i = 0; i < start_field; i++)
		{
			start += frame.getSize(i);
		}

		unsigned int stop = start + frame.getSize(start_field);

		return make_pair(start, stop);
	}
}
}

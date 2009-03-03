#include "data/Block.h"

#include "data/BundleFactory.h"
#include "data/Exceptions.h"
#include "data/SDNV.h"
#include "data/Bundle.h"

namespace dtn
{
	namespace data
	{
		Block::Block(Block *block) : m_frame(block->m_frame)
		{
			block->dissociateNetworkFrame();
			delete block;
		}

		Block::Block(NetworkFrame *frame) : m_frame(frame)
		{
		}

		Block::~Block()
		{
			if (m_frame != NULL) delete m_frame;
		}

		unsigned int Block::getBodyIndex() const
		{
			// BlockType + Processing Fields
			unsigned int position = 3;

			if ( getBlockFlags().getFlag(CONTAINS_EID_FIELD) )
			{
				// Addiere die Anzahl der EIDs
				position += m_frame->getSDNV( 2 ) * 2;

				// and the field for the eid count
				position++;
			}

			return position;
		}

		NetworkFrame& Block::getFrame() const
		{
			return *m_frame;
		}

		NetworkFrame* Block::dissociateNetworkFrame()
		{
			NetworkFrame *frame = m_frame;
			m_frame = NULL;
			return frame;
		}

		BlockFlags Block::getBlockFlags() const
		{
			// Hole die Block Processing Flags
			BlockFlags blockflags( m_frame->getSDNV(1) );
			return blockflags;
		}

		void Block::setBlockFlags(BlockFlags flags)
		{
			m_frame->set(1, flags.getValue() );
		}

		unsigned char Block::getType() const
		{
			return m_frame->getChar( 0 );
		}

		unsigned int Block::getHeaderSize() const
		{
			unsigned int begin = 0;
			unsigned int end = getBodyIndex();
			unsigned int size = 0;

			for (unsigned int i = begin; i < end; i++)
			{
				size += m_frame->getSize(i);
			}

			return size;
		}

		void Block::updateBlockSize()
		{
			NetworkFrame &frame = Block::getFrame();
			size_t maxfield = frame.getFieldSizeMap().size() - 1;

			unsigned int size = 0;
			unsigned int position = Block::getBodyIndex();

			for (unsigned int i = position; i <= maxfield; i++)
			{
				size += frame.getSize(i);
			}

			// Setze neue Bodylänge
			frame.set(Block::getBodyIndex() - 1, size);
		}
	}
}

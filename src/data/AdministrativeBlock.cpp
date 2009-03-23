#include "data/AdministrativeBlock.h"
#include "data/ProcessingFlags.h"
#include "data/SDNV.h"

namespace dtn
{
	namespace data
	{
		AdministrativeBlock::AdministrativeBlock(NetworkFrame *frame, AdministrativeBlockType type) : PayloadBlock(frame), m_type(type)
		{
			// get the field mapping of the block
			map<unsigned int, unsigned int> &mapping = frame->getFieldSizeMap();

			// first field: Administrative TypeCode 4bit + RecordFlags 4bit
			mapping[mapping.size() - 1] = 1;
		}

		AdministrativeBlock::AdministrativeBlock(Block *block, AdministrativeBlockType type) : PayloadBlock(block), m_type(type)
		{

		}

		AdministrativeBlock::~AdministrativeBlock()
		{
		}

		AdministrativeBlockType AdministrativeBlock::getAdministrativeType() const
		{
			return m_type;
		}

		ProcessingFlags AdministrativeBlock::getStatusFlags() const
		{
			unsigned char status = (unsigned int)getFrame().getChar( getBodyIndex() );

			ProcessingFlags flags( (unsigned int)status );
			return flags;
		}

		void AdministrativeBlock::setStatusFlags(ProcessingFlags flags)
		{
			unsigned char value = (unsigned char)flags.getValue();

			getFrame().set( getBodyIndex(), value );
		}

		bool AdministrativeBlock::isAdministrativeBlock() const
		{
			return true;
		}

		AdministrativeBlockType AdministrativeBlock::identify(PayloadBlock *block)
		{
			unsigned char* data = block->getPayload();
			AdministrativeBlockType type = AdministrativeBlockType(data[0] >> 4);
			return type;
		}
	}
}

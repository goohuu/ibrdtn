#include "data/CustodySignalBlock.h"
#include "data/SDNV.h"
#include "data/BundleFactory.h"
#include "data/ProcessingFlags.h"
#include <cstdlib>
#include <cstring>


namespace dtn
{
	namespace data
	{
		CustodySignalBlock::CustodySignalBlock(Block *block) : AdministrativeBlock(block, CUSTODY_SIGNAL)
		{

		}

		CustodySignalBlock::CustodySignalBlock(NetworkFrame *frame) : AdministrativeBlock(frame, CUSTODY_SIGNAL)
		{
			map<unsigned int, unsigned int> &mapping = frame->getFieldSizeMap();

			// get payload for parsing
			unsigned char *data = PayloadBlock::getPayload();

			// field index of the body
			unsigned int position = Block::getBodyIndex();

			// current field length
			unsigned int len = 0;

			// first field is administrative TypeCode 4bit + RecordFlags 4bit
			mapping[position] = 1;
			position++;
			data++;

			// Status Flags (1 bit) + 7bit Reason Code
			mapping[position] = 1;
			position++;
			data++;

			// start at time of signal field for decoding
			unsigned int fields = CUSTODY_TIMEOFSIGNAL;

			// start 2 field earlier if we have fragmentation fields
			if ( forFragment() )
			{
				fields -= 2;
			}

			for (int i = fields; i < CUSTODY_BUNDLE_SOURCE; i++)
			{
				len = SDNV::len(data);
				mapping[position] = len;
				position++;
				data += len;
			}

			// the length of the next field is the value of the previous field
			mapping[position] = frame->getSDNV(position - 1);

			// update the size of the frame
			frame->updateSize();
		}

		CustodySignalBlock::~CustodySignalBlock()
		{
		}

		unsigned int CustodySignalBlock::getField(CUSTODY_FIELDS field) const
		{
			unsigned int ret = getBodyIndex();

			if ( (field > CUSTODY_FRAGMENT_LENGTH) && (!forFragment()) )
			{
				ret -= 2;
			}

			return ret + field;
		}

		unsigned int CustodySignalBlock::getFragmentOffset() const
		{
			if ( !forFragment() ) return 0;
			return getFrame().getSDNV( getField(CUSTODY_FRAGMENT_OFFSET) );
		}

		void CustodySignalBlock::setFragmentOffset(unsigned int value)
		{
			NetworkFrame &frame = getFrame();

			if ( !forFragment() )
			{
				// create fragmentation fields
				frame.insert(getField(CUSTODY_FRAGMENT_OFFSET));
				frame.insert(getField(CUSTODY_FRAGMENT_LENGTH));

				// set the fragmentation flag
				ProcessingFlags flags = getStatusFlags();
				flags.setFlag(0, true);
				setStatusFlags(flags);
			}

			frame.set(getField(CUSTODY_FRAGMENT_OFFSET), value);
		}

		unsigned int CustodySignalBlock::getFragmentLength() const
		{
			if ( !forFragment() ) return 0;
			return getFrame().getSDNV( getField(CUSTODY_FRAGMENT_LENGTH) );
		}

		void CustodySignalBlock::setFragmentLength(unsigned int value)
		{
			NetworkFrame &frame = getFrame();

			if ( !forFragment() )
			{
				// create fragmentation fields
				frame.insert(getField(CUSTODY_FRAGMENT_OFFSET));
				frame.insert(getField(CUSTODY_FRAGMENT_LENGTH));

				// set the fragmentation flag
				ProcessingFlags flags = getStatusFlags();
				flags.setFlag(0, true);
				setStatusFlags(flags);
			}

			frame.set(getField(CUSTODY_FRAGMENT_LENGTH), value);
		}

		bool CustodySignalBlock::forFragment() const
		{
			return getStatusFlags().getFlag( 0 );
		}

		bool CustodySignalBlock::isAccepted() const
		{
			NetworkFrame &frame = Block::getFrame();

			// get status field
			ProcessingFlags flags( (unsigned int)frame.getChar( getField(CUSTODY_STATUS) ));

			// The first bit tells if custody is accepted or not
			return flags.getFlag( 0 );
		}

		void CustodySignalBlock::setAccepted(bool value)
		{
			NetworkFrame &frame = Block::getFrame();

			// get status field
			ProcessingFlags flags( (unsigned int)frame.getChar( getField(CUSTODY_STATUS) ));

			// The first bit tells if custody is accepted or not
			flags.setFlag( 0, value );

			frame.set( getField(CUSTODY_STATUS), (char)flags.getValue() );
		}

		unsigned int CustodySignalBlock::getTimeOfSignal() const
		{
			NetworkFrame &frame = Block::getFrame();
			return frame.getSDNV( getField(CUSTODY_TIMEOFSIGNAL) );
		}

		void CustodySignalBlock::setTimeOfSignal(unsigned int value)
		{
			NetworkFrame &frame = Block::getFrame();
			frame.set( getField(CUSTODY_TIMEOFSIGNAL), value );
		}

		unsigned int CustodySignalBlock::getCreationTimestamp() const
		{
			NetworkFrame &frame = Block::getFrame();
			return frame.getSDNV( getField(CUSTODY_BUNDLE_TIMESTAMP) );
		}

		void CustodySignalBlock::setCreationTimestamp(unsigned int value)
		{
			NetworkFrame &frame = Block::getFrame();
			frame.set( getField(CUSTODY_BUNDLE_TIMESTAMP), value );
		}

		unsigned int CustodySignalBlock::getCreationTimestampSequence() const
		{
			NetworkFrame &frame = Block::getFrame();
			return frame.getSDNV( getField(CUSTODY_BUNDLE_SEQUENCE) );
		}

		void CustodySignalBlock::setCreationTimestampSequence(unsigned int value)
		{
			NetworkFrame &frame = Block::getFrame();
			frame.set( getField(CUSTODY_BUNDLE_SEQUENCE), value );
		}

		string CustodySignalBlock::getSource() const
		{
			NetworkFrame &frame = Block::getFrame();
			size_t field = getField(CUSTODY_BUNDLE_SOURCE);
			return frame.getString(field);
		}

		void CustodySignalBlock::setSource(string value)
		{
			NetworkFrame &frame = Block::getFrame();
			frame.set(getField(CUSTODY_BUNDLE_SOURCE_LENGTH), value.length());
			frame.set(getField(CUSTODY_BUNDLE_SOURCE), value);
		}

		bool CustodySignalBlock::match(const Bundle &b) const
		{
			if ( b.getPrimaryFlags().isFragment() )
			{
				if ( b.getInteger( FRAGMENTATION_OFFSET ) != getFragmentOffset() ) return false;
				if ( b.getInteger( APPLICATION_DATA_LENGTH ) != getFragmentLength() ) return false;
			}

			if ( b.getInteger( CREATION_TIMESTAMP ) != getCreationTimestamp() ) return false;
			if ( b.getInteger( CREATION_TIMESTAMP_SEQUENCE ) != getCreationTimestampSequence() ) return false;
			if ( b.getSource() != getSource() ) return false;

			return true;
		}

		void CustodySignalBlock::setMatch(const Bundle &b)
		{
			if (b.getPrimaryFlags().isFragment())
			{
				setFragmentOffset( b.getInteger(FRAGMENTATION_OFFSET) );
				setFragmentLength( b.getInteger(APPLICATION_DATA_LENGTH) );
			}
			setCreationTimestamp( b.getInteger(CREATION_TIMESTAMP) );
			setCreationTimestampSequence( b.getInteger(CREATION_TIMESTAMP_SEQUENCE) );
			setSource( b.getSource() );

			updateBlockSize();
		}
	}
}

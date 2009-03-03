#include "data/StatusReportBlock.h"
#include "data/SDNV.h"
#include "data/BundleFactory.h"
#include <cstdlib>
#include <cstring>


namespace dtn
{
	namespace data
	{
		StatusReportBlock::StatusReportBlock(Block *block) : AdministrativeBlock(block, STATUS_REPORT)
		{

		}

		StatusReportBlock::StatusReportBlock(NetworkFrame *frame) : AdministrativeBlock(frame, STATUS_REPORT)
		{
			// Feldmapping des Subframe zum bearbeiten holen
			map<unsigned int, unsigned int> &mapping = frame->getFieldSizeMap();

			// get payload for parsing
			unsigned char *data = PayloadBlock::getPayload();

			// field index of the body
			unsigned int position = Block::getBodyIndex();

			// Aktuelle Feldlänge
			unsigned int len = 0;

			// first field is administrative TypeCode 4bit + RecordFlags 4bit
			mapping[position] = 1;
			position++;
			data++;

			// Status Flags (1 byte)
			mapping[position] = 1;
			position++;
			data++;

			// Reason Code (1 byte)
			mapping[position] = 1;
			position++;
			data++;

			// start at time of signal field for decoding
			unsigned int fields = STATUSREPORT_TIMEOF_RECEIPT;

			// start 2 field earlier if we have fragmentation fields
			if ( forFragment() )
			{
				fields -= 2;
			}

			for (int i = fields; i < STATUSREPORT_BUNDLE_SOURCE; i++)
			{
				len = SDNV::len(data);
				mapping[position] = len;
				position++;
				data += len;
			}

			// Das nun folgende Feld ist "Source endpoint ID of Bundle". Die Länge des Felder wird
			// durch den zuletzt decodierten SDNV bestimmt.
			mapping[position] = frame->getSDNV(position - 1);

			// update the size of the frame
			frame->updateSize();
		}

		StatusReportBlock::~StatusReportBlock()
		{
		}

		unsigned int StatusReportBlock::getField(STATUSREPORT_FIELDS field) const
		{
			unsigned int ret = getBodyIndex();

			if ( (field > STATUSREPORT_FRAGMENT_LENGTH) && (!forFragment()) )
			{
				ret -= 2;
			}

			return ret + field;
		}

		unsigned int StatusReportBlock::getFragmentOffset() const
		{
			if ( !forFragment() ) return 0;
			return getFrame().getSDNV( getField(STATUSREPORT_FRAGMENT_OFFSET) );
		}

		void StatusReportBlock::setFragmentOffset(unsigned int value)
		{
			NetworkFrame &frame = getFrame();

			if ( !forFragment() )
			{
				// create fragmentation fields
				frame.insert(getField(STATUSREPORT_FRAGMENT_OFFSET));
				frame.insert(getField(STATUSREPORT_FRAGMENT_LENGTH));

				// set the fragmentation flag
				ProcessingFlags flags = getStatusFlags();
				flags.setFlag(0, true);
				setStatusFlags(flags);
			}

			frame.set(getField(STATUSREPORT_FRAGMENT_OFFSET), value);
		}

		unsigned int StatusReportBlock::getFragmentLength() const
		{
			if ( !forFragment() ) return 0;
			return getFrame().getSDNV( getBodyIndex() + 4 );
		}

		void StatusReportBlock::setFragmentLength(unsigned int value)
		{
			NetworkFrame &frame = getFrame();

			if ( !forFragment() )
			{
				// create fragmentation fields
				frame.insert(getField(STATUSREPORT_FRAGMENT_OFFSET));
				frame.insert(getField(STATUSREPORT_FRAGMENT_LENGTH));

				// set the fragmentation flag
				ProcessingFlags flags = getStatusFlags();
				flags.setFlag(0, true);
				setStatusFlags(flags);
			}

			frame.set(getField(STATUSREPORT_FRAGMENT_LENGTH), value);
		}

		bool StatusReportBlock::forFragment() const
		{
			return getStatusFlags().getFlag( 0 );
		}

		ProcessingFlags StatusReportBlock::getStatusFlag() const
		{
			return ProcessingFlags( getFrame().getChar( getField(STATUSREPORT_STATUS) ) );
		}

		void StatusReportBlock::setStatusFlag(ProcessingFlags value)
		{
			getFrame().set( getField(STATUSREPORT_STATUS), value.getValue() );
		}

		ProcessingFlags StatusReportBlock::getReasonCode() const
		{
			return ProcessingFlags( getFrame().getChar( getField(STATUSREPORT_REASON) ) );
		}

		void StatusReportBlock::setReasonCode(ProcessingFlags value)
		{
			getFrame().set( getField(STATUSREPORT_REASON), value.getValue() );
		}

		unsigned int StatusReportBlock::getTimeOfReceipt() const
		{
			return getFrame().getSDNV( getField(STATUSREPORT_TIMEOF_RECEIPT) );
		}

		void StatusReportBlock::setTimeOfReceipt(unsigned int value)
		{
			getFrame().set( getField(STATUSREPORT_TIMEOF_RECEIPT), value );
		}

		// Time of custody acceptance of bundle
		unsigned int StatusReportBlock::getTimeOfCustodyAcceptance() const
		{
			return getFrame().getSDNV( getField(STATUSREPORT_TIMEOF_CUSTODYACCEPT) );
		}
		void StatusReportBlock::setTimeOfCustodyAcceptance(unsigned int value)
		{
			getFrame().set( getField(STATUSREPORT_TIMEOF_CUSTODYACCEPT), value );
		}

		// Time of forwarding of bundle
		unsigned int StatusReportBlock::getTimeOfForwarding() const
		{
			return getFrame().getSDNV( getField(STATUSREPORT_TIMEOF_FORWARDING) );
		}
		void StatusReportBlock::setTimeOfForwarding(unsigned int value)
		{
			getFrame().set( getField(STATUSREPORT_TIMEOF_FORWARDING), value );
		}

		// Time of delivery of bundle
		unsigned int StatusReportBlock::getTimeOfDelivery() const
		{
			return getFrame().getSDNV( getField(STATUSREPORT_TIMEOF_DELIVERY) );
		}
		void StatusReportBlock::setTimeOfDelivery(unsigned int value)
		{
			getFrame().set( getField(STATUSREPORT_TIMEOF_DELIVERY), value );
		}

		// Time of deletion
		unsigned int StatusReportBlock::getTimeOfDeletion() const
		{
			return getFrame().getSDNV( getField(STATUSREPORT_TIMEOF_DELETION) );
		}
		void StatusReportBlock::setTimeOfDeletion(unsigned int value)
		{
			getFrame().set( getField(STATUSREPORT_TIMEOF_DELETION), value );
		}

		void StatusReportBlock::setCreationTimestamp(unsigned int value)
		{
			getFrame().set( getField(STATUSREPORT_BUNDLE_TIMESTAMP), value );
		}

		unsigned int StatusReportBlock::getCreationTimestamp() const
		{
			return getFrame().getSDNV( getField(STATUSREPORT_BUNDLE_TIMESTAMP) );
		}

		void StatusReportBlock::setCreationTimestampSequence(unsigned int value)
		{
			getFrame().set( getField(STATUSREPORT_BUNDLE_SEQUENCE), value );
		}

		unsigned int StatusReportBlock::getCreationTimestampSequence() const
		{
			return getFrame().getSDNV( getField(STATUSREPORT_BUNDLE_SEQUENCE) );
		}

		void StatusReportBlock::setSource(string value)
		{
			NetworkFrame &frame = Block::getFrame();
			frame.set(getField(STATUSREPORT_BUNDLE_SOURCE_LENGTH), value.length());
			frame.set(getField(STATUSREPORT_BUNDLE_SOURCE), value);
		}

		string StatusReportBlock::getSource() const
		{
			NetworkFrame &frame = Block::getFrame();
			return frame.getString(getField(STATUSREPORT_BUNDLE_SOURCE));
		}

		bool StatusReportBlock::match(Bundle *b) const
		{
			// Prüfe die Fragmentfelder, falls das hier ein Fragment ist.
			if ( b->getPrimaryFlags().isFragment() )
			{
				if ( b->getInteger( FRAGMENTATION_OFFSET ) != getFragmentOffset() ) return false;
				if ( b->getInteger( APPLICATION_DATA_LENGTH ) != getFragmentLength() ) return false;
			}

			if ( b->getInteger( CREATION_TIMESTAMP ) != getCreationTimestamp() ) return false;
			if ( b->getInteger( CREATION_TIMESTAMP_SEQUENCE ) != getCreationTimestampSequence() ) return false;
			if ( b->getSource() != getSource() ) return false;

			return true;
		}

		void StatusReportBlock::setMatch(Bundle *b)
		{
			if (b->getPrimaryFlags().isFragment())
			{
				setFragmentOffset( b->getInteger(FRAGMENTATION_OFFSET) );
				setFragmentLength( b->getInteger(APPLICATION_DATA_LENGTH) );
			}
			setCreationTimestamp( b->getInteger(CREATION_TIMESTAMP) );
			setCreationTimestampSequence( b->getInteger(CREATION_TIMESTAMP_SEQUENCE) );
			setSource( b->getSource() );

			updateBlockSize();
		}
	}
}

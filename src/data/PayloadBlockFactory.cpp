/*
 * PayloadBlockFactory.cpp
 *
 *
 *  Created on: 19.02.2009
 *      Author: morgenro
 */

#include "data/PayloadBlockFactory.h"
#include "data/PayloadBlock.h"
#include "data/BundleFactory.h"
#include <stdio.h>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace dtn
{
	namespace data
	{
		PayloadBlockFactory::PayloadBlockFactory()
		{

		}

		char PayloadBlockFactory::getBlockType() const
		{
			return PayloadBlock::BLOCK_TYPE;
		}

		Block* PayloadBlockFactory::parse(const unsigned char *data, unsigned int size)
		{
			// initially parse the base block
			Block *block = BlockFactory().parse(data, size);

			// create a payload block out of this block
			PayloadBlock *payloadblock = new PayloadBlock(block);

			return payloadblock;
		}

		PayloadBlock* PayloadBlockFactory::copy(PayloadBlock *block)
		{
			NetworkFrame *frame = new NetworkFrame(block->getFrame());
			PayloadBlock *payload = new PayloadBlock(frame);

			switch (AdministrativeBlock::identify( payload ))
			{
				case CUSTODY_SIGNAL:
					return castAdministrativeBlock(payload);
				break;

				case STATUS_REPORT:
					return castAdministrativeBlock(payload);
				break;

				default:
				break;
			}

			return payload;
		}

		PayloadBlock* PayloadBlockFactory::newPayloadBlock(const unsigned char *data, unsigned int size)
		{
			// create a empty PayloadBlock
			PayloadBlock *block = new PayloadBlock( BlockFactory::newBlock() );

			// reference to NetworkFrame of PayloadBlock
			NetworkFrame &frame = block->getFrame();

			// set block type
			frame.set(0, PayloadBlock::BLOCK_TYPE);

			// append payload field
			frame.append(0);

			// set the payload
			block->setPayload(data, size);

			return block;
		}

		StatusReportBlock* PayloadBlockFactory::newStatusReportBlock()
		{
			// create a empty StatusReportBlock
			StatusReportBlock *block = new StatusReportBlock( BlockFactory::newBlock() );

			// reference to NetworkFrame of PayloadBlock
			NetworkFrame &frame = block->getFrame();

			// set block type
			frame.set(0, PayloadBlock::BLOCK_TYPE);

			// set the block type to CUSTODY_SIGNAL
			// first field is administrative TypeCode 4bit + RecordFlags 4bit
			AdministrativeBlockType type(STATUS_REPORT);
			unsigned char field1 = (type << 4);
			frame.append( field1 );

			// Status Flags (1 byte)
			frame.append( (unsigned char)0 );

			// Reason Code (1 byte)
			frame.append( (unsigned char)0 );

			// Time of receipt of bundle
			frame.append( (u_int64_t)0 );

			// Time of custody acceptance of bundle
			frame.append( (u_int64_t)0 );

			// Time of forwarding of bundle
			frame.append( (u_int64_t)0 );

			// Time of delivery of bundle
			frame.append( (u_int64_t)0 );

			// Time of deletion
			frame.append( (u_int64_t)0 );

			// Copy of bundles creation timestamp
			frame.append( 0 );

			// Copy of bundles creation timestamp sequence number
			frame.append( 0 );

			// source endpoint ID
			frame.append( "dtn:none" );

			// update the block size
			block->updateBlockSize();

			return block;
		}

		CustodySignalBlock* PayloadBlockFactory::newCustodySignalBlock(bool accepted)
		{
			// create a empty CustodySignalBlock
			CustodySignalBlock *block = new CustodySignalBlock( BlockFactory::newBlock() );

			// reference to NetworkFrame of PayloadBlock
			NetworkFrame &frame = block->getFrame();

			// set block type
			frame.set(0, PayloadBlock::BLOCK_TYPE);

			// set the type to custody signal
			// first field is administrative TypeCode 4bit + RecordFlags 4bit
			frame.append( (unsigned char)CUSTODY_SIGNAL );

			// Status Flags (1 bit) + 7bit Reason Code
			ProcessingFlags flags;
			flags.setFlag(0, accepted);
			frame.append( (unsigned char)flags.getValue() );

			// Time of Signal
			frame.append( BundleFactory::getDTNTime() );

	//		// unbekanntes Feld
	//		frame.append( 0 );

			// Copy of bundles creation timestamp
			frame.append( 0 );

			// Copy of bundles creation timestamp sequence number
			frame.append( 0 );

			// source endpoint ID
			frame.append( 8 );

			// source endpoint ID
			frame.append( "dtn:none" );

			// update the block size
			block->updateBlockSize();

			return block;
		}

		PayloadBlock* PayloadBlockFactory::merge(PayloadBlock *p1, PayloadBlock *p2, unsigned int p2offset)
		{
			// new size of data
			unsigned int size = p2->getLength() + p2offset;

			// new data array
			unsigned char *data = (unsigned char*)calloc(size, sizeof(char));
			unsigned char *wdata = data;

			// copy data to the array
			memcpy(wdata, p1->getPayload(), p1->getLength());

			// move the pointer to the start of fragment2
			wdata += p2offset;

			// copy data to the array
			memcpy(wdata, p2->getPayload(), p2->getLength());

			// create a payload block
			PayloadBlock *block = PayloadBlockFactory::newPayloadBlock(data, size);

			free(data);

			return block;
		}

		AdministrativeBlock* PayloadBlockFactory::castAdministrativeBlock(PayloadBlock *block)
		{
			// return value
			AdministrativeBlock *ret = NULL;

			// PayloadBlock in einen AdmRecord umwandeln
			AdministrativeBlockType type = AdministrativeBlock::identify( block );

			// dissociate the NetworkFrame from the old object
			NetworkFrame *frame = block->dissociateNetworkFrame();
			delete block;

			switch (type)
			{
				case CUSTODY_SIGNAL:
					ret = new CustodySignalBlock( frame );
				break;

				case STATUS_REPORT:
					ret = new StatusReportBlock( frame );
				break;

				default:
				break;
			}

			return ret;
		}
	}
}

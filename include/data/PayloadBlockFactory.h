/*
 * PayloadBlockFactory.h
 *
 *  Created on: 19.02.2009
 *      Author: morgenro
 */

#include "BlockFactory.h"
#include "PayloadBlock.h"
#include "AdministrativeBlock.h"
#include "CustodySignalBlock.h"
#include "StatusReportBlock.h"

#ifndef PAYLOADBLOCKFACTORY_H_
#define PAYLOADBLOCKFACTORY_H_

namespace dtn { namespace data
{
	class PayloadBlockFactory : public BlockFactory
	{
	public:
		PayloadBlockFactory();

		/**
		 * Parse existing data for fields of this block.
		 * @param[in] data Data-array to parse.
		 * @param[in] size The length of the data-array.
		 * @return The consumed bytes of the data-array.
		 */
		Block* parse(const unsigned char *data, unsigned int size);

		/**
		 * Copy a existing block to a new block
		 * @param[in] block The block to copy.
		 * @return A copy of the given block.
		 */
		PayloadBlock* copy(PayloadBlock *block);

		/**
		 * Create a new PayloadBlock with the given data as payload.
		 * @param[in] data The payload data for the PayloadBlock.
		 * @param[in] size The length of the data-array.
		 * @return PayloadBlock object.
		 */
		static PayloadBlock* newPayloadBlock(const unsigned char *data, unsigned int size);

		static StatusReportBlock* newStatusReportBlock();
		static CustodySignalBlock* newCustodySignalBlock(bool accepted);

		/**
		 * Merge two payloads to one.
		 * @param[in] p1 The first payload.
		 * @param[in] p2 The second payload.
		 * @param[in] p2offset The relative offset of p2 to p1.
		 * @return	  A new payload with the merged data.
		 */
		static PayloadBlock* merge(PayloadBlock *p1, PayloadBlock *p2, unsigned int p2offset);

		/**
		 * Casts a PayloadBlock into a AdministrativeBlock. The PayloadBlock
		 * will be deleted within the execution.
		 * @param[in] block A payload block which contains administrative data.
		 * @return a AdministrativeBlock object.
		 */
		static AdministrativeBlock* castAdministrativeBlock(PayloadBlock *block);

		char getBlockType() const;
	};
} }

#endif /* PAYLOADBLOCKFACTORY_H_ */
